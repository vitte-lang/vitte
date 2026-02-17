import * as vscode from "vscode";
import { DOC_SECTIONS, makeDocUrl } from "./documentationChapters";

interface DocumentationViewOptions {
  title: string;
  url: string;
}

class DocumentationViewProvider implements vscode.WebviewViewProvider {
  constructor(
    private readonly options: DocumentationViewOptions
  ) {}

  resolveWebviewView(webviewView: vscode.WebviewView): void {
    webviewView.webview.options = {
      enableScripts: true,
    };
    webviewView.webview.html = this.getHtml(webviewView.webview);
    webviewView.webview.onDidReceiveMessage((message) => {
      if (!message || typeof message !== "object") return;
      const type = (message as { type?: string }).type;
      if (type !== "openExternal") return;
      void vscode.env.openExternal(vscode.Uri.parse(this.options.url));
    });
  }

  private getHtml(webview: vscode.Webview): string {
    const nonce = String(Date.now());
    const freshUrl = withCacheBuster(this.options.url);
    const csp = [
      "default-src 'none'",
      "style-src 'unsafe-inline'",
      `script-src 'nonce-${nonce}'`,
      "img-src https: data:",
      `frame-src ${freshUrl} https://vitte.netlify.app`,
      `connect-src ${freshUrl} https://vitte.netlify.app`
    ].join("; ");
    const safeTitle = escapeHtml(this.options.title);
    const safeUrl = escapeHtml(freshUrl);
    const chapters = DOC_SECTIONS
      .map((section) => `<optgroup label="${escapeHtml(section.title)}">${
        section.chapters.map((chapter) => {
          const url = makeDocUrl(chapter.path);
          return `<option value="${escapeHtml(url)}">${escapeHtml(chapter.title)}</option>`;
        }).join("")
      }</optgroup>`)
      .join("");
    return `<!doctype html>
<html lang="fr">
<head>
  <meta charset="UTF-8" />
  <meta http-equiv="Content-Security-Policy" content="${csp}" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <style>
    :root { color-scheme: light dark; }
    body { margin: 0; font-family: var(--vscode-font-family); background: var(--vscode-editor-background); color: var(--vscode-editor-foreground); }
    .bar { display: flex; align-items: center; gap: 8px; padding: 8px; border-bottom: 1px solid var(--vscode-panel-border); flex-wrap: wrap; }
    .btn { cursor: pointer; border: 1px solid var(--vscode-button-border, transparent); background: var(--vscode-button-background); color: var(--vscode-button-foreground); padding: 4px 8px; border-radius: 4px; }
    .label { font-size: 12px; opacity: .85; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    iframe { width: 100%; height: calc(100vh - 42px); border: 0; }
  </style>
</head>
<body>
  <div class="bar">
    <button class="btn" id="openExternal" type="button">Open in browser</button>
    <select id="chapters" class="btn" aria-label="Sommaire">${chapters}</select>
    <button class="btn" id="goChapter" type="button">Open chapter</button>
    <div class="label">${safeTitle} · ${safeUrl}</div>
  </div>
  <iframe id="docsFrame" src="${safeUrl}" title="${safeTitle}" loading="lazy" referrerpolicy="no-referrer"></iframe>
  <script nonce="${nonce}">
    const vscode = acquireVsCodeApi();
    const frame = document.getElementById("docsFrame");
    const chapters = document.getElementById("chapters");
    const openChapter = () => {
      if (!frame || !chapters || !chapters.value) return;
      frame.src = chapters.value;
    };
    document.getElementById("openExternal")?.addEventListener("click", () => {
      vscode.postMessage({ type: "openExternal" });
    });
    document.getElementById("goChapter")?.addEventListener("click", openChapter);
    chapters?.addEventListener("change", openChapter);
  </script>
</body>
</html>`;
  }
}

function escapeHtml(value: string): string {
  return value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

function withCacheBuster(url: string): string {
  try {
    const parsed = new URL(url);
    parsed.searchParams.set("from", "vscode");
    parsed.searchParams.set("ts", String(Date.now()));
    return parsed.toString();
  } catch {
    return url;
  }
}

export function registerDocumentationView(
  context: vscode.ExtensionContext,
  viewId: string,
  title: string,
  url = "https://vitte.netlify.app"
): void {
  const provider = new DocumentationViewProvider({ title, url });
  context.subscriptions.push(
    vscode.window.registerWebviewViewProvider(viewId, provider, { webviewOptions: { retainContextWhenHidden: true } })
  );
}
