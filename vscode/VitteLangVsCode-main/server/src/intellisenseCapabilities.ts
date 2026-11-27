import matrix from "./capability-matrix.json";

type CapabilityStatus = "implemented" | "partial" | "planned" | "unavailable";
type EditorFeature = (typeof matrix.editorFeatures)[number];

export interface VitteCapabilityMatrix {
  schema: string;
  matrixVersion: string;
  updated: string;
  versions: typeof matrix.versions;
  advertisingRule: string;
  statusDefinitions: typeof matrix.statusDefinitions;
  editorFeatures: (EditorFeature & { enabled?: boolean })[];
  compilerFrontends: typeof matrix.compilerFrontends;
  cliCommands: typeof matrix.cliCommands;
  platforms: typeof matrix.platforms;
  followUps: typeof matrix.followUps;
}

const matrixFeatures = new Map(matrix.editorFeatures.map((feature) => [feature.id, feature]));

/** Only tested features may be included in LSP initialize capabilities. */
export function isCapabilityAdvertised(id: string): boolean {
  const feature = matrixFeatures.get(id);
  return Boolean(
    feature?.advertised &&
    feature.status !== "planned" &&
    feature.status !== "unavailable" &&
    feature.evidence.length > 0,
  );
}

export function getIntelliSenseCapabilities(
  enabledFeatures: Partial<Record<string, boolean>> = {},
): VitteCapabilityMatrix {
  return {
    ...matrix,
    editorFeatures: matrix.editorFeatures.map((feature) => ({
      ...feature,
      advertised: isCapabilityAdvertised(feature.id),
      enabled: feature.setting
        ? isCapabilityAdvertised(feature.id) && (enabledFeatures[feature.setting] ?? true)
        : undefined,
    })),
  };
}

export function capabilityStatusLabel(status: CapabilityStatus): string {
  switch (status) {
    case "implemented": return "Implemented and tested";
    case "partial": return "Partial; see scope";
    case "planned": return "Planned; not advertised";
    case "unavailable": return "Unavailable in compiler 0.1.1";
  }
}
