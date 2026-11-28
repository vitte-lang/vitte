import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import { fileURLToPath } from "node:url";

export interface VitteProjectScope {
  workspaceRoot: string;
  projectRoot: string;
  configFile?: string;
}

export function resolveProjectPath(value: string, base: string): string {
  const expanded = value
    .replace(/\$\{workspaceFolder\}/g, base)
    .replace(/\$\{env:([^}]+)\}/g, (_match, key: string) => process.env[key] ?? '')
    .replace(/^~(?=$|[\\/])/, os.homedir());
  return path.resolve(base, expanded);
}

/** Select the longest matching workspace folder and its nearest CLI project file. */
export function resolveVitteProjectScope(resourceUri: string, workspaceRoots: readonly string[], configuredRoot?: string): VitteProjectScope {
  let resourcePath: string | undefined;
  try { if (resourceUri.startsWith("file:")) resourcePath = fileURLToPath(resourceUri); } catch { /* use the workspace fallback */ }
  const matching = resourcePath
    ? workspaceRoots.filter((root) => {
      const relative = path.relative(path.resolve(root), path.resolve(resourcePath!));
      return relative === "" || (relative !== ".." && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative));
    }).sort((a, b) => b.length - a.length)
    : [];
  const workspaceRoot = path.resolve(matching[0] ?? workspaceRoots[0] ?? (resourcePath ? path.dirname(resourcePath) : process.cwd()));
  if (configuredRoot?.trim()) {
    const projectRoot = resolveProjectPath(configuredRoot.trim(), workspaceRoot);
    const detected = findCliConfig(projectRoot);
    return { workspaceRoot, projectRoot, ...(detected ? { configFile: detected.configFile } : {}) };
  }
  const detected = findCliConfig(resourcePath ?? workspaceRoot);
  return { workspaceRoot, projectRoot: detected?.root ?? workspaceRoot, ...(detected ? { configFile: detected.configFile } : {}) };
}

function findCliConfig(startPath: string): { root: string; configFile: string } | undefined {
  let current = path.resolve(startPath);
  try { if (!fs.statSync(current).isDirectory()) current = path.dirname(current); }
  catch { current = path.extname(current) ? path.dirname(current) : current; }
  while (true) {
    for (const relative of ["vitte.toml", path.join(".vitte", "config")]) {
      const configFile = path.join(current, relative);
      try { if (fs.statSync(configFile).isFile()) return { root: current, configFile }; }
      catch { /* only the CLI-supported project files are markers */ }
    }
    const parent = path.dirname(current);
    if (parent === current) return undefined;
    current = parent;
  }
}
