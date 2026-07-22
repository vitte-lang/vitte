package Vitte::City::Berlin::Buffer;

use strict;
use warnings;
use bytes ();
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    append
    buffer
    buffer_bytes
    buffer_clone
    buffer_compare
    buffer_equals
    buffer_frequency
    byte_at
    clear
    concat_buffers
    contains_bytes
    ends_with
    fill
    from_string
    index_of
    insert_bytes
    is_empty
    length_of
    pad_left
    pad_right
    remove_range
    repeat_buffer
    replace_range
    resize
    reverse_bytes
    slice
    starts_with
    to_string
    xor_buffers
    and_buffers
    or_buffers
);

sub _bytes {
    my ($buf) = @_;
    return [] unless defined $buf && ref($buf) eq 'HASH' && ref($buf->{bytes}) eq 'ARRAY';
    return $buf->{bytes};
}

sub _byte {
    my ($value) = @_;
    return int(defined $value ? $value : 0) & 255;
}

sub _range {
    my ($buf, $start, $count) = @_;
    my $length = length_of($buf);
    $start = 0 unless defined $start;
    die 'negative buffer index' if $start < 0;
    die 'buffer index out of range' if $start > $length;
    $count = $length - $start unless defined $count;
    die 'negative buffer length' if $count < 0;
    die 'buffer range out of range' if $start + $count > $length;
    return ($start, $count);
}

sub buffer {
    my @bytes = @_;
    return {
        bytes  => [ map { _byte($_) } @bytes ],
        cursor => 0,
    };
}

sub from_string {
    my ($text) = @_;
    $text = '' unless defined $text;
    return buffer(unpack('C*', $text));
}

sub to_string {
    my ($buf) = @_;
    return pack('C*', @{ _bytes($buf) });
}

sub append {
    my ($buf, @bytes) = @_;
    push @{ _bytes($buf) }, map { _byte($_) } @bytes;
    return $buf;
}

sub buffer_bytes {
    my ($buf) = @_;
    return [ @{ _bytes($buf) } ];
}

sub buffer_clone {
    my ($buf) = @_;
    my $copy = buffer(@{ _bytes($buf) });
    $copy->{cursor} = $buf->{cursor} || 0 if defined $buf && ref($buf) eq 'HASH';
    return $copy;
}

sub length_of {
    my ($buf) = @_;
    return scalar @{ _bytes($buf) };
}

sub is_empty {
    return length_of($_[0]) == 0 ? 1 : 0;
}

sub byte_at {
    my ($buf, $index) = @_;
    die 'byte index out of range' if !defined $index || $index < 0 || $index >= length_of($buf);
    return _bytes($buf)->[$index];
}

sub slice {
    my ($buf, $start, $count) = @_;
    ($start, $count) = _range($buf, $start, $count);
    return buffer() if $count == 0;
    return buffer(@{ _bytes($buf) }[$start .. $start + $count - 1]);
}

sub clear {
    my ($buf) = @_;
    $buf->{bytes} = [];
    $buf->{cursor} = 0;
    return $buf;
}

sub resize {
    my ($buf, $size, $fill) = @_;
    die 'negative buffer size' if !defined $size || $size < 0;
    $fill = 0 unless defined $fill;
    while (length_of($buf) < $size) {
        append($buf, $fill);
    }
    splice @{ _bytes($buf) }, $size if length_of($buf) > $size;
    $buf->{cursor} = $size if ($buf->{cursor} || 0) > $size;
    return $buf;
}

sub fill {
    my ($buf, $value, $start, $count) = @_;
    ($start, $count) = _range($buf, $start, $count);
    for my $i ($start .. $start + $count - 1) {
        _bytes($buf)->[$i] = _byte($value);
    }
    return $buf;
}

sub reverse_bytes {
    my ($buf) = @_;
    return buffer(reverse @{ _bytes($buf) });
}

sub concat_buffers {
    my @buffers = @_;
    my @bytes;
    for my $buf (@buffers) {
        push @bytes, @{ _bytes($buf) };
    }
    return buffer(@bytes);
}

sub insert_bytes {
    my ($buf, $index, @bytes) = @_;
    die 'insert index out of range' if !defined $index || $index < 0 || $index > length_of($buf);
    splice @{ _bytes($buf) }, $index, 0, map { _byte($_) } @bytes;
    $buf->{cursor} += @bytes if ($buf->{cursor} || 0) >= $index;
    return $buf;
}

sub remove_range {
    my ($buf, $start, $count) = @_;
    ($start, $count) = _range($buf, $start, $count);
    my @removed = splice @{ _bytes($buf) }, $start, $count;
    $buf->{cursor} = length_of($buf) if ($buf->{cursor} || 0) > length_of($buf);
    return buffer(@removed);
}

sub replace_range {
    my ($buf, $start, $count, @bytes) = @_;
    ($start, $count) = _range($buf, $start, $count);
    splice @{ _bytes($buf) }, $start, $count, map { _byte($_) } @bytes;
    $buf->{cursor} = length_of($buf) if ($buf->{cursor} || 0) > length_of($buf);
    return $buf;
}

sub repeat_buffer {
    my ($buf, $count) = @_;
    die 'negative repeat count' if !defined $count || $count < 0;
    my @bytes;
    push @bytes, @{ _bytes($buf) } for 1 .. $count;
    return buffer(@bytes);
}

sub pad_left {
    my ($buf, $size, $fill) = @_;
    $fill = 0 unless defined $fill;
    return $buf if length_of($buf) >= $size;
    my $missing = $size - length_of($buf);
    splice @{ _bytes($buf) }, 0, 0, (($fill) x $missing);
    return fill($buf, $fill, 0, $missing);
}

sub pad_right {
    my ($buf, $size, $fill) = @_;
    $fill = 0 unless defined $fill;
    resize($buf, $size, $fill);
    return $buf;
}

sub buffer_compare {
    my ($left, $right) = @_;
    my $a = _bytes($left);
    my $b = _bytes($right);
    my $n = @$a < @$b ? @$a : @$b;
    for my $i (0 .. $n - 1) {
        return -1 if $a->[$i] < $b->[$i];
        return 1 if $a->[$i] > $b->[$i];
    }
    return @$a <=> @$b;
}

sub buffer_equals {
    return buffer_compare($_[0], $_[1]) == 0 ? 1 : 0;
}

sub starts_with {
    my ($buf, $prefix) = @_;
    return 0 if length_of($prefix) > length_of($buf);
    return buffer_equals(slice($buf, 0, length_of($prefix)), $prefix);
}

sub ends_with {
    my ($buf, $suffix) = @_;
    return 0 if length_of($suffix) > length_of($buf);
    return buffer_equals(slice($buf, length_of($buf) - length_of($suffix), length_of($suffix)), $suffix);
}

sub index_of {
    my ($buf, $needle, $start) = @_;
    $start = 0 unless defined $start;
    return -1 if $start < 0;
    my $needle_length = length_of($needle);
    return $start <= length_of($buf) ? $start : -1 if $needle_length == 0;
    return -1 if $needle_length > length_of($buf);
    for my $i ($start .. length_of($buf) - $needle_length) {
        return $i if buffer_equals(slice($buf, $i, $needle_length), $needle);
    }
    return -1;
}

sub contains_bytes {
    return index_of($_[0], $_[1], $_[2]) >= 0 ? 1 : 0;
}

sub buffer_frequency {
    my ($buf) = @_;
    my %freq;
    $freq{$_}++ for @{ _bytes($buf) };
    return \%freq;
}

sub _binary_op {
    my ($left, $right, $op) = @_;
    my $length = length_of($left) > length_of($right) ? length_of($left) : length_of($right);
    my @out;
    for my $i (0 .. $length - 1) {
        my $a = _bytes($left)->[$i] || 0;
        my $b = _bytes($right)->[$i] || 0;
        push @out, _byte($op->($a, $b));
    }
    return buffer(@out);
}

sub xor_buffers { return _binary_op($_[0], $_[1], sub { $_[0] ^ $_[1] }) }
sub and_buffers { return _binary_op($_[0], $_[1], sub { $_[0] & $_[1] }) }
sub or_buffers  { return _binary_op($_[0], $_[1], sub { $_[0] | $_[1] }) }

1;
