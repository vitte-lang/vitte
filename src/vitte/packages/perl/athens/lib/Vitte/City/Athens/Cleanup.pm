package Vitte::City::Athens::Cleanup;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(cleanup_plan cleanup_add cleanup_run cleanup_dry_run);

sub cleanup_plan {
    my @paths = @_;
    return { remove => [ @paths ], count => scalar @paths };
}

sub cleanup_add {
    my ($plan, @paths) = @_;
    push @{ $plan->{remove} }, @paths;
    $plan->{count} = scalar @{ $plan->{remove} };
    return $plan;
}

sub cleanup_dry_run {
    my ($plan) = @_;
    return [ @{ $plan->{remove} || [] } ];
}

sub _remove_path {
    my ($path) = @_;
    return 0 unless defined $path && -e $path;
    if (-d $path && !-l $path) {
        opendir(my $dh, $path) or die "open directory $path: $!";
        my @entries = grep { $_ ne '.' && $_ ne '..' } readdir($dh);
        closedir($dh);
        _remove_path("$path/$_") for @entries;
        rmdir($path) or die "remove directory $path: $!";
        return 1;
    }
    unlink($path) or die "remove file $path: $!";
    return 1;
}

sub cleanup_run {
    my ($plan) = @_;
    my @removed;
    for my $path (@{ $plan->{remove} || [] }) {
        push @removed, $path if _remove_path($path);
    }
    $plan->{removed} = \@removed;
    return \@removed;
}

1;
