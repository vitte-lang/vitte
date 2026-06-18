import * as crypto from "node:crypto";
import * as fs from "node:fs";
import * as cp from "node:child_process";
import * as path from "node:path";
import { Worker } from "node:worker_threads";
import * as vscode from "vscode";

interface Chunk {
  id: string;
  uri: string;
  text: string;
  tokens: Map<string, number>;
}

interface FileDoc {
  hash: string;
  chunks: Chunk[];
  symbols: Set<string>;
}

interface Candidate {
  text: string;
  score: number;
  source: "backend" | "local";
}

export interface AiInlinePipelineOptions {
  enabled: boolean;
  backendEnabled: boolean;
  backendUrl: string;
  backendTimeoutMs: number;
  ragTopK: number;
  maxCandidates: number;
  antiHallucination: boolean;
  allowUnknownSymbols: boolean;
  styleMode: "project" | "user";
  userMode: "aggressive" | "balanced" | "conservative";
  granularity: "classic_only" | "inline_only" | "hybrid";
  privacyStrict: boolean;
  backendAllowlist: string[];
  redactSecrets: boolean;
  trustedWorkspaceOnly: boolean;
  cloudOptIn: boolean;
  localOnly: boolean;
  dataRetentionDays: number;
  allowExternalTraining: boolean;
  useWorkerIndexing: boolean;
  promptCacheEnabled: boolean;
  promptCacheTtlMs: number;
  promptCacheSize: number;
}

export interface AiInlineTelemetry {
  requests: number;
  backendRequests: number;
  backendFailures: number;
  backendTimeouts: number;
  fallbackLocalCount: number;
  acceptedLikeCount: number;
  avgLatencyMs: number;
  p95LatencyMs: number;
  chunks: number;
  files: number;
  avgCpuMicros: number;
  p95CpuMicros: number;
  avgMemDeltaKb: number;
  p95MemDeltaKb: number;
}

export class AiInlinePipeline {
  private readonly byUri = new Map<string, FileDoc>();
  private readonly chunks: Chunk[] = [];
  private readonly symbols = new Set<string>();
  private stats = {
    requests: 0,
    backendRequests: 0,
    backendFailures: 0,
    backendTimeouts: 0,
    fallbackLocalCount: 0,
    acceptedLikeCount: 0,
    latencies: [] as number[],
  };
  private workspaceContext = {
    buildConfig: "",
    testConfig: "",
    externalDeps: [] as string[],
    gitRecent: [] as string[],
  };
  private promptCache = new Map<string, { ts: number; chunks: Chunk[] }>();
  private worker: Worker | undefined;
  private workerPending = new Map<string, (value: { chunks: Chunk[]; symbols: Set<string> }) => void>();
  private statsCpuMicros: number[] = [];
  private statsMemDeltaKb: number[] = [];

  constructor(private readonly opts: AiInlinePipelineOptions) {}

  async initialize(): Promise<void> {
    if (!this.opts.enabled) return;
    if (this.opts.useWorkerIndexing) this.ensureWorker();
    const files = await vscode.workspace.findFiles("**/*.{vit,vitte,vitl}", "**/{node_modules,.git,out,dist,build,target}/**", 5000);
    for (const uri of files) {
      try {
        const text = await fs.promises.readFile(uri.fsPath, "utf8");
        await this.upsert(uri.toString(), text);
      } catch {
        // ignore unreadable files
      }
    }
    await this.refreshWorkspaceContext();
  }
  
  async refreshWorkspaceContext(): Promise<void> {
    const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!root) return;
    this.workspaceContext.buildConfig = await readSmallFileIfExists(`${root}/vitte.config.json`);
    this.workspaceContext.testConfig = await readSmallFileIfExists(`${root}/.vitte/config.json`);
    this.workspaceContext.externalDeps = collectExternalDeps(this.byUri);
    this.workspaceContext.gitRecent = readGitRecentFiles(root);
  }

  async upsert(uri: string, text: string): Promise<void> {
    if (!this.opts.enabled) return;
    const hash = digest(text);
    const prev = this.byUri.get(uri);
    if (prev?.hash === hash) return;
    if (prev) this.remove(uri);
    const parsed = await this.parseWithBestPath(uri, text);
    const symbols = parsed.symbols;
    const chunks = parsed.chunks;
    this.byUri.set(uri, { hash, chunks, symbols });
    for (const s of symbols) this.symbols.add(s);
    for (const ch of chunks) this.chunks.push(ch);
    this.promptCache.clear();
  }

  remove(uri: string): void {
    const prev = this.byUri.get(uri);
    if (!prev) return;
    this.byUri.delete(uri);
    for (let i = this.chunks.length - 1; i >= 0; i -= 1) {
      if (this.chunks[i]?.uri === uri) this.chunks.splice(i, 1);
    }
    this.rebuildSymbols();
    this.promptCache.clear();
  }

  async suggest(
    document: vscode.TextDocument,
    position: vscode.Position,
    localCandidates: string[],
  ): Promise<string[]> {
    if (!this.opts.enabled) return localCandidates;
    if (this.opts.localOnly) return localCandidates;
    if (this.opts.granularity === "classic_only") return [];
    if (this.opts.granularity === "inline_only") localCandidates = [];
    const started = Date.now();
    const cpuStart = process.cpuUsage();
    const memStart = process.memoryUsage().rss;
    this.stats.requests += 1;
    const left = document.lineAt(position.line).text.slice(0, position.character);
    const query = tokenMap(left);
    const top = this.resolveTopChunks(left, query);
    const relatedImports = extractImportTargets(document).slice(0, 5).join(", ");
    const diagHints = vscode.languages.getDiagnostics(document.uri)
      .filter((d) => d.range.start.line <= position.line && d.range.end.line >= position.line)
      .slice(0, 3)
      .map((d) => `${d.code ?? "diag"}:${d.message}`)
      .join(" | ");
    const context = [
      `FILE: ${document.uri.fsPath}`,
      `LEFT: ${left.slice(Math.max(0, left.length - 320))}`,
      `IMPORTS: ${relatedImports}`,
      `DIAGNOSTICS: ${diagHints}`,
      `BUILD_CFG: ${this.workspaceContext.buildConfig.slice(0, 500)}`,
      `TEST_CFG: ${this.workspaceContext.testConfig.slice(0, 500)}`,
      `EXTERNAL_DEPS: ${this.workspaceContext.externalDeps.slice(0, 20).join(", ")}`,
      `GIT_RECENT: ${this.workspaceContext.gitRecent.slice(0, 20).join(", ")}`,
      "CHUNKS:",
      ...top.map((c) => `--- ${c.uri}\n${c.text.slice(0, 700)}`),
    ].join("\n");
    const safeContext = this.opts.redactSecrets ? redactSensitive(context) : context;

    let backendCandidates: string[] = [];
    if (this.opts.backendEnabled && this.opts.backendUrl.trim().length > 0 && this.canUseBackend()) {
      this.stats.backendRequests += 1;
      backendCandidates = await this.fetchBackendCandidates(safeContext, left);
    }

    const merged = this.rankAndFilter(localCandidates, backendCandidates, left);
    const cpuDelta = process.cpuUsage(cpuStart);
    const memEnd = process.memoryUsage().rss;
    this.pushLatency(Date.now() - started);
    this.pushCpuAndMemory(cpuDelta.user + cpuDelta.system, Math.max(0, memEnd - memStart));
    return merged.slice(0, Math.max(1, this.opts.maxCandidates));
  }

  noteAccepted(): void {
    this.stats.acceptedLikeCount += 1;
  }

  getTelemetry(): AiInlineTelemetry {
    return {
      requests: this.stats.requests,
      backendRequests: this.stats.backendRequests,
      backendFailures: this.stats.backendFailures,
      backendTimeouts: this.stats.backendTimeouts,
      fallbackLocalCount: this.stats.fallbackLocalCount,
      acceptedLikeCount: this.stats.acceptedLikeCount,
      avgLatencyMs: average(this.stats.latencies),
      p95LatencyMs: percentile(this.stats.latencies, 0.95),
      chunks: this.chunks.length,
      files: this.byUri.size,
      avgCpuMicros: average(this.statsCpuMicros),
      p95CpuMicros: percentile(this.statsCpuMicros, 0.95),
      avgMemDeltaKb: average(this.statsMemDeltaKb),
      p95MemDeltaKb: percentile(this.statsMemDeltaKb, 0.95),
    };
  }

  private async fetchBackendCandidates(context: string, left: string): Promise<string[]> {
    const ctrl = new AbortController();
    const t = setTimeout(() => ctrl.abort(), Math.max(100, this.opts.backendTimeoutMs));
    try {
      const fetchFn = (globalThis as unknown as {
        fetch?: (input: string, init?: {
          method?: string;
          headers?: Record<string, string>;
          body?: string;
          signal?: AbortSignal;
        }) => Promise<{ ok: boolean; json(): Promise<unknown> }>;
      }).fetch;
      if (!fetchFn) {
        this.stats.backendFailures += 1;
        return [];
      }
      const r = await fetchFn(this.opts.backendUrl, {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({
          task: "inline_completion",
          language: "vitte",
          left,
          context,
          maxCandidates: this.opts.maxCandidates,
          policy: {
            allowExternalTraining: this.opts.allowExternalTraining,
            retentionDays: this.opts.dataRetentionDays,
          },
        }),
        signal: ctrl.signal,
      });
      if (!r.ok) {
        this.stats.backendFailures += 1;
        return [];
      }
      const data = await r.json() as { candidates?: string[] };
      return Array.isArray(data.candidates) ? data.candidates.filter((v) => typeof v === "string") : [];
    } catch (err) {
      if (String(err).includes("AbortError")) this.stats.backendTimeouts += 1;
      else this.stats.backendFailures += 1;
      return [];
    } finally {
      clearTimeout(t);
    }
  }

  private rankAndFilter(localCandidates: string[], backendCandidates: string[], left: string): string[] {
    const out: Candidate[] = [];
    for (const c of localCandidates) out.push({ text: c, score: 50, source: "local" });
    for (const c of backendCandidates) out.push({ text: c, score: 46, source: "backend" });
    const style = detectStyleWeight(this.opts.styleMode, this.opts.userMode);
    const seen = new Set<string>();
    const ranked: Candidate[] = [];
    for (const c of out) {
      const clean = postProcessCandidate(c.text, left);
      if (!clean || clean.trim().length === 0) continue;
      if (isNoisyCandidate(clean)) continue;
      const key = clean.trim();
      if (seen.has(key)) continue;
      seen.add(key);
      const s = c.score
        + relevanceScore(clean, left) * 6
        + (c.source === "local" ? 5 : 0)
        + style;
      ranked.push({ ...c, text: clean, score: s });
    }
    if (this.opts.antiHallucination && !this.opts.allowUnknownSymbols) {
      for (let i = ranked.length - 1; i >= 0; i -= 1) {
        if (hasLikelyUnknownIdentifier(ranked[i]!.text, this.symbols)) ranked.splice(i, 1);
      }
    }
    ranked.sort((a, b) => b.score - a.score);
    if (backendCandidates.length === 0) this.stats.fallbackLocalCount += 1;
    return ranked.map((r) => r.text);
  }

  private rebuildSymbols(): void {
    this.symbols.clear();
    for (const doc of this.byUri.values()) {
      for (const s of doc.symbols) this.symbols.add(s);
    }
  }

  private pushLatency(ms: number): void {
    this.stats.latencies.push(Math.max(0, ms));
    if (this.stats.latencies.length > 2048) this.stats.latencies.shift();
  }

  private canUseBackend(): boolean {
    if (!this.opts.cloudOptIn) return false;
    if (this.opts.localOnly) return false;
    if (this.opts.privacyStrict && this.opts.backendAllowlist.length === 0) return false;
    if (this.opts.trustedWorkspaceOnly && !vscode.workspace.isTrusted) return false;
    const url = this.opts.backendUrl.trim();
    if (!url) return false;
    if (!this.opts.privacyStrict) return true;
    try {
      const host = new URL(url).hostname.toLowerCase();
      return this.opts.backendAllowlist.some((entry) => host === entry || host.endsWith(`.${entry}`));
    } catch {
      return false;
    }
  }
}

function detectStyleWeight(styleMode: "project" | "user", userMode: "aggressive" | "balanced" | "conservative"): number {
  if (styleMode === "project") return 2;
  if (userMode === "aggressive") return 4;
  if (userMode === "conservative") return -2;
  return 0;
}

function postProcessCandidate(text: string, left: string): string {
  let out = text.replace(/\r/g, "");
  const oParen = (left.match(/\(/g) ?? []).length;
  const cParen = (left.match(/\)/g) ?? []).length;
  if (oParen > cParen && !out.includes(")")) out += ")";
  const oBrace = (left.match(/\{/g) ?? []).length;
  const cBrace = (left.match(/\}/g) ?? []).length;
  if (oBrace > cBrace && !out.includes("}")) out += "\n}";
  return out;
}

function hasLikelyUnknownIdentifier(text: string, symbols: Set<string>): boolean {
  const ids = text.match(/[A-Za-z_][A-Za-z0-9_]{2,}/g) ?? [];
  for (const id of ids) {
    if (KEYWORDS.has(id)) continue;
    if (!symbols.has(id)) return true;
  }
  return false;
}

function isNoisyCandidate(text: string): boolean {
  const t = text.trim();
  if (t.length === 0) return true;
  if (/^(.)\1{5,}$/.test(t)) return true;
  if (t === "{}" || t === "()" || t === "[]") return true;
  return false;
}

function relevanceScore(candidate: string, left: string): number {
  const p = (left.match(/[A-Za-z_][A-Za-z0-9_]*$/)?.[0] ?? "").toLowerCase();
  if (!p) return 0.2;
  const c = candidate.toLowerCase();
  if (c.startsWith(p)) return 1.2;
  if (c.includes(p)) return 0.7;
  return 0;
}

function extractSymbols(text: string): Set<string> {
  const out = new Set<string>();
  for (const m of text.matchAll(/\b(?:let|const|fn|proc|test|type|enum|struct)\s+([A-Za-z_][A-Za-z0-9_]*)/g)) {
    if (m[1]) out.add(m[1]);
  }
  for (const m of text.matchAll(/\b([A-Z][A-Za-z0-9_]*)::([A-Z][A-Za-z0-9_]*)/g)) {
    if (m[1]) out.add(m[1]);
    if (m[2]) out.add(m[2]);
  }
  return out;
}

function chunkText(uri: string, text: string, linesPerChunk: number): Chunk[] {
  const lines = text.split(/\r?\n/);
  const out: Chunk[] = [];
  for (let i = 0; i < lines.length; i += linesPerChunk) {
    const slice = lines.slice(i, i + linesPerChunk).join("\n").trim();
    if (!slice) continue;
    out.push({
      id: `${uri}#${i}`,
      uri,
      text: slice,
      tokens: tokenMap(slice),
    });
  }
  return out;
}

function tokenMap(text: string): Map<string, number> {
  const m = new Map<string, number>();
  const toks = text.toLowerCase().match(/[a-z_][a-z0-9_]+/g) ?? [];
  for (const t of toks) m.set(t, (m.get(t) ?? 0) + 1);
  return m;
}

function cosine(a: Map<string, number>, b: Map<string, number>): number {
  let dot = 0;
  let na = 0;
  let nb = 0;
  for (const [, av] of a) na += av * av;
  for (const [, bv] of b) nb += bv * bv;
  for (const [k, av] of a) {
    const bv = b.get(k);
    if (bv) dot += av * bv;
  }
  const d = Math.sqrt(na) * Math.sqrt(nb);
  if (!d) return 0;
  return dot / d;
}

function topKChunks(query: Map<string, number>, chunks: Chunk[], k: number): Chunk[] {
  return chunks
    .map((c) => ({ c, s: cosine(query, c.tokens) }))
    .sort((a, b) => b.s - a.s)
    .slice(0, Math.max(1, k))
    .map((x) => x.c);
}

function extractImportTargets(document: vscode.TextDocument): string[] {
  const out: string[] = [];
  const maxLines = Math.min(document.lineCount, 300);
  for (let i = 0; i < maxLines; i += 1) {
    const t = document.lineAt(i).text.trim();
    const m = /^(?:import|use|pull)\s+([A-Za-z_][\w./:]*)/.exec(t);
    if (m?.[1]) out.push(m[1]);
  }
  return out;
}

function collectExternalDeps(byUri: Map<string, FileDoc>): string[] {
  const out = new Set<string>();
  for (const doc of byUri.values()) {
    for (const ch of doc.chunks) {
      for (const m of ch.text.matchAll(/^(?:import|use|pull)\s+([A-Za-z_][\w./:-]*)/gm)) {
        const target = m[1] ?? "";
        if (!target) continue;
        if (target.includes("/") || target.includes(":")) out.add(target);
      }
    }
  }
  return [...out].sort((a, b) => a.localeCompare(b)).slice(0, 200);
}

async function readSmallFileIfExists(file: string): Promise<string> {
  try {
    const stat = await fs.promises.stat(file);
    if (!stat.isFile() || stat.size > 256 * 1024) return "";
    return await fs.promises.readFile(file, "utf8");
  } catch {
    return "";
  }
}

function readGitRecentFiles(root: string): string[] {
  try {
    const status = cp.execFileSync("git", ["status", "--porcelain"], { cwd: root, encoding: "utf8" });
    const changed = status.split(/\r?\n/).map((l) => l.trim()).filter(Boolean).map((l) => l.slice(3).trim());
    if (changed.length > 0) return changed.slice(0, 80);
    const recent = cp.execFileSync("git", ["log", "--name-only", "--pretty=format:", "-n", "20"], { cwd: root, encoding: "utf8" });
    return recent.split(/\r?\n/).map((l) => l.trim()).filter(Boolean).slice(0, 80);
  } catch {
    return [];
  }
}

function redactSensitive(input: string): string {
  let out = input;
  out = out.replace(/\b(AKIA[0-9A-Z]{16})\b/g, "<REDACTED_AWS_KEY>");
  out = out.replace(/\b(sk-[A-Za-z0-9]{16,})\b/g, "<REDACTED_API_KEY>");
  out = out.replace(/\b(ghp_[A-Za-z0-9]{20,})\b/g, "<REDACTED_GH_TOKEN>");
  out = out.replace(/\b(password|passwd|secret|token)\s*[:=]\s*([^\s]+)/gi, "$1=<REDACTED>");
  out = out.replace(/^\s*[A-Z0-9_]{2,}\s*=\s*.+$/gm, "<REDACTED_ENV_LINE>");
  out = out.replace(/\b(xox[baprs]-[A-Za-z0-9-]{10,})\b/g, "<REDACTED_SLACK_TOKEN>");
  out = out.replace(/\b(eyJ[A-Za-z0-9_-]{10,}\.[A-Za-z0-9._-]{10,}\.[A-Za-z0-9._-]{10,})\b/g, "<REDACTED_JWT>");
  out = out.replace(/-----BEGIN [A-Z ]+-----[\s\S]*?-----END [A-Z ]+-----/g, "<REDACTED_PEM_BLOCK>");
  return out;
}

function digest(text: string): string {
  return crypto.createHash("sha1").update(text).digest("hex");
}

function average(xs: number[]): number {
  if (xs.length === 0) return 0;
  return Math.round(xs.reduce((a, b) => a + b, 0) / xs.length);
}

function percentile(xs: number[], q: number): number {
  if (xs.length === 0) return 0;
  const s = [...xs].sort((a, b) => a - b);
  const i = Math.max(0, Math.min(s.length - 1, Math.round((s.length - 1) * q)));
  return s[i] ?? 0;
}

const KEYWORDS = new Set([
  "if", "else", "for", "while", "match", "let", "const", "return", "break", "continue", "fn", "proc", "test", "import", "export", "type", "enum", "struct",
]);
