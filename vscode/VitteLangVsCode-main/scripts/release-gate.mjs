#!/usr/bin/env node
import { spawn } from "node:child_process";

function runStep(name, cmd, args) {
  return new Promise((resolve, reject) => {
    process.stdout.write(`\n[release-gate] ${name}\n`);
    const child = spawn(cmd, args, {
      stdio: "inherit",
      shell: process.platform === "win32",
    });
    child.on("error", reject);
    child.on("close", (code) => {
      if (code === 0) resolve();
      else reject(new Error(`${name} failed (${code})`));
    });
  });
}

const steps = [
  { name: "lint", cmd: "npm", args: ["run", "lint"] },
  { name: "test", cmd: "npm", args: ["run", "test"] },
  { name: "perf gate", cmd: "npm", args: ["run", "test:perf-gate"] },
  { name: "package vsix", cmd: "npm", args: ["run", "vsix"] },
  { name: "verify vsix", cmd: "npm", args: ["run", "vsix:verify"] },
  { name: "smoke vsix", cmd: "npx", args: ["@vscode/vsce@3.7.1", "ls", "vitte-studio.vsix"] },
];

for (const step of steps) {
  await runStep(step.name, step.cmd, step.args);
}
process.stdout.write("\n[release-gate] OK\n");
