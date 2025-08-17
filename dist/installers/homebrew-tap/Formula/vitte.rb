# Tap example (copy to: github.com/<org>/homebrew-tap/Formula/vitte.rb)
class Vitte < Formula
  desc "Vitte toolchain"
  homepage "https://example.org/vitte"
  url "https://example.org/releases/vitte-macos-universal.tar.gz"
  sha256 "<fill-me>"
  def install
    bin.install "vitte"
    bin.install "vitte-cli"
  end
end
