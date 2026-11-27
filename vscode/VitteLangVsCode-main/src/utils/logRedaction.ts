/** Redact common credentials before writing subprocess or protocol text to logs. */
export function redactLogText(value: unknown): string {
  return String(value)
    .replace(/(https?:\/\/)[^/@\s]+:[^/@\s]+@/gi, "$1[redacted]@")
    .replace(/((?:authorization|proxy-authorization)\s*[:=]\s*)(?:(?:Bearer|Basic)\s+)?[^\s,;&]+/gi, "$1[redacted]")
    .replace(/\bBearer\s+[A-Za-z0-9._~+/-]+=*/gi, "Bearer [redacted]")
    .replace(/((?:access[_-]?token|refresh[_-]?token|authorization|token|password|passwd|secret|api[_-]?key|client[_-]?secret)\s*[:=]\s*)(?:"[^"]*"|'[^']*'|[^\s,;&]+)/gi, "$1[redacted]");
}
