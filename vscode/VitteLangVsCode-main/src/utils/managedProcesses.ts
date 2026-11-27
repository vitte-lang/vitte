import { spawn as nodeSpawn } from "node:child_process";
import type { ChildProcess, SpawnOptions } from "node:child_process";

const children = new Set<ChildProcess>();

/** Spawn an extension-owned helper and track it for reload/deactivation cleanup. */
export function spawnManaged(command: string, args: readonly string[] = [], options: SpawnOptions = {}): ChildProcess {
  const child = nodeSpawn(command, [...args], options);
  children.add(child);
  const forget = (): void => { children.delete(child); };
  child.once("exit", forget);
  child.once("error", forget);
  return child;
}

export async function stopManagedProcesses(graceMs = 1000): Promise<void> {
  const running = [...children].filter((child) => child.exitCode === null && child.signalCode === null);
  if (running.length === 0) return;

  for (const child of running) {
    try { child.kill(); } catch { /* process may already be exiting */ }
  }

  await Promise.race([
    Promise.all(running.map(waitForExit)),
    new Promise<void>((resolve) => setTimeout(resolve, graceMs)),
  ]);

  for (const child of running) {
    if (child.exitCode !== null || child.signalCode !== null) continue;
    try { child.kill("SIGKILL"); } catch { /* process may already be exiting */ }
  }
  children.clear();
}

function waitForExit(child: ChildProcess): Promise<void> {
  if (child.exitCode !== null || child.signalCode !== null) return Promise.resolve();
  return new Promise((resolve) => child.once("exit", () => resolve()));
}
