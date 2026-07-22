package Vitte::Crates::Anyhow;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(ok err is_ok is_err unwrap context);

sub ok { return { ok => 1, value => $_[0], error => undef } }
sub err { return { ok => 0, value => undef, error => $_[0] } }
sub is_ok { $_[0]->{ok} ? 1 : 0 }
sub is_err { $_[0]->{ok} ? 0 : 1 }
sub unwrap { die $_[0]->{error} if is_err($_[0]); return $_[0]->{value}; }
sub context { my ($result,$message)=@_; return $result if is_ok($result); return err($message . ': ' . $result->{error}); }

1;
