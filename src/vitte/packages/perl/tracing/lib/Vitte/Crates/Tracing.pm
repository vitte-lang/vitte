package Vitte::Crates::Tracing;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(span event record fields);

sub span { my ($name,%fields)=@_; return { name=>$name, fields=>\%fields, events=>[] }; }
sub event { my ($level,$message,%fields)=@_; return { level=>$level, message=>$message, fields=>\%fields }; }
sub record { my ($span,$event)=@_; push @{$span->{events}}, $event; return $span; }
sub fields { my ($span)=@_; return $span->{fields}; }

1;
