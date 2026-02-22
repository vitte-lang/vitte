#!/usr/bin/env node
"use strict";

const { spawnSync } = require("node:child_process");
const { resolve } = require("node:path");

const root = resolve(__dirname, "..", "..");
const compiler = resolve(root, "bin", "vitte");
const source = resolve(root, "vitteos", "scripts", "vitteos_tooling.vit");

const result = spawnSync(compiler, ["check", source], { stdio: "inherit" });
if (result.error) {
  console.error(`[run_check.js] failed to run compiler: ${result.error.message}`);
  process.exit(1);
}

process.exit(result.status ?? 1);
