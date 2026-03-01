"use strict";
/**
 * index.ts — configuration Mocha pour les tests d’intégration VS Code.
 */
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.run = run;
const path = __importStar(require("node:path"));
const mocha_1 = __importDefault(require("mocha"));
const node_fs_1 = require("node:fs");
async function collectTestFiles(root) {
    const entries = await node_fs_1.promises.readdir(root, { withFileTypes: true });
    const files = [];
    for (const entry of entries) {
        const fullPath = path.resolve(root, entry.name);
        if (entry.isDirectory()) {
            const nested = await collectTestFiles(fullPath);
            files.push(...nested);
        }
        else if (entry.isFile() && entry.name.endsWith(".test.js")) {
            files.push(fullPath);
        }
    }
    return files;
}
async function run() {
    const mocha = new mocha_1.default({
        ui: "tdd",
        color: true,
        timeout: 60000,
    });
    const testsRoot = path.resolve(__dirname);
    const files = await collectTestFiles(testsRoot);
    if (files.length === 0) {
        console.warn("[tests] Aucun fichier *.test.js trouvé dans", testsRoot);
    }
    for (const file of files) {
        mocha.addFile(file);
    }
    await new Promise((resolve, reject) => {
        mocha.run((failures) => {
            if (failures > 0) {
                reject(new Error(`${failures} test(s) ont échoué.`));
            }
            else {
                resolve();
            }
        });
    });
}
//# sourceMappingURL=index.js.map