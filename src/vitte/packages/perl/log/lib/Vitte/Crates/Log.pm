package Vitte::Crates::Log;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(logger log_info log_warn entries);

sub logger { return { entries => [] }; }
sub log_info { my ($logger,$message)=@_; push @{$logger->{entries}}, ['INFO',$message]; return $logger; }
sub log_warn { my ($logger,$message)=@_; push @{$logger->{entries}}, ['WARN',$message]; return $logger; }
sub entries { return $_[0]->{entries}; }

1;
