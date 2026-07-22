package Vitte::Crates::Chrono;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(duration seconds millis format_epoch);

sub duration { my (%args)=@_; return ($args{seconds}||0) * 1000 + ($args{millis}||0); }
sub seconds { return $_[0] * 1000; }
sub millis { return $_[0]; }
sub format_epoch { my ($seconds)=@_; return 'epoch+' . $seconds . 's'; }

1;
