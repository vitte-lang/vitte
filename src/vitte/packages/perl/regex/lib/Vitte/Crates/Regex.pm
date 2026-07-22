package Vitte::Crates::Regex;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(is_match find_all replace_all escape_regex);

sub is_match { my ($pattern,$text)=@_; return $text =~ /$pattern/ ? 1 : 0; }
sub find_all { my ($pattern,$text)=@_; my @m = ($text =~ /($pattern)/g); return \@m; }
sub replace_all { my ($pattern,$replacement,$text)=@_; $text =~ s/$pattern/$replacement/g; return $text; }
sub escape_regex { my ($text)=@_; $text =~ s/([^A-Za-z0-9_])/\\$1/g; return $text; }

1;
