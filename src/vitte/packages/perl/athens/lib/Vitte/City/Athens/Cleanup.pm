package Vitte::City::Athens::Cleanup;

use strict;
use warnings;

use Exporter 'import';
use File::Path qw(remove_tree);

use Vitte::City::Athens::Path qw(
    normalize_path
);

our $VERSION = '0.1.0';

our @EXPORT_OK = qw(
    cleanup_plan
    cleanup_add
    cleanup_add_file
    cleanup_add_dir
    cleanup_add_callback
    cleanup_run
    cleanup_dry_run
    cleanup_clear
    cleanup_size
    cleanup_is_empty
    cleanup_errors
);

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

my $KIND_FILE     = 'file';
my $KIND_DIR      = 'directory';
my $KIND_CALLBACK = 'callback';

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

sub _error {
    my ($operation, $detail) = @_;

    $operation = 'operation'
        unless defined $operation
            && length $operation;

    $detail = 'unknown error'
        unless defined $detail
            && length $detail;

    die "Athens::Cleanup: $operation: $detail\n";
}

sub _validate_plan {
    my ($plan) = @_;

    if (!defined $plan || ref($plan) ne 'HASH') {
        _error(
            'validate cleanup plan',
            'expected a cleanup plan hash reference',
        );
    }

    if (
        !exists $plan->{entries}
        || ref($plan->{entries}) ne 'ARRAY'
    ) {
        _error(
            'validate cleanup plan',
            'cleanup plan has no valid entries array',
        );
    }

    if (
        !exists $plan->{errors}
        || ref($plan->{errors}) ne 'ARRAY'
    ) {
        $plan->{errors} = [];
    }

    return $plan;
}

sub _normalize_options {
    my ($options) = @_;

    return {} unless defined $options;

    if (ref($options) ne 'HASH') {
        _error(
            'validate options',
            'expected a hash reference',
        );
    }

    return $options;
}

sub _normalize_entry_options {
    my ($options) = @_;

    $options = _normalize_options($options);

    return {
        ignore_missing => exists $options->{ignore_missing}
            ? $options->{ignore_missing}
            : 1,

        recursive => exists $options->{recursive}
            ? $options->{recursive}
            : 1,

        force => exists $options->{force}
            ? $options->{force}
            : 0,

        label => $options->{label},

        metadata => ref($options->{metadata}) eq 'HASH'
            ? { %{ $options->{metadata} } }
            : {},
    };
}

sub _normalize_kind {
    my ($kind) = @_;

    $kind = ''
        unless defined $kind;

    return $KIND_FILE
        if $kind eq 'file';

    return $KIND_DIR
        if $kind eq 'dir'
            || $kind eq 'directory';

    return $KIND_CALLBACK
        if $kind eq 'callback'
            || $kind eq 'code';

    _error(
        'validate cleanup entry',
        "unsupported cleanup kind '$kind'",
    );
}

sub _path_exists {
    my ($path) = @_;

    return 1 if -e $path;
    return 1 if -l $path;

    return 0;
}

sub _entry_description {
    my ($entry) = @_;

    my $kind = $entry->{kind};

    if ($kind eq $KIND_CALLBACK) {
        return defined $entry->{label}
            && length $entry->{label}
            ? "callback:$entry->{label}"
            : 'callback';
    }

    return "$kind:$entry->{path}";
}

sub _record_error {
    my ($plan, $entry, $operation, $detail) = @_;

    my $error = {
        operation => $operation,
        detail    => $detail,
        entry     => $entry,
        kind      => $entry->{kind},
        path      => $entry->{path},
        label     => $entry->{label},
    };

    push @{ $plan->{errors} }, $error;

    return $error;
}

sub _remove_file {
    my ($entry) = @_;

    my $path = $entry->{path};

    if (!_path_exists($path)) {
        return {
            status => 'missing',
            path   => $path,
            kind   => $KIND_FILE,
        } if $entry->{ignore_missing};

        _error(
            'remove file',
            "path does not exist: '$path'",
        );
    }

    if (-d $path && !-l $path) {
        _error(
            'remove file',
            "path is a directory: '$path'",
        );
    }

    if ($entry->{force} && -e $path && !-w $path) {
        chmod 0600, $path
            or _error(
                'change file permissions',
                "'$path': $!",
            );
    }

    unlink $path
        or _error(
            'remove file',
            "'$path': $!",
        );

    return {
        status => 'removed',
        path   => $path,
        kind   => $KIND_FILE,
    };
}

sub _remove_directory {
    my ($entry) = @_;

    my $path = $entry->{path};

    if (!_path_exists($path)) {
        return {
            status => 'missing',
            path   => $path,
            kind   => $KIND_DIR,
        } if $entry->{ignore_missing};

        _error(
            'remove directory',
            "path does not exist: '$path'",
        );
    }

    if (!-d $path || -l $path) {
        _error(
            'remove directory',
            "path is not a directory: '$path'",
        );
    }

    if (!$entry->{recursive}) {
        rmdir $path
            or _error(
                'remove directory',
                "'$path': $!",
            );

        return {
            status => 'removed',
            path   => $path,
            kind   => $KIND_DIR,
        };
    }

    my $errors = [];

    remove_tree(
        $path,
        {
            error => \$errors,
            safe  => $entry->{force} ? 0 : 1,
        },
    );

    if (@{$errors}) {
        my @messages;

        for my $item (@{$errors}) {
            my ($failed_path, $message) = %{$item};

            $failed_path = $path
                unless defined $failed_path
                    && length $failed_path;

            $message = 'unknown removal error'
                unless defined $message
                    && length $message;

            push @messages, "$failed_path: $message";
        }

        _error(
            'remove directory tree',
            join('; ', @messages),
        );
    }

    return {
        status => 'removed',
        path   => $path,
        kind   => $KIND_DIR,
    };
}

sub _run_callback {
    my ($entry) = @_;

    my $callback = $entry->{callback};

    if (ref($callback) ne 'CODE') {
        _error(
            'run callback',
            'cleanup callback is not executable',
        );
    }

    my $result = $callback->(
        $entry->{metadata},
        $entry,
    );

    return {
        status => 'executed',
        kind   => $KIND_CALLBACK,
        label  => $entry->{label},
        result => $result,
    };
}

sub _execute_entry {
    my ($entry) = @_;

    return _remove_file($entry)
        if $entry->{kind} eq $KIND_FILE;

    return _remove_directory($entry)
        if $entry->{kind} eq $KIND_DIR;

    return _run_callback($entry)
        if $entry->{kind} eq $KIND_CALLBACK;

    _error(
        'execute cleanup entry',
        "unsupported entry kind '$entry->{kind}'",
    );
}

sub _clone_entry {
    my ($entry) = @_;

    my %copy = %{$entry};

    if (ref($entry->{metadata}) eq 'HASH') {
        $copy{metadata} = {
            %{ $entry->{metadata} },
        };
    }

    return \%copy;
}

# ---------------------------------------------------------------------------
# Plan construction
# ---------------------------------------------------------------------------

sub cleanup_plan {
    my ($options) = @_;

    $options = _normalize_options($options);

    return {
        version => 1,

        entries => [],
        errors  => [],

        order => $options->{order} // 'lifo',

        stop_on_error => exists $options->{stop_on_error}
            ? $options->{stop_on_error}
            : 0,

        retain_entries => exists $options->{retain_entries}
            ? $options->{retain_entries}
            : 0,

        metadata => ref($options->{metadata}) eq 'HASH'
            ? { %{ $options->{metadata} } }
            : {},
    };
}

sub cleanup_add {
    my ($plan, $kind, $target, $options) = @_;

    _validate_plan($plan);

    $kind = _normalize_kind($kind);
    $options = _normalize_entry_options($options);

    my $entry = {
        kind           => $kind,
        ignore_missing => $options->{ignore_missing},
        recursive      => $options->{recursive},
        force          => $options->{force},
        label          => $options->{label},
        metadata       => $options->{metadata},
    };

    if ($kind eq $KIND_CALLBACK) {
        if (ref($target) ne 'CODE') {
            _error(
                'add callback',
                'expected a code reference',
            );
        }

        $entry->{callback} = $target;
    }
    else {
        if (!defined $target || $target eq '') {
            _error(
                'add cleanup path',
                'path must not be empty',
            );
        }

        $entry->{path} = normalize_path($target);
    }

    push @{ $plan->{entries} }, $entry;

    return $entry;
}

sub cleanup_add_file {
    my ($plan, $path, $options) = @_;

    return cleanup_add(
        $plan,
        $KIND_FILE,
        $path,
        $options,
    );
}

sub cleanup_add_dir {
    my ($plan, $path, $options) = @_;

    return cleanup_add(
        $plan,
        $KIND_DIR,
        $path,
        $options,
    );
}

sub cleanup_add_callback {
    my ($plan, $callback, $options) = @_;

    return cleanup_add(
        $plan,
        $KIND_CALLBACK,
        $callback,
        $options,
    );
}

# ---------------------------------------------------------------------------
# Plan inspection
# ---------------------------------------------------------------------------

sub cleanup_size {
    my ($plan) = @_;

    _validate_plan($plan);

    return scalar @{ $plan->{entries} };
}

sub cleanup_is_empty {
    my ($plan) = @_;

    return cleanup_size($plan) == 0 ? 1 : 0;
}

sub cleanup_errors {
    my ($plan) = @_;

    _validate_plan($plan);

    return [
        map {
            {
                %{$_},
                entry => _clone_entry($_->{entry}),
            }
        } @{ $plan->{errors} }
    ];
}

sub cleanup_clear {
    my ($plan) = @_;

    _validate_plan($plan);

    my $count = scalar @{ $plan->{entries} };

    $plan->{entries} = [];
    $plan->{errors}  = [];

    return $count;
}

# ---------------------------------------------------------------------------
# Dry-run
# ---------------------------------------------------------------------------

sub cleanup_dry_run {
    my ($plan, $options) = @_;

    _validate_plan($plan);
    $options = _normalize_options($options);

    my $order = $options->{order}
        // $plan->{order}
        // 'lifo';

    my @entries = @{ $plan->{entries} };

    if ($order eq 'lifo') {
        @entries = reverse @entries;
    }
    elsif ($order ne 'fifo') {
        _error(
            'dry-run cleanup plan',
            "unsupported execution order '$order'",
        );
    }

    my @operations;

    for my $index (0 .. $#entries) {
        my $entry = $entries[$index];

        push @operations, {
            index       => $index,
            kind        => $entry->{kind},
            path        => $entry->{path},
            label       => $entry->{label},
            description => _entry_description($entry),
            exists      => defined $entry->{path}
                ? _path_exists($entry->{path})
                : undef,
            recursive   => $entry->{recursive},
            force       => $entry->{force},
            metadata    => {
                %{ $entry->{metadata} // {} },
            },
        };
    }

    return wantarray
        ? @operations
        : \@operations;
}

# ---------------------------------------------------------------------------
# Execution
# ---------------------------------------------------------------------------

sub cleanup_run {
    my ($plan, $options) = @_;

    _validate_plan($plan);
    $options = _normalize_options($options);

    my $order = $options->{order}
        // $plan->{order}
        // 'lifo';

    my $stop_on_error = exists $options->{stop_on_error}
        ? $options->{stop_on_error}
        : $plan->{stop_on_error};

    my $retain_entries = exists $options->{retain_entries}
        ? $options->{retain_entries}
        : $plan->{retain_entries};

    my @entries = @{ $plan->{entries} };

    if ($order eq 'lifo') {
        @entries = reverse @entries;
    }
    elsif ($order ne 'fifo') {
        _error(
            'run cleanup plan',
            "unsupported execution order '$order'",
        );
    }

    $plan->{errors} = [];

    my @results;
    my @remaining;

    for my $entry (@entries) {
        my $result;

        my $success = eval {
            $result = _execute_entry($entry);
            1;
        };

        if ($success) {
            push @results, {
                ok          => 1,
                entry       => _clone_entry($entry),
                description => _entry_description($entry),
                result      => $result,
            };

            push @remaining, $entry
                if $retain_entries;

            next;
        }

        my $error = $@ || 'unknown cleanup error';
        chomp $error;

        my $record = _record_error(
            $plan,
            $entry,
            'cleanup',
            $error,
        );

        push @results, {
            ok          => 0,
            entry       => _clone_entry($entry),
            description => _entry_description($entry),
            error       => $record,
        };

        push @remaining, $entry;

        last if $stop_on_error;
    }

    if (!$retain_entries) {
        if ($order eq 'lifo') {
            $plan->{entries} = [
                reverse @remaining,
            ];
        }
        else {
            $plan->{entries} = [
                @remaining,
            ];
        }
    }

    my $failed = scalar grep {
        !$_->{ok}
    } @results;

    my $succeeded = scalar @results - $failed;

    return {
        ok        => $failed == 0 ? 1 : 0,
        total     => scalar @results,
        succeeded => $succeeded,
        failed    => $failed,
        results   => \@results,
        errors    => cleanup_errors($plan),
        remaining => cleanup_size($plan),
    };
}

1;

__END__

=head1 NAME

Vitte::City::Athens::Cleanup - Structured cleanup plans for files, directories, and callbacks

=head1 VERSION

Version 0.1.0

=head1 SYNOPSIS

    use Vitte::City::Athens::Cleanup qw(
        cleanup_plan
        cleanup_add_file
        cleanup_add_dir
        cleanup_add_callback
        cleanup_dry_run
        cleanup_run
    );

    my $plan = cleanup_plan({
        order         => 'lifo',
        stop_on_error => 0,
    });

    cleanup_add_file(
        $plan,
        'build/output.tmp',
    );

    cleanup_add_dir(
        $plan,
        'build/work',
        {
            recursive => 1,
            force     => 1,
        },
    );

    cleanup_add_callback(
        $plan,
        sub {
            my ($metadata, $entry) = @_;
            print "custom cleanup\n";
            return 1;
        },
        {
            label => 'custom-finalizer',
        },
    );

    my $operations = cleanup_dry_run($plan);

    my $report = cleanup_run($plan);

    die "cleanup failed\n"
        unless $report->{ok};

=head1 DESCRIPTION

C<Vitte::City::Athens::Cleanup> provides explicit cleanup plans for temporary
files, temporary directories, generated build artifacts, and custom cleanup
callbacks.

Cleanup operations are registered before execution. They may then be inspected,
executed in first-in-first-out or last-in-first-out order, retried after partial
failure, or retained for repeated use.

The default order is last-in-first-out. This mirrors resource-stack behavior:
the most recently acquired resource is released first.

=head1 FUNCTIONS

=head2 cleanup_plan

    my $plan = cleanup_plan();
    my $plan = cleanup_plan(\%options);

Creates a cleanup plan.

Supported options:

=over 4

=item * C<order>

Execution order. Supported values are C<lifo> and C<fifo>. Defaults to C<lifo>.

=item * C<stop_on_error>

Stops execution after the first failure.

=item * C<retain_entries>

Keeps successful entries inside the plan after execution.

=item * C<metadata>

User-defined metadata associated with the plan.

=back

=head2 cleanup_add

    cleanup_add($plan, $kind, $target);
    cleanup_add($plan, $kind, $target, \%options);

Adds a generic cleanup entry.

Supported kinds:

=over 4

=item * C<file>

=item * C<directory>

=item * C<callback>

=back

=head2 cleanup_add_file

    cleanup_add_file($plan, $path);
    cleanup_add_file($plan, $path, \%options);

Adds a file or symbolic-link removal operation.

=head2 cleanup_add_dir

    cleanup_add_dir($plan, $path);
    cleanup_add_dir($plan, $path, \%options);

Adds a directory removal operation.

=head2 cleanup_add_callback

    cleanup_add_callback($plan, $callback);
    cleanup_add_callback($plan, $callback, \%options);

Adds a custom code reference to the cleanup plan.

The callback receives:

    $callback->($metadata, $entry);

=head2 cleanup_dry_run

    my $operations = cleanup_dry_run($plan);

Returns the operations in their actual execution order without modifying the
filesystem.

=head2 cleanup_run

    my $report = cleanup_run($plan);

Executes the cleanup plan and returns a structured report:

    {
        ok        => 1,
        total     => 3,
        succeeded => 3,
        failed    => 0,
        results   => [...],
        errors    => [],
        remaining => 0,
    }

=head2 cleanup_clear

    my $removed = cleanup_clear($plan);

Removes all entries and errors from the plan without touching the filesystem.

=head2 cleanup_size

    my $count = cleanup_size($plan);

Returns the number of registered entries.

=head2 cleanup_is_empty

    my $empty = cleanup_is_empty($plan);

Returns a boolean indicating whether the plan contains no entries.

=head2 cleanup_errors

    my $errors = cleanup_errors($plan);

Returns a copy of the recorded cleanup errors.

=head1 ENTRY OPTIONS

=over 4

=item * C<ignore_missing>

Treats absent paths as successful no-op operations. Enabled by default.

=item * C<recursive>

Allows recursive directory removal. Enabled by default.

=item * C<force>

Allows permission changes or less restrictive recursive deletion behavior.

=item * C<label>

Human-readable callback or operation label.

=item * C<metadata>

Arbitrary metadata associated with the cleanup entry.

=back

=head1 ORDERING

The default C<lifo> order is recommended for temporary resources:

    create directory
    create file
    register directory
    register file

Execution becomes:

    remove file
    remove directory

C<fifo> may be used when operations are independent and registration order must
be preserved.

=head1 ERROR MODEL

Cleanup failures are captured inside the plan and returned through the execution
report.

Each error includes:

=over 4

=item * failed operation;

=item * entry kind;

=item * affected path;

=item * optional label;

=item * operating-system error;

=item * original cleanup entry.

=back

When C<stop_on_error> is disabled, remaining operations continue after a failure.

=head1 SECURITY

Paths received from untrusted sources must be validated before registration.

Recursive cleanup operations can permanently remove directory trees. Callers
should restrict cleanup roots to project-controlled temporary or build
directories.

Symbolic links are removed as links and are not traversed as directories.

=head1 PORTABILITY

The module relies on standard Perl filesystem primitives and C<File::Path>.

Filesystem semantics may differ between Unix, macOS, Windows, network
filesystems, and mounted volumes.

=head1 AUTHOR

Vitte Project

=cut
