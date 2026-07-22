package Vitte::Crates::Uuid;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(nil is_nil parse_uuid format_uuid);

sub nil { return '00000000-0000-0000-0000-000000000000'; }
sub is_nil { return $_[0] eq nil() ? 1 : 0; }
sub parse_uuid { my ($text)=@_; return $text =~ /\A[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\z/ ? lc($text) : undef; }
sub format_uuid { return lc($_[0]); }

1;
