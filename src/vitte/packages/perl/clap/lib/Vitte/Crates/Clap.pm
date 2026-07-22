package Vitte::Crates::Clap;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(parse_flags has_flag flag_value usage);

sub parse_flags {
    my (@args) = @_;
    my %out;
    while (@args) {
        my $arg = shift @args;
        if ($arg =~ /\A--([^=]+)=(.*)\z/) { $out{$1} = $2; next; }
        if ($arg =~ /\A--(.+)\z/) { $out{$1} = (@args && $args[0] !~ /\A-/) ? shift @args : 1; next; }
    }
    return \%out;
}
sub has_flag { exists $_[0]->{$_[1]} ? 1 : 0 }
sub flag_value { $_[0]->{$_[1]} }
sub usage { my ($name,@flags)=@_; return 'usage: '.$name.' '.join(' ', map {'--'.$_} @flags); }

1;
