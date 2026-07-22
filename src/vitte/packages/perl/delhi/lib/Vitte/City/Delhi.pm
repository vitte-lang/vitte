package Vitte::City::Delhi;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(render_template indent dedent wrap_words slugify lines);

sub render_template {
    my ($template, $vars) = @_;
    $template =~ s/\{\{\s*([A-Za-z0-9_]+)\s*\}\}/exists $vars->{$1} ? $vars->{$1} : ''/ge;
    return $template;
}

sub indent {
    my ($text, $prefix) = @_;
    $prefix = '  ' unless defined $prefix;
    $text =~ s/^/$prefix/mg;
    return $text;
}

sub dedent {
    my ($text) = @_;
    $text =~ s/^\s+//mg;
    return $text;
}

sub wrap_words {
    my ($text, $width) = @_;
    $width ||= 80;
    my @lines;
    my $line = '';
    for my $word (split /\s+/, $text) {
        if (length($line) && length($line) + 1 + length($word) > $width) {
            push @lines, $line;
            $line = $word;
        } else {
            $line = length($line) ? "$line $word" : $word;
        }
    }
    push @lines, $line if length $line;
    return join("\n", @lines);
}

sub slugify { my ($text) = @_; $text = lc $text; $text =~ s/[^a-z0-9]+/-/g; $text =~ s/^-|-$//g; return $text }
sub lines { return [ split /\n/, $_[0] || '' ] }

1;
