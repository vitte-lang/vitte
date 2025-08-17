class Vitte < Formula
  desc "Vitte language toolchain"
  homepage "https://example.org/vitte"
  version "0.1.0"
  on_macos do
    url "https://example.org/releases/vitte-macos-universal.tar.gz"
    sha256 "<fill-me>"
  end
  on_linux do
    if Hardware::CPU.arm?
      url "https://example.org/releases/vitte-linux-aarch64.tar.gz"
    else
      url "https://example.org/releases/vitte-linux-x86_64.tar.gz"
    end
    sha256 "<fill-me>"
  end
  def install
    bin.install "vitte"
    bin.install "vitte-cli"
  end
  test do
    system "#<built-in function bin>/vitte", "--version"
  end
end
