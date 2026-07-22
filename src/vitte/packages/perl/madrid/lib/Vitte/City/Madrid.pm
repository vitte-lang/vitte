package Vitte::City::Madrid;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(nil_id is_nil normalize_id is_id format_id new_counter_id short_id);

my $COUNTER = 0;

sub nil_id { return '00000000-0000-4000-8000-000000000000' }
sub is_nil { return normalize_id($_[0]) eq nil_id() ? 1 : 0 }

sub normalize_id {
    my ($id) = @_;
    $id = lc($id || '');
    $id =~ s/[^0-9a-f]//g;
    die 'identifier must contain 32 hex digits' unless length($id) == 32;
    return format_id($id);
}

sub is_id {
    my ($id) = @_;
    return eval { normalize_id($id); 1 } ? 1 : 0;
}

sub format_id {
    my ($hex) = @_;
    $hex =~ s/-//g;
    return substr($hex, 0, 8) . '-' . substr($hex, 8, 4) . '-' . substr($hex, 12, 4) . '-' . substr($hex, 16, 4) . '-' . substr($hex, 20, 12);
}

sub new_counter_id {
    my ($seed) = @_;
    $COUNTER++;
    my $base = sprintf('%08x%04x40008000%012x', int($seed || 0) & 0xffffffff, $COUNTER & 0xffff, $COUNTER);
    return format_id($base);
}

sub short_id {
    my ($id) = @_;
    return substr(normalize_id($id), 0, 8);
}

1;
