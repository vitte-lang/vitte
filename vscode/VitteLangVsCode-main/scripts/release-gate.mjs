#!/usr/bin/env node
import { spawn } from "node:child_process";

const npmCli = process.env.npm_execpath;
if (process.platform === "win32" && !npmCli) {
  throw new Error("Run this gate as `npm run ci:release-gate` on Windows so npm can be started without a shell.");
}
const npmCommand = process.platform === "win32" ? process.execPath : "npm";
const npmPrefix = process.platform === "win32" ? [npmCli] : [];

function runStep(name, args) {
  return new Promise((resolve, reject) => {
    process.stdout.write(`\n[release-gate] ${name}\n`);
    const child = spawn(npmCommand, [...npmPrefix, ...args], {
      stdio: "inherit",
      shell: false,
    });
    child.on("error", reject);
    child.on("close", (code) => {
      if (code === 0) resolve();
      else reject(new Error(`${name} failed (${code})`));
    });
  });
}

const steps = [
  { name: "lint", args: ["run", "lint"] },
  { name: "test", args: ["run", "test"] },
  { name: "perf gate", args: ["run", "test:perf-gate"] },
  { name: "package vsix", args: ["run", "vsix"] },
  { name: "verify vsix file list", args: ["run", "vsix:verify"] },
  { name: "verify vsix contents", args: ["run", "vsix:verify-artifact"] },
  { name: "smoke installed vsix", args: ["run", "vsix:smoke"] },
];

for (const step of steps) {
  await runStep(step.name, step.args);
}
process.stdout.write("\n[release-gate] OK\n");
