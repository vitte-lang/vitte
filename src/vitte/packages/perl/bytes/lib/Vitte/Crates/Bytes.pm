package Vitte::Crates::Bytes;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(bytes from_string len slice);

sub bytes { return [ map { ord($_) } split //, ($_[0] // '') ]; }
sub from_string { bytes($_[0]) }
sub len { scalar @{$_[0]} }
sub slice { my ($b,$start,$count)=@_; return [ @$b[$start .. ($start+$count-1)] ]; }

1;
