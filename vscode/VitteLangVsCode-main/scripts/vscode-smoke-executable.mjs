import { promises as fs } from 'node:fs';
import path from 'node:path';

export async function resolveSmokeExecutable(executable, platform = process.platform) {
  try {
    await fs.access(executable);
    return executable;
  } catch (error) {
    if (platform !== 'darwin' || error?.code !== 'ENOENT') throw error;
  }

  const currentExecutable = path.join(path.dirname(executable), 'Code');
  await fs.access(currentExecutable);
  return currentExecutable;
}
