import * as crypto from "node:crypto";
import * as fs from "node:fs";
import * as path from "node:path";
import * as vscode from "vscode";

type CounterMap = Map<string, number>;

type PairCounterMap = Map<string, CounterMap>;

interface FileContribution {
  hash: string;
  trigram: PairCounterMap;
  bigram: PairCounterMap;
  unigram: CounterMap;
  patterns: CounterMap;
  members: CounterMap;
  calls: CounterMap;
  blockSnippets: CounterMap;
  enumVariants: CounterMap;
  apiChains: PairCounterMap;
}

interface PersistedIndex {
  version: 1 | 2;
  workspaceKey: string;
  updatedAt: number;
  files: Array<{
    uri: string;
    hash: string;
    trigram: Array<[string, Array<[string, number]>]>;
    bigram: Array<[string, Array<[string, number]>]>;
    unigram: Array<[string, number]>;
    patterns: Array<[string, number]>;
    members: Array<[string, number]>;
    calls: Array<[string, number]>;
    blockSnippets: Array<[string, number]>;
    enumVariants: Array<[string, number]>;
    apiChains: Array<[string, Array<[string, number]>]>;
  }>;
  acceptance?: Array<[string, Array<[string, number]>]>;
}

export interface LocalInlineEngineOptions {
  enabled: boolean;
  maxFiles: number;
  maxFileSizeKB: number;
  maxSuggestions: number;
  reindexDebounceMs: number;
  persistIndex: boolean;
  expectedValueHints: boolean;
  errorRecoveryHints: boolean;
  multilineEnabled: boolean;
  beamWidth: number;
  maxGeneratedTokens: number;
  apiChainHints: boolean;
  adaptiveLearning: boolean;
  contextClassifierEnabled: boolean;
  fastCacheEnabled: boolean;
  fastCacheSize: number;
  fastCacheTtlMs: number;
}

export class LocalInlineEngine {
  private readonly fileContribByUri = new Map<string, FileContribution>();
  private globalTrigram: PairCounterMap = new Map();
  private globalBigram: PairCounterMap = new Map();
  private globalUnigram: CounterMap = new Map();
  private globalPatterns: CounterMap = new Map();
  private globalMembers: CounterMap = new Map();
  private globalCalls: CounterMap = new Map();
  private globalBlockSnippets: CounterMap = new Map();
  private globalEnumVariants: CounterMap = new Map();
  private globalApiChains: PairCounterMap = new Map();
  private acceptanceByContext: PairCounterMap = new Map();
  private hotSuggestionCache = new Map<string, { ts: number; items: string[] }>();
  private statsRequests = 0;
  private statsCacheHits = 0;
  private statsCacheMisses = 0;
  private statsLatencyMs: number[] = [];
  private persistTimer: NodeJS.Timeout | undefined;
  private storageFile: string | undefined;
  private workspaceKey = "";

  constructor(private readonly opts: LocalInlineEngineOptions) {}

  async initialize(context: vscode.ExtensionContext): Promise<void> {
    const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!workspaceRoot) return;
    this.workspaceKey = crypto.createHash("sha1").update(workspaceRoot).digest("hex").slice(0, 16);
    this.storageFile = path.join(context.globalStorageUri.fsPath, `inline-index-${this.workspaceKey}.json`);
    await fs.promises.mkdir(context.globalStorageUri.fsPath, { recursive: true });
    if (this.opts.persistIndex) {
      await this.loadPersisted();
    }
    await this.primeFromWorkspace();
  }

  dispose(): void {
    if (this.persistTimer) {
      clearTimeout(this.persistTimer);
      this.persistTimer = undefined;
    }
    this.hotSuggestionCache.clear();
  }

  async persistNow(): Promise<void> {
    if (!this.opts.persistIndex || !this.storageFile) return;
    const payload: PersistedIndex = {
      version: 2,
      workspaceKey: this.workspaceKey,
      updatedAt: Date.now(),
      files: [],
      acceptance: serializePairMap(this.acceptanceByContext),
    };
    for (const [uri, c] of this.fileContribByUri.entries()) {
      payload.files.push({
        uri,
        hash: c.hash,
        trigram: serializePairMap(c.trigram),
        bigram: serializePairMap(c.bigram),
        unigram: Array.from(c.unigram.entries()),
        patterns: Array.from(c.patterns.entries()),
        members: Array.from(c.members.entries()),
        calls: Array.from(c.calls.entries()),
        blockSnippets: Array.from(c.blockSnippets.entries()),
        enumVariants: Array.from(c.enumVariants.entries()),
        apiChains: serializePairMap(c.apiChains),
      });
    }
    await fs.promises.writeFile(this.storageFile, `${JSON.stringify(payload)}\n`, "utf8");
  }

  queuePersist(): void {
    if (!this.opts.persistIndex || !this.storageFile) return;
    if (this.persistTimer) clearTimeout(this.persistTimer);
    this.persistTimer = setTimeout(() => {
      void this.persistNow();
    }, Math.max(250, this.opts.reindexDebounceMs));
  }

  upsertDocument(document: vscode.TextDocument): void {
    if (!this.opts.enabled) return;
    if (!isIndexableDocument(document, this.opts.maxFileSizeKB)) return;
    const uri = document.uri.toString();
    const text = document.getText();
    const hash = digest(text);
    const prev = this.fileContribByUri.get(uri);
    if (prev?.hash === hash) return;
    if (prev) this.removeContribution(uri, prev);
    const next = buildContribution(text, hash);
    this.addContribution(uri, next);
    this.queuePersist();
  }

  removeDocument(uri: vscode.Uri): void {
    const key = uri.toString();
    const prev = this.fileContribByUri.get(key);
    if (!prev) return;
    this.removeContribution(key, prev);
    this.queuePersist();
  }

  suggest(
    document: vscode.TextDocument,
    position: vscode.Position,
    maxItems: number,
    opts?: { preferMultiline?: boolean },
  ): string[] {
    const started = Date.now();
    this.statsRequests += 1;
    if (!this.opts.enabled) return [];
    const docContrib = this.fileContribByUri.get(document.uri.toString());
    const line = document.lineAt(position.line).text;
    const left = line.slice(0, position.character);
    const ctxClass = this.opts.contextClassifierEnabled ? classifyInlineContext(left) : "generic";
    const cacheKey = this.buildHotCacheKey(document.uri.toString(), docContrib?.hash ?? "na", left, ctxClass, !!opts?.preferMultiline, maxItems);
    if (this.opts.fastCacheEnabled) {
      const cached = this.readHotCache(cacheKey);
      if (cached) {
        this.statsCacheHits += 1;
        this.pushLatency(Date.now() - started);
        return cached;
      }
    }
    this.statsCacheMisses += 1;
    const weighted = new Map<string, number>();
    const scope = extractLocalScopeContext(document, position);
    const expected = detectExpectedValueContext(document, position);
    const contextKey = buildAcceptanceContextKey(left);

    const pattern = patternSuggestion(left, docContrib?.patterns ?? this.globalPatterns);
    if (pattern) addWeighted(weighted, pattern, 40);
    if (this.opts.expectedValueHints) {
      addExpectedValueSuggestions(weighted, expected, docContrib?.enumVariants ?? this.globalEnumVariants);
    }
    if (this.opts.adaptiveLearning) {
      const learnt = this.acceptanceByContext.get(contextKey);
      if (learnt) {
        for (const [text, score] of learnt.entries()) addWeighted(weighted, text, score * 14);
      }
    }
    applyContextClassBoost(weighted, ctxClass);

    const memberPrefix = /\.([A-Za-z_][A-Za-z0-9_]*)?$/.exec(left)?.[1] ?? "";
    if (left.endsWith(".") || /\.[A-Za-z_][A-Za-z0-9_]*$/.test(left)) {
      const localMembers = topStartingWith(docContrib?.members ?? new Map(), memberPrefix, 6);
      const fileMembers = topStartingWith(this.globalMembers, memberPrefix, 6);
      for (const name of localMembers) addWeighted(weighted, name.slice(memberPrefix.length), 30);
      for (const name of fileMembers) addWeighted(weighted, name.slice(memberPrefix.length), 10);
    }

    const tokens = tokenize(left);
    const p1 = tokens[tokens.length - 1] ?? "";
    const p2 = tokens[tokens.length - 2] ?? "";
    const next = this.predictTokens(docContrib, p2, p1, 8);
    const wordPrefix = /[A-Za-z_][A-Za-z0-9_]*$/.exec(left)?.[0] ?? "";
    for (const tok of next) {
      if (wordPrefix && tok.startsWith(wordPrefix)) {
        addWeighted(weighted, tok.slice(wordPrefix.length), 16);
      } else if (!wordPrefix) {
        addWeighted(weighted, tokenToInline(tok), 12);
      }
    }

    const tailIdent = /\b([A-Za-z_][A-Za-z0-9_]*)$/.exec(left)?.[1];
    if (tailIdent && !left.endsWith("(")) {
      const partial = tailIdent;
      const localCalls = topStartingWith(docContrib?.calls ?? new Map(), partial, 6);
      const repoCalls = topStartingWith(this.globalCalls, partial, 6);
      for (const fn of localCalls) {
        addWeighted(weighted, `${fn.slice(partial.length)}(`, 28);
      }
      for (const fn of repoCalls) {
        addWeighted(weighted, `${fn.slice(partial.length)}(`, 10);
      }
    }
    if (this.opts.errorRecoveryHints) {
      addRecoverySuggestions(weighted, left);
    }
    for (const localVar of scope.locals) {
      if (tailIdent && localVar.startsWith(tailIdent)) {
        addWeighted(weighted, localVar.slice(tailIdent.length), 36);
      } else if (!tailIdent) {
        addWeighted(weighted, localVar, 8);
      }
    }
    if (scope.nonNullLikelyVar && /^(?:\s*if\s+.*\{\s*)?$/.test(left)) {
      addWeighted(weighted, `\n\t${scope.nonNullLikelyVar}.`, 18);
    }
    const blockSeeds = topStartingWith(this.globalBlockSnippets, blockSnippetPrefix(left), opts?.preferMultiline ? 12 : 6);
    for (const s of blockSeeds) {
      const snippet = s.replace(/^[a-z]+:/, "");
      addWeighted(weighted, snippet, opts?.preferMultiline ? 26 : 14);
    }
    if (this.opts.multilineEnabled) {
      const generated = this.generateTokenContinuations(
        left,
        docContrib,
        this.opts.maxGeneratedTokens,
        this.opts.beamWidth,
      );
      for (const g of generated) addWeighted(weighted, g.text, g.score + (opts?.preferMultiline ? 12 : 0));
    }
    if (this.opts.apiChainHints && /(?:\.[A-Za-z_][A-Za-z0-9_]*)$/.test(left)) {
      const baseMember = /\.([A-Za-z_][A-Za-z0-9_]*)$/.exec(left)?.[1];
      if (baseMember) {
        const nexts = docContrib?.apiChains.get(baseMember) ?? this.globalApiChains.get(baseMember);
        if (nexts) {
          for (const [member, score] of Array.from(nexts.entries()).sort((a, b) => b[1] - a[1]).slice(0, 6)) {
            addWeighted(weighted, `.${member}`, score * 6);
            addWeighted(weighted, `.${member}(`, score * 8);
          }
        }
      }
    }
    const limit = Math.max(1, Math.min(maxItems, this.opts.maxSuggestions));
    const result = Array.from(weighted.entries())
      .filter(([text]) => text.length > 0)
      .sort((a, b) => b[1] - a[1])
      .map(([text]) => text)
      .slice(0, limit);
    if (this.opts.fastCacheEnabled) this.writeHotCache(cacheKey, result);
    this.pushLatency(Date.now() - started);
    return result;
  }

  noteAccepted(leftContext: string, acceptedText: string): void {
    if (!this.opts.adaptiveLearning) return;
    const ctx = buildAcceptanceContextKey(leftContext);
    const trimmed = acceptedText.trim();
    if (!trimmed) return;
    incPair(this.acceptanceByContext, ctx, trimmed, 1);
    this.hotSuggestionCache.clear();
    this.queuePersist();
  }

  getStats(): {
    requests: number;
    cacheHits: number;
    cacheMisses: number;
    cacheHitRate: number;
    latencyP50Ms: number;
    latencyP95Ms: number;
    indexedFiles: number;
    acceptanceContexts: number;
    acceptanceEntries: number;
    hotCacheEntries: number;
  } {
    const acceptanceEntries = Array.from(this.acceptanceByContext.values())
      .reduce((acc, row) => acc + row.size, 0);
    const req = Math.max(1, this.statsRequests);
    return {
      requests: this.statsRequests,
      cacheHits: this.statsCacheHits,
      cacheMisses: this.statsCacheMisses,
      cacheHitRate: this.statsCacheHits / req,
      latencyP50Ms: percentile(this.statsLatencyMs, 0.5),
      latencyP95Ms: percentile(this.statsLatencyMs, 0.95),
      indexedFiles: this.fileContribByUri.size,
      acceptanceContexts: this.acceptanceByContext.size,
      acceptanceEntries,
      hotCacheEntries: this.hotSuggestionCache.size,
    };
  }

  private buildHotCacheKey(
    uri: string,
    hash: string,
    left: string,
    ctxClass: InlineContextClass,
    preferMultiline: boolean,
    limit: number,
  ): string {
    const tail = left.slice(Math.max(0, left.length - 240));
    const digestTail = crypto.createHash("sha1").update(tail).digest("hex").slice(0, 16);
    return `${uri}::${hash}::${ctxClass}::${preferMultiline ? "ml" : "sl"}::${limit}::${digestTail}`;
  }

  private readHotCache(key: string): string[] | undefined {
    const hit = this.hotSuggestionCache.get(key);
    if (!hit) return undefined;
    if ((Date.now() - hit.ts) > Math.max(20, this.opts.fastCacheTtlMs)) {
      this.hotSuggestionCache.delete(key);
      return undefined;
    }
    this.hotSuggestionCache.delete(key);
    this.hotSuggestionCache.set(key, hit);
    return hit.items;
  }

  private writeHotCache(key: string, items: string[]): void {
    this.hotSuggestionCache.set(key, { ts: Date.now(), items });
    const max = Math.max(32, this.opts.fastCacheSize);
    while (this.hotSuggestionCache.size > max) {
      const first = this.hotSuggestionCache.keys().next().value;
      if (typeof first !== "string") break;
      this.hotSuggestionCache.delete(first);
    }
  }

  private pushLatency(ms: number): void {
    this.statsLatencyMs.push(Math.max(0, ms));
    if (this.statsLatencyMs.length > 2048) this.statsLatencyMs.shift();
  }

  private predictTokens(docContrib: FileContribution | undefined, prev2: string, prev1: string, limit: number): string[] {
    const weighted = new Map<string, number>();
    const localTri = docContrib?.trigram.get(`${prev2}\u0000${prev1}`);
    if (localTri) {
      for (const [tok, score] of localTri.entries()) weighted.set(tok, (weighted.get(tok) ?? 0) + score * 7);
    }
    const localBi = docContrib?.bigram.get(prev1);
    if (localBi) {
      for (const [tok, score] of localBi.entries()) weighted.set(tok, (weighted.get(tok) ?? 0) + score * 5);
    }
    for (const [tok, score] of docContrib?.unigram.entries() ?? []) {
      weighted.set(tok, (weighted.get(tok) ?? 0) + score * 0.5);
    }
    const tri = this.globalTrigram.get(`${prev2}\u0000${prev1}`);
    if (tri) {
      for (const [tok, score] of tri.entries()) weighted.set(tok, (weighted.get(tok) ?? 0) + score * 3);
    }
    const bi = this.globalBigram.get(prev1);
    if (bi) {
      for (const [tok, score] of bi.entries()) weighted.set(tok, (weighted.get(tok) ?? 0) + score * 2);
    }
    for (const [tok, score] of this.globalUnigram.entries()) {
      weighted.set(tok, (weighted.get(tok) ?? 0) + score * 0.25);
    }
    return Array.from(weighted.entries())
      .sort((a, b) => b[1] - a[1])
      .map(([tok]) => tok)
      .filter((tok) => tok.length > 0 && tok !== prev1)
      .slice(0, limit);
  }

  private addContribution(uri: string, c: FileContribution): void {
    this.fileContribByUri.set(uri, c);
    addPairCounters(this.globalTrigram, c.trigram);
    addPairCounters(this.globalBigram, c.bigram);
    addCounters(this.globalUnigram, c.unigram);
    addCounters(this.globalPatterns, c.patterns);
    addCounters(this.globalMembers, c.members);
    addCounters(this.globalCalls, c.calls);
    addCounters(this.globalBlockSnippets, c.blockSnippets);
    addCounters(this.globalEnumVariants, c.enumVariants);
    addPairCounters(this.globalApiChains, c.apiChains);
  }

  private removeContribution(uri: string, c: FileContribution): void {
    this.fileContribByUri.delete(uri);
    subtractPairCounters(this.globalTrigram, c.trigram);
    subtractPairCounters(this.globalBigram, c.bigram);
    subtractCounters(this.globalUnigram, c.unigram);
    subtractCounters(this.globalPatterns, c.patterns);
    subtractCounters(this.globalMembers, c.members);
    subtractCounters(this.globalCalls, c.calls);
    subtractCounters(this.globalBlockSnippets, c.blockSnippets);
    subtractCounters(this.globalEnumVariants, c.enumVariants);
    subtractPairCounters(this.globalApiChains, c.apiChains);
  }

  private async primeFromWorkspace(): Promise<void> {
    const files = await vscode.workspace.findFiles("**/*.{vit,vitte,vitl}", "**/{node_modules,.git,out,dist,build,target}/**", this.opts.maxFiles);
    for (const uri of files) {
      try {
        const stat = await fs.promises.stat(uri.fsPath);
        if ((stat.size / 1024) > this.opts.maxFileSizeKB) continue;
        const text = await fs.promises.readFile(uri.fsPath, "utf8");
        const hash = digest(text);
        const existing = this.fileContribByUri.get(uri.toString());
        if (existing?.hash === hash) continue;
        if (existing) this.removeContribution(uri.toString(), existing);
        this.addContribution(uri.toString(), buildContribution(text, hash));
      } catch {
        // ignore unreadable files
      }
    }
    this.queuePersist();
  }

  private async loadPersisted(): Promise<void> {
    if (!this.storageFile || !fs.existsSync(this.storageFile)) return;
    try {
      const raw = await fs.promises.readFile(this.storageFile, "utf8");
      const parsed = JSON.parse(raw) as PersistedIndex;
      if ((parsed.version !== 1 && parsed.version !== 2) || parsed.workspaceKey !== this.workspaceKey) return;
      this.acceptanceByContext = deserializePairMap(parsed.acceptance ?? []);
      for (const file of parsed.files) {
        const c: FileContribution = {
          hash: file.hash,
          trigram: deserializePairMap(file.trigram),
          bigram: deserializePairMap(file.bigram),
          unigram: new Map(file.unigram),
          patterns: new Map(file.patterns),
          members: new Map(file.members),
          calls: new Map(file.calls),
          blockSnippets: new Map((file as { blockSnippets?: Array<[string, number]> }).blockSnippets ?? []),
          enumVariants: new Map((file as { enumVariants?: Array<[string, number]> }).enumVariants ?? []),
          apiChains: deserializePairMap((file as { apiChains?: Array<[string, Array<[string, number]>]> }).apiChains ?? []),
        };
        this.addContribution(file.uri, c);
      }
    } catch {
      // ignore invalid cache
    }
  }

  private generateTokenContinuations(
    left: string,
    docContrib: FileContribution | undefined,
    maxTokens: number,
    beamWidth: number,
  ): Array<{ text: string; score: number }> {
    const ctx = tokenize(left);
    let beams: Array<{ p2: string; p1: string; seq: string[]; score: number }> = [{
      p2: ctx[ctx.length - 2] ?? "",
      p1: ctx[ctx.length - 1] ?? "",
      seq: [],
      score: 0,
    }];
    const steps = Math.max(2, Math.min(64, maxTokens));
    const width = Math.max(2, Math.min(16, beamWidth));
    for (let i = 0; i < steps; i += 1) {
      const nextBeams: Array<{ p2: string; p1: string; seq: string[]; score: number }> = [];
      for (const b of beams) {
        const candidates = this.predictTokenScores(docContrib, b.p2, b.p1, 8);
        for (const [tok, sc] of candidates) {
          const repeatedPenalty = b.seq.length > 0 && b.seq[b.seq.length - 1] === tok ? 2 : 0;
          nextBeams.push({
            p2: b.p1,
            p1: tok,
            seq: [...b.seq, tok],
            score: b.score + sc - repeatedPenalty,
          });
        }
      }
      beams = nextBeams.sort((a, b) => b.score - a.score).slice(0, width);
      if (beams.every((b) => terminalToken(b.p1))) break;
    }
    return beams
      .map((b) => ({ text: tokensToInlineText(b.seq), score: b.score }))
      .filter((r) => r.text.trim().length > 0)
      .slice(0, Math.max(2, Math.floor(width / 2)));
  }

  private predictTokenScores(
    docContrib: FileContribution | undefined,
    prev2: string,
    prev1: string,
    limit: number,
  ): Array<[string, number]> {
    const weighted = new Map<string, number>();
    const addRows = (row: CounterMap | undefined, w: number): void => {
      if (!row) return;
      for (const [tok, score] of row.entries()) {
        weighted.set(tok, (weighted.get(tok) ?? 0) + score * w);
      }
    };
    addRows(docContrib?.trigram.get(`${prev2}\u0000${prev1}`), 7);
    addRows(docContrib?.bigram.get(prev1), 5);
    addRows(this.globalTrigram.get(`${prev2}\u0000${prev1}`), 3);
    addRows(this.globalBigram.get(prev1), 2);
    return Array.from(weighted.entries()).sort((a, b) => b[1] - a[1]).slice(0, limit);
  }
}

function digest(text: string): string {
  return crypto.createHash("sha1").update(text).digest("hex");
}

function tokenize(text: string): string[] {
  return text.match(/[A-Za-z_][A-Za-z0-9_]*|\d+|==|!=|<=|>=|=>|->|&&|\|\||[{}()[\].,;:+\-*/%<>:=]/g) ?? [];
}

function buildContribution(text: string, hash: string): FileContribution {
  const tokens = tokenize(text);
  const trigram: PairCounterMap = new Map();
  const bigram: PairCounterMap = new Map();
  const unigram: CounterMap = new Map();
  const patterns: CounterMap = new Map();
  const members: CounterMap = new Map();
  const calls: CounterMap = new Map();
  const blockSnippets: CounterMap = new Map();
  const enumVariants: CounterMap = new Map();
  const apiChains: PairCounterMap = new Map();

  for (let i = 0; i < tokens.length; i += 1) {
    const t = tokens[i] ?? "";
    if (!t) continue;
    inc(unigram, t, 1);
    const p1 = tokens[i - 1] ?? "";
    const p2 = tokens[i - 2] ?? "";
    if (p1) incPair(bigram, p1, t, 1);
    if (p1 && p2) incPair(trigram, `${p2}\u0000${p1}`, t, 1);
  }

  for (const m of text.matchAll(/\.([A-Za-z_][A-Za-z0-9_]*)/g)) {
    if (m[1]) inc(members, m[1], 1);
  }
  for (const m of text.matchAll(/\b([A-Za-z_][A-Za-z0-9_]*)\s*\(/g)) {
    const fn = m[1];
    if (!fn || KEYWORDS.has(fn)) continue;
    inc(calls, fn, 1);
  }

  for (const line of text.split(/\r?\n/)) {
    const trim = line.trim();
    if (/^(if|for|while|match)\b/.test(trim)) inc(patterns, "control_block", 1);
    if (/^(proc|fn)\s+[A-Za-z_][A-Za-z0-9_]*\s*\([^)]*\)\s*$/.test(trim)) inc(patterns, "fn_block", 1);
    if (/^test\s+"[^"]*"\s*$/.test(trim)) inc(patterns, "test_block", 1);
    if (/^let\s+[A-Za-z_][A-Za-z0-9_]*\s*=/.test(trim)) inc(patterns, "let_assign", 1);
  }
  harvestBlockSnippets(text, blockSnippets);
  for (const m of text.matchAll(/\b([A-Z][A-Za-z0-9_]*)::([A-Z][A-Za-z0-9_]*)\b/g)) {
    const variant = `${m[1]}::${m[2]}`;
    inc(enumVariants, variant, 1);
  }
  harvestApiChains(tokens, apiChains);
  return { hash, trigram, bigram, unigram, patterns, members, calls, blockSnippets, enumVariants, apiChains };
}

function patternSuggestion(left: string, patterns: CounterMap): string | undefined {
  if (/^(if|for|while|match)\s+[^{]*$/.test(left.trim())) {
    if ((patterns.get("control_block") ?? 0) > 0) {
      return left.trimStart().startsWith("match") ? " {\n\t_ => \n}" : " {\n\t\n}";
    }
  }
  if (/^(proc|fn)\s+[A-Za-z_][A-Za-z0-9_]*\s*\([^)]*\)\s*$/.test(left.trim())) {
    return " {\n\t\n}";
  }
  if (/^test\s+"[^"]*"\s*$/.test(left.trim())) {
    return " {\n\t\n}";
  }
  return undefined;
}

function tokenToInline(tok: string): string {
  if (/^[A-Za-z_][A-Za-z0-9_]*$/.test(tok)) return tok;
  if (/^[,.;:)}\]]$/.test(tok)) return tok;
  if (/^(?:\(|\{|\[)$/.test(tok)) return `${tok}`;
  return ` ${tok}`;
}

function topStartingWith(counter: CounterMap, prefix: string, limit: number): string[] {
  return Array.from(counter.entries())
    .filter(([k]) => k.startsWith(prefix))
    .sort((a, b) => b[1] - a[1])
    .map(([k]) => k)
    .slice(0, limit);
}

function addWeighted(weighted: Map<string, number>, text: string, score: number): void {
  if (!text) return;
  weighted.set(text, (weighted.get(text) ?? 0) + score);
}

function extractLocalScopeContext(document: vscode.TextDocument, position: vscode.Position): {
  locals: string[];
  nonNullLikelyVar?: string;
} {
  const from = Math.max(0, position.line - 80);
  const locals = new Map<string, number>();
  let nonNullLikelyVar: string | undefined;
  for (let line = position.line; line >= from; line -= 1) {
    const text = document.lineAt(line).text;
    for (const m of text.matchAll(/\blet\s+([A-Za-z_][A-Za-z0-9_]*)\b/g)) {
      const id = m[1];
      if (id) locals.set(id, (locals.get(id) ?? 0) + 1 + ((position.line - line) < 10 ? 3 : 0));
    }
    for (const m of text.matchAll(/\b(?:proc|fn)\s+[A-Za-z_][A-Za-z0-9_]*\s*\(([^)]*)\)/g)) {
      const params = m[1] ?? "";
      for (const p of params.split(",")) {
        const id = /\b([A-Za-z_][A-Za-z0-9_]*)\b/.exec(p)?.[1];
        if (id) locals.set(id, (locals.get(id) ?? 0) + 2);
      }
    }
    const guard = /\bif\s+([A-Za-z_][A-Za-z0-9_]*)\s*!=\s*null\b/.exec(text)
      ?? /\bif\s+([A-Za-z_][A-Za-z0-9_]*)\s*is\s+not\s+null\b/.exec(text);
    if (!nonNullLikelyVar && guard?.[1]) nonNullLikelyVar = guard[1];
  }
  const ordered = Array.from(locals.entries()).sort((a, b) => b[1] - a[1]).map(([k]) => k).slice(0, 16);
  return { locals: ordered, nonNullLikelyVar };
}

function blockSnippetPrefix(left: string): string {
  const t = left.trimStart();
  if (t.startsWith("if ")) return "if:";
  if (t.startsWith("for ")) return "for:";
  if (t.startsWith("while ")) return "while:";
  if (t.startsWith("match ")) return "match:";
  if (t.startsWith("fn ") || t.startsWith("proc ")) return "fn:";
  return "any:";
}

function harvestBlockSnippets(text: string, out: CounterMap): void {
  const lines = text.split(/\r?\n/);
  for (let i = 0; i < lines.length; i += 1) {
    const cur = lines[i]?.trim() ?? "";
    const key = cur.startsWith("if ") ? "if:"
      : cur.startsWith("for ") ? "for:"
      : cur.startsWith("while ") ? "while:"
      : cur.startsWith("match ") ? "match:"
      : (cur.startsWith("fn ") || cur.startsWith("proc ")) ? "fn:"
      : "";
    if (!key) continue;
    const a = lines[i + 1]?.trimEnd() ?? "";
    const b = lines[i + 2]?.trimEnd() ?? "";
    if (!a) continue;
    const snippet = `\n${a}${b ? `\n${b}` : ""}`;
    inc(out, `${key}${snippet}`, 1);
    inc(out, `any:${snippet}`, 1);
  }
}

type ExpectedValueContext = "bool" | "string" | "number" | "enum_like" | "none";

function detectExpectedValueContext(document: vscode.TextDocument, position: vscode.Position): ExpectedValueContext {
  const line = document.lineAt(position.line).text.slice(0, position.character);
  const trimmed = line.trim();
  if (/=\s*$/.test(trimmed) || /\breturn\s+$/.test(trimmed)) {
    const nearby = collectNearbyTypeHints(document, position);
    if (nearby.has("bool")) return "bool";
    if (nearby.has("string")) return "string";
    if (nearby.has("number")) return "number";
    if (nearby.has("enum")) return "enum_like";
  }
  if (/(==|!=)\s*$/.test(trimmed)) return "bool";
  if (/:\s*$/.test(trimmed)) return "enum_like";
  return "none";
}

function collectNearbyTypeHints(document: vscode.TextDocument, position: vscode.Position): Set<string> {
  const out = new Set<string>();
  const from = Math.max(0, position.line - 30);
  for (let i = position.line; i >= from; i -= 1) {
    const line = document.lineAt(i).text;
    if (/\bbool\b/.test(line)) out.add("bool");
    if (/\bstring\b/.test(line)) out.add("string");
    if (/\b(?:i32|i64|u32|u64|int|float|f32|f64)\b/.test(line)) out.add("number");
    if (/\b[A-Z][A-Za-z0-9_]*::[A-Z][A-Za-z0-9_]*\b/.test(line)) out.add("enum");
  }
  return out;
}

function addExpectedValueSuggestions(
  weighted: Map<string, number>,
  expected: ExpectedValueContext,
  enumVariants: CounterMap,
): void {
  if (expected === "bool") {
    addWeighted(weighted, "true", 34);
    addWeighted(weighted, "false", 34);
    return;
  }
  if (expected === "string") {
    addWeighted(weighted, "\"\"", 32);
    addWeighted(weighted, "\"TODO\"", 20);
    return;
  }
  if (expected === "number") {
    addWeighted(weighted, "0", 28);
    addWeighted(weighted, "1", 22);
    return;
  }
  if (expected === "enum_like") {
    const top = topStartingWith(enumVariants, "", 6);
    for (const variant of top) addWeighted(weighted, variant, 30);
  }
}

function addRecoverySuggestions(weighted: Map<string, number>, left: string): void {
  const opens = (left.match(/\(/g) ?? []).length;
  const closes = (left.match(/\)/g) ?? []).length;
  if (opens > closes) addWeighted(weighted, ")", 18);
  const bOpens = (left.match(/\[/g) ?? []).length;
  const bCloses = (left.match(/\]/g) ?? []).length;
  if (bOpens > bCloses) addWeighted(weighted, "]", 16);
  const cOpens = (left.match(/\{/g) ?? []).length;
  const cCloses = (left.match(/\}/g) ?? []).length;
  if (cOpens > cCloses) addWeighted(weighted, "\n}", 14);
}

function harvestApiChains(tokens: string[], out: PairCounterMap): void {
  for (let i = 0; i < tokens.length - 4; i += 1) {
    const a = tokens[i];
    const d1 = tokens[i + 1];
    const b = tokens[i + 2];
    const d2 = tokens[i + 3];
    const c = tokens[i + 4];
    if (!a || !b || !c || d1 !== "." || d2 !== ".") continue;
    if (!isIdent(a) || !isIdent(b) || !isIdent(c)) continue;
    incPair(out, b, c, 1);
  }
}

function terminalToken(tok: string): boolean {
  return tok === ";" || tok === "}" || tok === ")" || tok === "]";
}

function buildAcceptanceContextKey(left: string): string {
  const toks = tokenize(left);
  const tail = toks.slice(Math.max(0, toks.length - 5)).join(" ");
  const shape = /\.[A-Za-z_][A-Za-z0-9_]*$/.test(left) ? "member"
    : /\b[A-Za-z_][A-Za-z0-9_]*$/.test(left) ? "ident"
    : /[({[]\s*$/.test(left) ? "open"
    : "other";
  return `${shape}::${tail}`;
}

type InlineContextClass = "declaration" | "callsite" | "member" | "guard" | "return" | "test" | "generic";

function classifyInlineContext(left: string): InlineContextClass {
  const t = left.trimStart();
  if (/^(?:let|const)\s+[A-Za-z_][A-Za-z0-9_]*\s*=?\s*$/.test(t)) return "declaration";
  if (/^return\b/.test(t)) return "return";
  if (/^if\b/.test(t) || /^while\b/.test(t)) return "guard";
  if (/^test\b/.test(t)) return "test";
  if (/\.[A-Za-z_][A-Za-z0-9_]*$/.test(t) || t.endsWith(".")) return "member";
  if (/[A-Za-z_][A-Za-z0-9_]*\($/.test(t) || /,\s*$/.test(t)) return "callsite";
  return "generic";
}

function applyContextClassBoost(weighted: Map<string, number>, klass: InlineContextClass): void {
  if (klass === "declaration") {
    addWeighted(weighted, " = ", 10);
    addWeighted(weighted, "0", 6);
    addWeighted(weighted, "\"\"", 6);
  } else if (klass === "return") {
    addWeighted(weighted, ";", 9);
  } else if (klass === "guard") {
    addWeighted(weighted, " {", 10);
    addWeighted(weighted, " != null", 8);
  } else if (klass === "test") {
    addWeighted(weighted, " {\n\t\n}", 12);
  } else if (klass === "callsite") {
    addWeighted(weighted, ")", 10);
  } else if (klass === "member") {
    addWeighted(weighted, "(", 7);
  }
}

function tokensToInlineText(tokens: string[]): string {
  const parts: string[] = [];
  for (const tok of tokens) {
    if (tok === "}") {
      parts.push("\n}");
      break;
    }
    if (/^[,.;:)}\]]$/.test(tok)) {
      parts.push(tok);
      continue;
    }
    if (tok === "{") {
      parts.push(" {\n\t");
      continue;
    }
    if (tok === ";") {
      parts.push(";\n");
      continue;
    }
    if (tok === ".") {
      parts.push(".");
      continue;
    }
    if (parts.length === 0) parts.push(tok);
    else parts.push(` ${tok}`);
  }
  return parts.join("").replace(/\s+\n/g, "\n");
}

function isIdent(tok: string): boolean {
  return /^[A-Za-z_][A-Za-z0-9_]*$/.test(tok);
}

function percentile(values: number[], q: number): number {
  if (values.length === 0) return 0;
  const xs = [...values].sort((a, b) => a - b);
  const i = Math.max(0, Math.min(xs.length - 1, Math.round((xs.length - 1) * q)));
  return xs[i] ?? 0;
}

function inc(map: CounterMap, key: string, delta: number): void {
  map.set(key, (map.get(key) ?? 0) + delta);
}

function incPair(map: PairCounterMap, key: string, next: string, delta: number): void {
  let row = map.get(key);
  if (!row) {
    row = new Map();
    map.set(key, row);
  }
  row.set(next, (row.get(next) ?? 0) + delta);
}

function addCounters(dst: CounterMap, src: CounterMap): void {
  for (const [k, v] of src.entries()) inc(dst, k, v);
}

function subtractCounters(dst: CounterMap, src: CounterMap): void {
  for (const [k, v] of src.entries()) {
    const next = (dst.get(k) ?? 0) - v;
    if (next <= 0) dst.delete(k);
    else dst.set(k, next);
  }
}

function addPairCounters(dst: PairCounterMap, src: PairCounterMap): void {
  for (const [k, row] of src.entries()) {
    for (const [next, v] of row.entries()) {
      incPair(dst, k, next, v);
    }
  }
}

function subtractPairCounters(dst: PairCounterMap, src: PairCounterMap): void {
  for (const [k, row] of src.entries()) {
    const current = dst.get(k);
    if (!current) continue;
    for (const [next, v] of row.entries()) {
      const rem = (current.get(next) ?? 0) - v;
      if (rem <= 0) current.delete(next);
      else current.set(next, rem);
    }
    if (current.size === 0) dst.delete(k);
  }
}

function serializePairMap(map: PairCounterMap): Array<[string, Array<[string, number]>]> {
  const out: Array<[string, Array<[string, number]>]> = [];
  for (const [k, row] of map.entries()) out.push([k, Array.from(row.entries())]);
  return out;
}

function deserializePairMap(rows: Array<[string, Array<[string, number]>]>): PairCounterMap {
  const out: PairCounterMap = new Map();
  for (const [k, vals] of rows) out.set(k, new Map(vals));
  return out;
}

function isIndexableDocument(document: vscode.TextDocument, maxFileSizeKB: number): boolean {
  if (document.uri.scheme !== "file") return false;
  if (document.languageId !== "vitte" && document.languageId !== "vit") return false;
  const bytes = Buffer.byteLength(document.getText(), "utf8");
  return (bytes / 1024) <= maxFileSizeKB;
}

const KEYWORDS = new Set([
  "if", "else", "for", "while", "match", "let", "const", "return", "break", "continue", "fn", "proc", "test", "import", "export",
]);
