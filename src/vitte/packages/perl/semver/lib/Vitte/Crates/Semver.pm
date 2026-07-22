package Vitte::Crates::Semver;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(parse_version cmp_version satisfies);

sub parse_version { my ($v)=@_; return undef unless $v =~ /\A(\d+)\.(\d+)\.(\d+)\z/; return { major=>$1+0, minor=>$2+0, patch=>$3+0 }; }
sub cmp_version { my ($a,$b)=@_; for my $k (qw(major minor patch)) { return $a->{$k} <=> $b->{$k} if $a->{$k} != $b->{$k}; } return 0; }
sub satisfies { my ($v,$min)=@_; return cmp_version(parse_version($v), parse_version($min)) >= 0 ? 1 : 0; }

1;
