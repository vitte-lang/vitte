package Vitte::City::Athens::Permissions;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    ensure_dir permissions is_readable is_writable is_executable permission_bits
    chmod_path file_owner_id file_group_id file_mode_summary
);

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

sub is_executable {
    my ($path) = @_;
    return -x $path ? 1 : 0;
}

sub permission_bits {
    my ($path) = @_;
    my @stat = stat($path);
    return undef unless @stat;
    return $stat[2] & 07777;
}

sub chmod_path {
    my ($path, $mode) = @_;
    chmod($mode, $path) or die "chmod $path: $!";
    return permissions($path);
}

sub file_owner_id {
    my ($path) = @_;
    my @stat = stat($path);
    return undef unless @stat;
    return $stat[4];
}

sub file_group_id {
    my ($path) = @_;
    my @stat = stat($path);
    return undef unless @stat;
    return $stat[5];
}

sub file_mode_summary {
    my ($path) = @_;
    return {
        permissions => permissions($path),
        readable => is_readable($path),
        writable => is_writable($path),
        executable => is_executable($path),
        owner_id => file_owner_id($path),
        group_id => file_group_id($path),
    };
}

1;
