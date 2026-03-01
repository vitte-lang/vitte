import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as net from 'net';
import * as path from 'path';

/**
 * Vitte Debug Adapter factory.
 * Spawns `vitte-runtime` in DAP server mode when possible.
 * Falls back to stdio if server mode is not available.
 */
export class VitteDebugAdapterDescriptorFactory implements vscode.DebugAdapterDescriptorFactory, vscode.Disposable {
  private _subscriptions: vscode.Disposable[] = [];
  private _sessions = new Map<string, { proc: cp.ChildProcess; port?: number }>();

  private toVscodeEnv(env: NodeJS.ProcessEnv | undefined): Record<string, string> | undefined {
    if (!env) return undefined;
    const out: Record<string, string> = {};
    for (const [key, value] of Object.entries(env)) {
      if (typeof value === 'string') out[key] = value;
    }
    return out;
  }

  private ensureString(x: string | undefined, fallback: string): string {
    return (typeof x === 'string' && x.length > 0) ? x : fallback;
  }

  private sanitizeArgs(args: unknown[]): string[] {
    return args.filter((a): a is string => typeof a === 'string' && a.length > 0);
  }

  dispose(): void {
    for (const d of this._subscriptions) d.dispose();
    for (const { proc } of this._sessions.values()) {
      try { proc.kill(); } catch { /* noop */ }
    }
    this._sessions.clear();
  }

  async createDebugAdapterDescriptor(session: vscode.DebugSession): Promise<vscode.DebugAdapterDescriptor> {
    const cfg = vscode.workspace.getConfiguration('vitte');
    const program = this.resolveRuntimePath(cfg);
    const rawArgs = Array.isArray(session.configuration.args) ? session.configuration.args : [];
    const extraArgs = this.sanitizeArgs(rawArgs);
    const cwd = typeof session.configuration.cwd === 'string' && session.configuration.cwd.length > 0
      ? session.configuration.cwd
      : (vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? process.cwd());

    // Try DAP server mode first.
    const serverTry = await this.tryStartServer(program, cwd, extraArgs);
    if (serverTry.ok) {
      const { proc, port } = serverTry;
      this._sessions.set(session.id, { proc, port });
      return new vscode.DebugAdapterServer(port, '127.0.0.1');
    }

    // Fallback to stdio executable.
    const exec = this.buildExecutable(program, cwd, extraArgs);
    const opts: vscode.DebugAdapterExecutableOptions = { cwd: exec.cwd };
    const envMap = this.toVscodeEnv(exec.env);
    if (envMap) { opts.env = envMap; }
    const desc = new vscode.DebugAdapterExecutable(exec.command, exec.args, opts);
    return desc;
  }

  private resolveRuntimePath(cfg: vscode.WorkspaceConfiguration): string {
    // Prefer project setting `vitte.debug.program`, then `vitte.runtime` or `vitte.build.path`, then default.
    const direct = cfg.get<string>('debug.program');
    if (direct?.trim().length) return direct;

    const toolchainRoot = cfg.get<string>('toolchainPath') ?? cfg.get<string>('toolchain.root');
    const candidate = cfg.get<string>('runtime.path') ?? cfg.get<string>('runtime') ?? 'vitte-runtime';

    if (toolchainRoot && !path.isAbsolute(candidate)) {
      return path.join(toolchainRoot, candidate);
    }
    return candidate;
  }

  private async tryStartServer(program: string, cwd: string, extraArgs: string[]): Promise<{ ok: true, proc: cp.ChildProcess, port: number } | { ok: false, error: Error } > {
    // Strategy A: runtime supports `dap --port 0` and prints the selected port.
    const argsA = ['dap', '--port', '0', ...extraArgs];
    const attemptA = await this.spawnAndDetectPort(program, argsA, cwd);
    if (attemptA.ok) return attemptA;

    // Strategy B: runtime supports `--dap --port 0`.
    const argsB = ['--dap', '--port', '0', ...extraArgs];
    const attemptB = await this.spawnAndDetectPort(program, argsB, cwd);
    if (attemptB.ok) return attemptB;

    // Strategy C: runtime supports `dap` with a fixed ephemeral port printed on stdout.
    const argsC = ['dap', ...extraArgs];
    const attemptC = await this.spawnAndDetectPort(program, argsC, cwd);
    if (attemptC.ok) return attemptC;

    return { ok: false, error: new Error('DAP server mode not detected on vitte-runtime.') };
  }

  private spawnAndDetectPort(program: string, args: string[], cwd: string): Promise<{ ok: true, proc: cp.ChildProcess, port: number } | { ok: false, error: Error }> {
    return new Promise((resolve) => {
      let resolved = false;
      const cmd = this.ensureString(program, 'vitte-runtime');
      const argv = this.sanitizeArgs(args);
      const proc = cp.spawn(cmd, argv, { cwd: this.ensureString(cwd, process.cwd()), env: process.env, stdio: ['ignore', 'pipe', 'pipe'] });

      const cleanup = (err?: Error) => {
        if (!resolved) {
          resolved = true;
          resolve({ ok: false, error: err ?? new Error('Failed to detect DAP port') });
        }
      };

      const onData = (data: Buffer) => {
        if (resolved) return;
        const text = data.toString();
        // Common patterns to detect an announced port
        // Examples:
        //  - DAP listening on 127.0.0.1:51234
        //  - debug-adapter: port=51234
        //  - DAP_PORT=51234
        const portMatch = /(?:listening\s+on\s+[^:]+:|port\s*=|DAP_PORT=)(\d{3,5})/i;
        const match = portMatch.exec(text);
        if (match) {
          const g = match[1];
          if (!g) return; // group may be undefined in TS types
          const port = Number.parseInt(g, 10);
          if (Number.isInteger(port) && port > 0 && port < 65536) {
            resolved = true;
            // Verify the port is accepting connections briefly
            const sock = new net.Socket();
            sock.once('error', () => {
              // Might not be ready yet, still resolve and let VSCode retry
              resolve({ ok: true, proc, port });
            });
            sock.connect(port, '127.0.0.1', () => {
              sock.destroy();
              resolve({ ok: true, proc, port });
            });
          }
        }
      };

      proc.stdout?.on('data', onData);
      proc.stderr?.on('data', onData);

      proc.once('error', (err) => cleanup(err));
      proc.once('exit', (code, signal) => {
        if (!resolved) {
          cleanup(new Error(`runtime exited before announcing port (code=${code}, signal=${signal ?? 'null'})`));
        }
      });

      // Safety timeout
      setTimeout(() => cleanup(new Error('timeout waiting for DAP port')), 4000);
    });
  }

  private buildExecutable(program: string, cwd: string, extraArgs: string[]) {
    // Fallback: stdio mode executed as a DebugAdapterExecutable
    // Try common flags to force stdio DAP if supported. Otherwise, leave args as-is.
    const stdioArgsCandidates: string[][] = [
      ['dap', '--stdio'],
      ['--dap', '--stdio'],
      []
    ];
    let args = extraArgs;
    if (args.length === 0) {
      args = stdioArgsCandidates.find(a => a.length > 0) ?? [];
    }

    return { command: program, args: this.sanitizeArgs(args), cwd, env: process.env };
  }
}

export function registerDebugFactory(ctx: vscode.ExtensionContext) {
  const factory = new VitteDebugAdapterDescriptorFactory();
  ctx.subscriptions.push(
    vscode.debug.registerDebugAdapterDescriptorFactory('vitte', factory),
    factory
  );
}
