package Vitte::City::Paris;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    args_to_hash
    command
    command_spec
    flag_count
    get_flag_count
    has_flag
    help_requested
    merge_defaults
    normalize_flag
    option
    option_bool
    option_int
    option_list
    options
    parse_args
    parse_config_line
    parse_env
    positionals
    quote_arg
    render_table
    require_option
    route_command
    shell_join
    spec_option
    split_kv
    subcommand
    unknown_options
    usage
    validate_options
    validate_spec
    version_requested
);

sub normalize_flag {
    my ($name) = @_;
    $name = '' unless defined $name;
    $name =~ s/\A--?//;
    $name =~ tr/-/_/;
    return $name;
}

sub split_kv {
    my ($arg) = @_;
    return ($1, $2) if defined $arg && $arg =~ /\A--?([^=]+)=(.*)\z/s;
    return (undef, undef);
}

sub parse_args {
    my ($argv) = @_;
    $argv ||= [];

    my %flags;
    my %options;
    my %counts;
    my @positionals;
    my @unknown;
    my $literal = 0;

    for (my $i = 0; $i < @$argv; $i++) {
        my $arg = $argv->[$i];

        if ($literal) {
            push @positionals, $arg;
            next;
        }

        if ($arg eq '--') {
            $literal = 1;
            next;
        }

        if ($arg =~ /\A--([^=]+)=(.*)\z/s) {
            my $key = normalize_flag($1);
            push @{ $options{$key} }, $2;
            $counts{$key}++;
            next;
        }

        if ($arg =~ /\A--(.+)\z/s) {
            my $key = normalize_flag($1);
            if ($i + 1 < @$argv && $argv->[$i + 1] !~ /\A-/) {
                push @{ $options{$key} }, $argv->[++$i];
            } else {
                $flags{$key} = 1;
            }
            $counts{$key}++;
            next;
        }

        if ($arg =~ /\A-o(.+)\z/s) {
            push @{ $options{o} }, $1;
            $counts{o}++;
            next;
        }

        if ($arg =~ /\A-([A-Za-z0-9]+)\z/s) {
            for my $char (split //, $1) {
                my $key = normalize_flag($char);
                $flags{$key} = 1;
                $counts{$key}++;
            }
            next;
        }

        push @positionals, $arg;
    }

    return {
        flags       => \%flags,
        options     => \%options,
        counts      => \%counts,
        positionals => \@positionals,
        unknown     => \@unknown,
        command     => @positionals ? $positionals[0] : undef,
        subcommand  => @positionals > 1 ? $positionals[1] : undef,
    };
}

sub has_flag {
    my ($parsed, $name) = @_;
    my $key = normalize_flag($name);
    return $parsed->{flags}{$key} ? 1 : 0;
}

sub flag_count {
    my ($parsed, $name) = @_;
    my $key = normalize_flag($name);
    return $parsed->{counts}{$key} || 0;
}

sub get_flag_count {
    return flag_count(@_);
}

sub option {
    my ($parsed, $name, $default) = @_;
    my $key = normalize_flag($name);
    return exists $parsed->{options}{$key} ? $parsed->{options}{$key}[-1] : $default;
}

sub options {
    my ($parsed) = @_;
    return {
        map { $_ => [ @{ $parsed->{options}{$_} || [] } ] }
        sort keys %{ $parsed->{options} || {} }
    };
}

sub option_list {
    my ($parsed, $name) = @_;
    my $key = normalize_flag($name);
    return [ @{ $parsed->{options}{$key} || [] } ];
}

sub option_int {
    my ($parsed, $name, $default) = @_;
    my $value = option($parsed, $name, $default);
    return defined $value && $value =~ /\A-?[0-9]+\z/ ? 0 + $value : $default;
}

sub option_bool {
    my ($parsed, $name, $default) = @_;
    return 1 if has_flag($parsed, $name);
    my $value = option($parsed, $name, undef);
    return $default unless defined $value;
    return 1 if $value =~ /\A(?:1|true|yes|on)\z/i;
    return 0 if $value =~ /\A(?:0|false|no|off)\z/i;
    return $default;
}

sub positionals {
    my ($parsed) = @_;
    return [ @{ $parsed->{positionals} || [] } ];
}

sub command {
    return $_[0]->{command};
}

sub subcommand {
    return $_[0]->{subcommand};
}

sub usage {
    my (%spec) = @_;
    my $name = $spec{name} || 'command';
    my $summary = $spec{summary} || '';
    my @lines = ("Usage: $name [options] [args]");
    push @lines, $summary if length $summary;
    for my $opt (@{ $spec{options} || [] }) {
        my $flag = $opt->{flag} || $opt->{name} || '';
        my $desc = $opt->{description} || '';
        push @lines, sprintf('  %-18s %s', $flag, $desc);
    }
    return join("\n", @lines) . "\n";
}

sub require_option {
    my ($parsed, $name) = @_;
    my $value = option($parsed, $name, undef);
    die 'missing required option --' . normalize_flag($name) . "\n" unless defined $value;
    return $value;
}

sub merge_defaults {
    my ($parsed, $defaults) = @_;
    $defaults ||= {};
    my $copy = {
        flags       => { %{ $parsed->{flags} || {} } },
        options     => {
            map { $_ => [ @{ $parsed->{options}{$_} || [] } ] }
            keys %{ $parsed->{options} || {} }
        },
        counts      => { %{ $parsed->{counts} || {} } },
        positionals => [ @{ $parsed->{positionals} || [] } ],
        unknown     => [ @{ $parsed->{unknown} || [] } ],
        command     => $parsed->{command},
        subcommand  => $parsed->{subcommand},
    };
    for my $name (sort keys %$defaults) {
        my $key = normalize_flag($name);
        $copy->{options}{$key} ||= [ $defaults->{$name} ];
    }
    return $copy;
}

sub parse_env {
    my ($prefix, $env) = @_;
    $env ||= \%ENV;
    my %out;
    for my $key (sort keys %$env) {
        next unless index($key, $prefix) == 0;
        my $name = lc substr($key, length($prefix));
        $name =~ tr/-/_/;
        $out{$name} = $env->{$key};
    }
    return \%out;
}

sub help_requested {
    return has_flag($_[0], 'help') || has_flag($_[0], 'h');
}

sub version_requested {
    return has_flag($_[0], 'version') || has_flag($_[0], 'V');
}

sub unknown_options {
    my ($parsed, $allowed) = @_;
    my %allowed = map { normalize_flag($_) => 1 } @{ $allowed || [] };
    my @seen = sort keys %{ $parsed->{counts} || {} };
    return [ grep { !$allowed{$_} } @seen ];
}

sub validate_options {
    my ($parsed, $allowed) = @_;
    my $unknown = unknown_options($parsed, $allowed);
    return {
        ok      => @$unknown ? 0 : 1,
        unknown => $unknown,
    };
}

sub parse_config_line {
    my ($line) = @_;
    return undef unless defined $line;
    $line =~ s/#.*\z//;
    $line =~ s/\A\s+|\s+\z//g;
    return undef unless length $line;
    return { key => $1, value => $2 } if $line =~ /\A([^=:\s]+)\s*[:=]\s*(.*)\z/s;
    return undef;
}

sub args_to_hash {
    my ($parsed) = @_;
    return {
        flags       => { %{ $parsed->{flags} || {} } },
        options     => options($parsed),
        positionals => positionals($parsed),
    };
}

sub render_table {
    my ($rows) = @_;
    $rows ||= [];
    my $width = 0;
    for my $row (@$rows) {
        my $left = defined $row->[0] ? $row->[0] : '';
        $width = length($left) if length($left) > $width;
    }
    return join("\n", map {
        sprintf('%-*s  %s', $width, defined $_->[0] ? $_->[0] : '', defined $_->[1] ? $_->[1] : '')
    } @$rows) . (@$rows ? "\n" : '');
}

sub quote_arg {
    my ($arg) = @_;
    $arg = '' unless defined $arg;
    return $arg if $arg =~ /\A[A-Za-z0-9_.,:+=\/@%-]+\z/;
    $arg =~ s/'/'"'"'/g;
    return "'$arg'";
}

sub shell_join {
    return join ' ', map { quote_arg($_) } @_;
}

sub spec_option {
    my (%option) = @_;
    my $name = normalize_flag($option{name} || $option{flag});
    return {
        %option,
        name => $name,
    };
}

sub command_spec {
    my (%spec) = @_;
    $spec{options} ||= [];
    $spec{commands} ||= {};
    return \%spec;
}

sub validate_spec {
    my ($spec) = @_;
    my @errors;
    push @errors, 'missing name' unless $spec->{name};
    my %seen;
    for my $opt (@{ $spec->{options} || [] }) {
        my $name = normalize_flag($opt->{name} || $opt->{flag});
        push @errors, "duplicate option $name" if $seen{$name}++;
    }
    return {
        ok     => @errors ? 0 : 1,
        errors => \@errors,
    };
}

sub route_command {
    my ($parsed, $routes) = @_;
    $routes ||= {};
    my $cmd = command($parsed);
    return $routes->{$cmd} if defined $cmd && exists $routes->{$cmd};
    return $routes->{default};
}

1;
