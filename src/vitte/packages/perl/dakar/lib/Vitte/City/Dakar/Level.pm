package Vitte::City::Dakar::Level;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    level_all
    level_above
    level_below
    level_compare
    level_enabled
    level_known
    level_name
    level_value
    max_level
    min_level
    normalize_level
);

my %LEVEL = (
    trace => 5,
    debug => 10,
    info  => 20,
    warn  => 30,
    error => 40,
    fatal => 50,
);

my %NAME = reverse %LEVEL;

sub normalize_level {
    my ($level) = @_;
    $level = 'info' unless defined $level && length $level;
    $level = lc "$level";
    die "unknown log level '$level'" unless exists $LEVEL{$level};
    return $level;
}

sub level_value {
    return $LEVEL{ normalize_level($_[0]) };
}

sub level_name {
    my ($value) = @_;
    return $NAME{$value} if exists $NAME{$value};
    die "unknown log level value '$value'";
}

sub level_known {
    my ($level) = @_;
    return defined $level && exists $LEVEL{ lc "$level" } ? 1 : 0;
}

sub level_all {
    return [ sort { $LEVEL{$a} <=> $LEVEL{$b} } keys %LEVEL ];
}

sub level_compare {
    return level_value($_[0]) <=> level_value($_[1]);
}

sub level_enabled {
    my ($event_level, $logger_level) = @_;
    return level_value($event_level) >= level_value($logger_level) ? 1 : 0;
}

sub level_above {
    return level_value($_[0]) > level_value($_[1]) ? 1 : 0;
}

sub level_below {
    return level_value($_[0]) < level_value($_[1]) ? 1 : 0;
}

sub min_level {
    my ($left, $right) = @_;
    return level_value($left) <= level_value($right) ? normalize_level($left) : normalize_level($right);
}

sub max_level {
    my ($left, $right) = @_;
    return level_value($left) >= level_value($right) ? normalize_level($left) : normalize_level($right);
}

1;
