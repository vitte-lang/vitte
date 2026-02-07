export interface ServerMetricEntry {
  name: string;
  count: number;
  averageMs: number;
  maxMs: number;
  lastMs: number;
  lastAt: number;
  lastUri: string;
  lastCount: number | null;
}
