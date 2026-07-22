package Vitte::Crates::Toml;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(parse_toml emit_toml get_key);

sub parse_toml { my ($text)=@_; my %out; for my $line (split /\n/, $text) { next if $line =~ /\A\s*(?:#|\z)/; if ($line =~ /\A\s*([A-Za-z0-9_.-]+)\s*=\s*"?(.*?)"?\s*\z/) { $out{$1}=$2; } } return \%out; }
sub emit_toml { my ($hash)=@_; return join "\n", map { $_.' = "'.$hash->{$_}.'"' } sort keys %$hash; }
sub get_key { return $_[0]->{$_[1]}; }

1;
