class Vitte < Formula
  desc "Vitte compiler"
  homepage "https://github.com/vitte-lang/vitte"
  url "https://github.com/vitte-lang/vitte/releases/download/Vitte2026_02/vitte-2.0.0-bin.tar.gz"
  sha256 "294217e20221445281129ee97a36cef613d0538ac2f3d50c529c546ed3db0586"
  version "2.0.0"

  def install
    bin.install "bin/vitte"
  end

  test do
    assert_match "vitte", shell_output("#{bin}/vitte --help", 1)
  end
end
