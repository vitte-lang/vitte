package Vitte::City::Tokyo;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(runtime spawn run_next block_on join_all defer task_count);

sub runtime { return { queue => [], done => [] } }

sub spawn {
    my ($runtime, $name, $task) = @_;
    push @{ $runtime->{queue} }, { name => $name, task => $task };
    return scalar @{ $runtime->{queue} };
}

sub defer {
    my ($runtime, $task) = @_;
    return spawn($runtime, 'deferred-' . (task_count($runtime) + 1), $task);
}

sub task_count {
    my ($runtime) = @_;
    return scalar @{ $runtime->{queue} || [] };
}

sub run_next {
    my ($runtime) = @_;
    return undef unless @{ $runtime->{queue} || [] };
    my $entry = shift @{ $runtime->{queue} };
    my $value = $entry->{task}->();
    push @{ $runtime->{done} }, { name => $entry->{name}, value => $value };
    return $value;
}

sub block_on {
    my ($runtime) = @_;
    my @values;
    push @values, run_next($runtime) while task_count($runtime);
    return @values;
}

sub join_all {
    my @tasks = @_;
    my $rt = runtime();
    my $i = 0;
    spawn($rt, 'task-' . ++$i, $_) for @tasks;
    return [ block_on($rt) ];
}

1;
