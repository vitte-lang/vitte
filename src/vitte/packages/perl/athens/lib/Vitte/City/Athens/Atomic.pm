package Vitte::City::Athens::Atomic;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Athens::Path qw(dirname join_path);
use Vitte::City::Athens::Permissions qw(ensure_dir);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(atomic_write read_text write_text);

sub write_text {
    my ($path, $content) = @_;
    $content = '' unless defined $content;
    ensure_dir(dirname($path));
    open(my $fh, '>', $path) or die "write $path: $!";
    binmode($fh);
    print {$fh} $content;
    close($fh) or die "close $path: $!";
    return length($content);
}

sub read_text {
    my ($path) = @_;
    open(my $fh, '<', $path) or die "read $path: $!";
    binmode($fh);
    local $/;
    my $content = <$fh>;
    close($fh) or die "close $path: $!";
    return $content;
}

sub atomic_write {
    my ($path, $content) = @_;
    my $dir = dirname($path);
    ensure_dir($dir);
    my $tmp = join_path($dir, '.athens-' . $$ . '-' . time() . '.tmp');
    write_text($tmp, $content);
    rename($tmp, $path) or die "rename $tmp to $path: $!";
    return length(defined $content ? $content : '');
}

1;
