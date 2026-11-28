import * as fs from "node:fs/promises";
import * as path from "node:path";

/** Write unsaved source beside its original file so project-relative imports still resolve. */
export async function writeCompilerSourceSnapshot(file: string, text: string): Promise<string> {
  const extension = path.extname(file) || ".vit";
  const stem = path.basename(file, extension);
  for (let attempt = 0; attempt < 4; attempt += 1) {
    const candidate = path.join(
      path.dirname(file),
      `.${stem}.vitte-diagnostics-${process.pid}-${Date.now()}-${Math.random().toString(36).slice(2)}${extension}`,
    );
    try {
      await fs.writeFile(candidate, text, { encoding: "utf8", flag: "wx" });
      return candidate;
    } catch (error) {
      if ((error as NodeJS.ErrnoException).code !== "EEXIST") throw error;
    }
  }
  throw new Error("could not allocate a temporary source snapshot");
}
