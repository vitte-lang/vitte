package Vitte::Crates::Tempfile;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(temp_name temp_path cleanup_plan);

sub temp_name { my ($prefix,$suffix)=@_; $prefix ||= 'tmp'; $suffix ||= ''; return $prefix . '-' . $$ . '-' . int(rand(1000000)) . $suffix; }
sub temp_path { my ($dir,$name)=@_; $dir ||= '/tmp'; return $dir . '/' . $name; }
sub cleanup_plan { my (@paths)=@_; return { remove => \@paths }; }

1;
