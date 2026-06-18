import { parentPort } from "node:worker_threads";

interface WorkerRequest {
  id: string;
  uri: string;
  text: string;
  linesPerChunk: number;
}

interface WorkerChunk {
  id: string;
  uri: string;
  text: string;
  tokens: Array<[string, number]>;
}

interface WorkerResponse {
  id: string;
  uri: string;
  chunks: WorkerChunk[];
  symbols: string[];
}

function tokenMap(text: string): Map<string, number> {
  const m = new Map<string, number>();
  const toks = text.toLowerCase().match(/[a-z_][a-z0-9_]+/g) ?? [];
  for (const t of toks) m.set(t, (m.get(t) ?? 0) + 1);
  return m;
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

function chunkText(uri: string, text: string, linesPerChunk: number): WorkerChunk[] {
  const lines = text.split(/\r?\n/);
  const out: WorkerChunk[] = [];
  for (let i = 0; i < lines.length; i += linesPerChunk) {
    const slice = lines.slice(i, i + linesPerChunk).join("\n").trim();
    if (!slice) continue;
    out.push({
      id: `${uri}#${i}`,
      uri,
      text: slice,
      tokens: Array.from(tokenMap(slice).entries()),
    });
  }
  return out;
}

parentPort?.on("message", (req: WorkerRequest) => {
  const chunks = chunkText(req.uri, req.text, req.linesPerChunk);
  const symbols = [...extractSymbols(req.text)];
  const res: WorkerResponse = { id: req.id, uri: req.uri, chunks, symbols };
  parentPort?.postMessage(res);
});

