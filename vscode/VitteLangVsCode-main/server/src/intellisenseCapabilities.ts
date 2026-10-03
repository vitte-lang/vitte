/**
 * Public capability contract for the Vitte IntelliSense surface.
 *
 * This deliberately describes editor capabilities separately from the native
 * compiler/DAP slice. The Microsoft C/C++ extension exposes both through one
 * product; Vitte exposes the same contract and keeps external DAP fallback.
 */

export interface VitteIntelliSenseCapabilities {
  schemaVersion: 1;
  provider: "vitte-lsp";
  paritySurface: "microsoft-cpp-intellisense";
  lsp: {
    completion: true;
    signatureHelp: true;
    hover: true;
    diagnostics: true;
    documentSymbols: true;
    workspaceSymbols: true;
    definition: true;
    references: true;
    rename: true;
    codeActions: true;
    formatting: true;
    semanticTokens: true;
    inlayHints: true;
    callHierarchy: true;
    typeHierarchy: true;
  };
  debugging: {
    dap: "native-process" | "external-adapter";
    nativeBackend: true;
    compilerDebugInfo: true;
  };
}

export function getIntelliSenseCapabilities(): VitteIntelliSenseCapabilities {
  return {
    schemaVersion: 1,
    provider: "vitte-lsp",
    paritySurface: "microsoft-cpp-intellisense",
    lsp: {
      completion: true,
      signatureHelp: true,
      hover: true,
      diagnostics: true,
      documentSymbols: true,
      workspaceSymbols: true,
      definition: true,
      references: true,
      rename: true,
      codeActions: true,
      formatting: true,
      semanticTokens: true,
      inlayHints: true,
      callHierarchy: true,
      typeHierarchy: true,
    },
    debugging: {
      dap: "native-process",
      nativeBackend: true,
      compilerDebugInfo: true,
    },
  };
}
