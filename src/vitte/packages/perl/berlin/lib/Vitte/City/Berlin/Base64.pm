package Vitte::City::Berlin::Base64;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Berlin::Buffer qw(buffer buffer_bytes);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    base64_decode
    base64_encode
    base64_is_valid
    base64_pad
    base64_unpad
    base64url_decode
    base64url_encode
);

my @ENC = split //, 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
my %DEC = map { $ENC[$_] => $_ } 0 .. $#ENC;

sub base64_encode {
    my ($buf) = @_;
    my @bytes = @{ buffer_bytes($buf) };
    my $out = '';
    while (@bytes) {
        my $a = shift @bytes;
        my $b = @bytes ? shift @bytes : undef;
        my $c = @bytes ? shift @bytes : undef;
        my $triple = ($a << 16) | ((defined $b ? $b : 0) << 8) | (defined $c ? $c : 0);
        $out .= $ENC[($triple >> 18) & 63];
        $out .= $ENC[($triple >> 12) & 63];
        $out .= defined $b ? $ENC[($triple >> 6) & 63] : '=';
        $out .= defined $c ? $ENC[$triple & 63] : '=';
    }
    return $out;
}

sub base64_pad {
    my ($text) = @_;
    $text = '' unless defined $text;
    my $missing = (4 - (length($text) % 4)) % 4;
    return $text . ('=' x $missing);
}

sub base64_unpad {
    my ($text) = @_;
    $text = '' unless defined $text;
    $text =~ s/=+\z//;
    return $text;
}

sub base64_is_valid {
    my ($text) = @_;
    $text = '' unless defined $text;
    $text =~ s/\s+//g;
    return 0 if length($text) % 4;
    return $text =~ /\A[A-Za-z0-9+\/]*={0,2}\z/ ? 1 : 0;
}

sub base64_decode {
    my ($text) = @_;
    $text = '' unless defined $text;
    $text =~ s/\s+//g;
    die 'invalid base64 length' if length($text) % 4;
    die 'invalid base64 data' unless base64_is_valid($text);
    my @out;
    while ($text =~ /(.)(.)(.)(.)/g) {
        my ($a, $b, $c, $d) = ($1, $2, $3, $4);
        my $triple = ($DEC{$a} << 18) | ($DEC{$b} << 12) | (($c eq '=' ? 0 : $DEC{$c}) << 6) | ($d eq '=' ? 0 : $DEC{$d});
        push @out, ($triple >> 16) & 255;
        push @out, ($triple >> 8) & 255 unless $c eq '=';
        push @out, $triple & 255 unless $d eq '=';
    }
    return buffer(@out);
}

sub base64url_encode {
    my ($buf, $padding) = @_;
    my $text = base64_encode($buf);
    $text =~ tr{+/}{-_};
    $text = base64_unpad($text) unless $padding;
    return $text;
}

sub base64url_decode {
    my ($text) = @_;
    $text = '' unless defined $text;
    $text =~ tr{-_}{+/};
    return base64_decode(base64_pad($text));
}

1;
