package Vitte::City::Oslo;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(duration seconds millis add_duration compare_duration format_duration epoch_parts);

sub duration {
    my (%parts) = @_;
    my $ms = 0;
    $ms += ($parts{days} || 0) * 86_400_000;
    $ms += ($parts{hours} || 0) * 3_600_000;
    $ms += ($parts{minutes} || 0) * 60_000;
    $ms += ($parts{seconds} || 0) * 1_000;
    $ms += ($parts{millis} || 0);
    return { millis => int($ms) };
}

sub seconds { return duration(seconds => $_[0]) }
sub millis { return duration(millis => $_[0]) }
sub add_duration { return millis($_[0]->{millis} + $_[1]->{millis}) }
sub compare_duration { return $_[0]->{millis} <=> $_[1]->{millis} }

sub format_duration {
    my ($d) = @_;
    my $ms = $d->{millis};
    my $s = int($ms / 1000);
    $ms = $ms % 1000;
    return sprintf('%d.%03ds', $s, $ms);
}

sub epoch_parts {
    my ($epoch) = @_;
    my @t = gmtime($epoch || 0);
    return { year => $t[5] + 1900, month => $t[4] + 1, day => $t[3], hour => $t[2], minute => $t[1], second => $t[0] };
}

1;
