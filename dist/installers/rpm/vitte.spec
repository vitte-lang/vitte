Name: vitte
Version: 0.1.0
Release: 1%{?dist}
Summary: Vitte language toolchain
License: MIT
URL: https://example.org/vitte
BuildArch: x86_64

%description
Vitte compiler, VM and CLI.

%install
mkdir -p %{buildroot}/usr/bin
install -m 0755 vitte %{buildroot}/usr/bin/vitte
install -m 0755 vitte-cli %{buildroot}/usr/bin/vitte-cli

%files
/usr/bin/vitte
/usr/bin/vitte-cli
