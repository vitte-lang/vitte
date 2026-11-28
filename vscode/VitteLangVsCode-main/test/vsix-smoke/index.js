const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vscode = require("vscode");

exports.run = async function run() {
  const mocha = new (require("mocha"))({ ui: "tdd", timeout: 90000 });
  mocha.suite.emit("pre-require", global, "vsix-smoke.test.js", mocha);

  suite("Installed Vitte VSIX", () => {
    test("activates from an isolated profile outside the checkout", async () => {
      const extension = vscode.extensions.getExtension("VitteStudio.vitte-studio");
      assert.ok(extension, "The installed Vitte VSIX was not discovered.");

      const installedPath = path.resolve(extension.extensionPath);
      const extensionsRoot = path.resolve(process.env.VITTE_SMOKE_EXTENSIONS_DIR);
      const sourceRoot = path.resolve(process.env.VITTE_SMOKE_SOURCE_ROOT);
      assert.ok(installedPath.startsWith(`${extensionsRoot}${path.sep}`), "Vitte loaded outside the isolated extensions directory.");
      assert.ok(!installedPath.startsWith(`${sourceRoot}${path.sep}`), "Vitte loaded from the source checkout.");

      const manifestPath = path.join(installedPath, "package.json");
      const manifest = JSON.parse(fs.readFileSync(manifestPath, "utf8"));
      assert.equal(manifest.version, process.env.VITTE_SMOKE_VERSION);
      for (const relative of [
        "assets/logo.png",
        "out/extension.js",
        "out/server.js",
        "server/out/src/server.js",
        "syntaxes/vitte.tmLanguage.json",
      ]) {
        assert.ok(fs.existsSync(path.join(installedPath, relative)), `Packaged resource is missing: ${relative}`);
      }

      const workspace = vscode.workspace.workspaceFolders?.[0];
      assert.ok(workspace, "The smoke workspace did not open.");
      const sourceFile = path.join(workspace.uri.fsPath, "activation smoke.vit");
      fs.writeFileSync(sourceFile, "proc main() -> int {\n  give 0;\n}\n", "utf8");
      const document = await vscode.workspace.openTextDocument(vscode.Uri.file(sourceFile));
      assert.equal(document.languageId, "vit");
      await vscode.window.showTextDocument(document, { preview: false });
      await extension.activate();
      assert.equal(extension.isActive, true);
      const commands = new Set(await vscode.commands.getCommands(true));
      assert.ok(commands.has("vitte.restartServer"));
      assert.ok(commands.has("vitte.showServerLog"));
      const api = extension.exports;
      assert.equal(api?.apiVersion, "1.0.0", "The packaged extension API did not activate.");

      const sentinel = path.join(workspace.uri.fsPath, "keep-user-data.txt");
      assert.equal(fs.readFileSync(sentinel, "utf8"), "keep-this-project-file\n");
    });
  });

  await new Promise((resolve, reject) => {
    mocha.run((failures) => failures === 0 ? resolve() : reject(new Error(`${failures} packaged smoke test(s) failed.`)));
  });
};
