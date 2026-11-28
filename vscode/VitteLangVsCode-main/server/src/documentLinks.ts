import { DocumentLink, Range } from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
import * as path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { maskNonCode } from "./lexical.js";
import { resolveModuleFiles, type CompletionEnvironment } from "./moduleResolver.js";

export async function provideDocumentLinks(doc: TextDocument, environment: CompletionEnvironment): Promise<DocumentLink[]> {
  const text = doc.getText();
  const code = maskNonCode(text);
  const roots = [...(environment.workspaceRoots ?? []), environment.projectRoot, ...(environment.modulePaths ?? [])]
    .filter((root): root is string => Boolean(root))
    .map((root) => path.resolve(root));
  const links: DocumentLink[] = [];
  const imports = /\buse\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)/g;
  for (const match of code.matchAll(imports)) {
    const modulePath = match[1];
    if (!modulePath || match.index === undefined) continue;
    const start = match.index + match[0].indexOf(modulePath);
    const candidates = await resolveModuleFiles(modulePath, doc.uri, environment);
    const local = candidates.filter((candidate) => {
      if (!candidate.startsWith("file:")) return roots.some((root) => isWithin(root, candidate));
      try { return roots.some((root) => isWithin(root, fileURLToPath(candidate))); }
      catch { return false; }
    });
    const unique = [...new Set(local)];
    if (unique.length !== 1) continue;
    const target = unique[0];
    if (!target) continue;
    const targetUri = target.startsWith("file:") ? target : pathToFileURL(target).toString();
    const link = DocumentLink.create(
      Range.create(doc.positionAt(start), doc.positionAt(start + modulePath.length)),
      targetUri,
    );
    link.tooltip = `Open module ${modulePath}`;
    links.push(link);
  }
  return links;
}

function isWithin(root: string, candidate: string): boolean {
  const relative = path.relative(root, path.resolve(candidate));
  return relative === "" || (!relative.startsWith(`..${path.sep}`) && relative !== ".." && !path.isAbsolute(relative));
}
