package Vitte::City::Athens::Permissions;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(ensure_dir permissions is_readable is_writable);

sub ensure_dir {
    my ($path, $mode) = @_;
    $mode = 0755 unless defined $mode;
    my $current = '';
    for my $part (grep { length $_ } split m{/+}, $path) {
        $current .= '/' if length $current || $path =~ m{^/};
        $current .= $part;
        next if -d $current;
        mkdir($current, $mode) or die "create directory $current: $!";
    }
    return $path;
}

sub permissions {
    my ($path) = @_;
    my @stat = stat($path);
    return undef unless @stat;
    return sprintf('%04o', $stat[2] & 07777);
}

sub is_readable {
    my ($path) = @_;
    return -r $path ? 1 : 0;
}

sub is_writable {
    my ($path) = @_;
    return -w $path ? 1 : 0;
}

1;
