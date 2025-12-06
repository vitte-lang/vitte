class Vittec < Formula
  desc "Vitte compiler placeholder (lexer/parser/AST/diag)"
  homepage "https://github.com/vitte-lang/vitte-core"
  url "https://github.com/vitte-lang/vitte-core/releases/download/v0.1.0/vittec-0.1.0-darwin-arm64.tar.gz"
  sha256 "8667f957960e326a9c727fce5a8d9c13cf5e67ffec08d88a1378619b41b8a3a7"
  version "0.1.0"
  license "MIT"

  def install
    bin.install "vittec"
    pkgshare.install "README.md" if File.exist?("README.md")
    pkgshare.install "LICENSE" if File.exist?("LICENSE")
  end

  test do
    (testpath/"hello.vitte").write("main() {}")
    output = shell_output("#{bin}/vittec #{testpath}/hello.vitte")
    assert_match "[stage1]", output
  end
end
