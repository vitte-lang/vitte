package Vitte::City::Athens::Atomic;

use strict;
use warnings;
use bytes ();

use Exporter 'import';
use Fcntl qw(
    O_RDONLY
    O_WRONLY
    O_CREAT
    O_EXCL
    O_TRUNC
);
use IO::Handle ();
use POSIX qw();

use Vitte::City::Athens::Path qw(
    basename
    dirname
    join_path
);
use Vitte::City::Athens::Permissions qw(
    ensure_dir
);

our $VERSION = '0.1.0';

our @EXPORT_OK = qw(
    atomic_write
    atomic_write_bytes
    read_text
    read_bytes
    write_text
    write_bytes
);

my $TEMP_COUNTER = 0;

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

sub _error {
    my ($operation, $path, $detail) = @_;

    $operation = 'operation' unless defined $operation && length $operation;
    $path      = '<unknown>' unless defined $path && length $path;
    $detail    = "$!"        unless defined $detail && length $detail;

    die "Athens::Atomic: $operation '$path': $detail\n";
}

sub _normalize_options {
    my ($options) = @_;

    return {} unless defined $options;

    if (ref($options) ne 'HASH') {
        _error(
            'options',
            '<arguments>',
            'expected a hash reference',
        );
    }

    return $options;
}

sub _byte_length {
    my ($content) = @_;
    return bytes::length(defined $content ? $content : '');
}

sub _parent_directory {
    my ($path) = @_;

    if (!defined $path || $path eq '') {
        _error('validate path', '<empty>', 'path must not be empty');
    }

    my $directory = dirname($path);

    return '.'
        unless defined $directory
            && length $directory;

    return $directory;
}

sub _ensure_parent_directory {
    my ($path) = @_;

    my $directory = _parent_directory($path);

    eval {
        ensure_dir($directory);
        1;
    } or do {
        my $error = $@ || 'unable to create parent directory';
        chomp $error;
        _error('create parent directory', $directory, $error);
    };

    return $directory;
}

sub _open_for_read {
    my ($path) = @_;

    my $handle;

    sysopen($handle, $path, O_RDONLY)
        or _error('open for reading', $path);

    binmode($handle)
        or _error('enable binary mode', $path);

    return $handle;
}

sub _open_for_write {
    my ($path, $mode) = @_;

    $mode = 0666 unless defined $mode;

    my $handle;

    sysopen(
        $handle,
        $path,
        O_WRONLY | O_CREAT | O_TRUNC,
        $mode,
    ) or _error('open for writing', $path);

    binmode($handle)
        or _error('enable binary mode', $path);

    return $handle;
}

sub _write_all {
    my ($handle, $path, $content) = @_;

    $content = '' unless defined $content;

    my $length = _byte_length($content);
    my $offset = 0;

    while ($offset < $length) {
        my $written = syswrite(
            $handle,
            $content,
            $length - $offset,
            $offset,
        );

        if (!defined $written) {
            next if $!{EINTR};
            _error('write', $path);
        }

        if ($written == 0) {
            _error(
                'write',
                $path,
                'zero bytes written before completion',
            );
        }

        $offset += $written;
    }

    return $offset;
}

sub _flush {
    my ($handle, $path) = @_;

    $handle->flush()
        or _error('flush', $path);

    return 1;
}

sub _sync_file {
    my ($handle, $path) = @_;

    _flush($handle, $path);

    my $descriptor = fileno($handle);

    if (!defined $descriptor) {
        _error('synchronize', $path, 'invalid file descriptor');
    }

    my $result = eval {
        POSIX::fsync($descriptor);
    };

    if (!defined $result || $result != 0) {
        my $detail = $@ || "$!";
        chomp $detail;
        _error('synchronize', $path, $detail);
    }

    return 1;
}

sub _close {
    my ($handle, $path) = @_;

    close($handle)
        or _error('close', $path);

    return 1;
}

sub _temporary_path {
    my ($directory, $target_name) = @_;

    $TEMP_COUNTER++;

    my $timestamp = time();
    my $pid       = $$;

    $target_name = 'file'
        unless defined $target_name
            && length $target_name;

    return join_path(
        $directory,
        sprintf(
            '.athens-%s-%d-%d-%d.tmp',
            $target_name,
            $pid,
            $timestamp,
            $TEMP_COUNTER,
        ),
    );
}

sub _create_temporary_file {
    my ($directory, $target_name, $mode) = @_;

    $mode = 0600 unless defined $mode;

    for my $attempt (1 .. 128) {
        my $temporary_path = _temporary_path(
            $directory,
            $target_name,
        );

        my $handle;

        if (
            sysopen(
                $handle,
                $temporary_path,
                O_WRONLY | O_CREAT | O_EXCL,
                $mode,
            )
        ) {
            binmode($handle)
                or do {
                    my $error = "$!";
                    close($handle);
                    unlink($temporary_path);
                    _error(
                        'enable binary mode',
                        $temporary_path,
                        $error,
                    );
                };

            return ($handle, $temporary_path);
        }

        next if $!{EEXIST};

        _error(
            'create temporary file',
            $temporary_path,
        );
    }

    _error(
        'create temporary file',
        $directory,
        'temporary-name collision limit reached',
    );
}

sub _existing_mode {
    my ($path) = @_;

    my @metadata = stat($path);

    return undef unless @metadata;

    return $metadata[2] & 07777;
}

sub _apply_mode {
    my ($path, $mode) = @_;

    return 1 unless defined $mode;

    chmod($mode, $path)
        or _error(
            'set permissions',
            $path,
            sprintf('unable to apply mode %04o: %s', $mode, $!),
        );

    return 1;
}

sub _decode_text {
    my ($content, $encoding, $path) = @_;

    return $content
        unless defined $encoding
            && length $encoding;

    require Encode;

    my $decoded = eval {
        Encode::decode(
            $encoding,
            $content,
            Encode::FB_CROAK(),
        );
    };

    if ($@) {
        my $error = $@;
        chomp $error;
        _error('decode text', $path, $error);
    }

    return $decoded;
}

sub _encode_text {
    my ($content, $encoding, $path) = @_;

    $content = '' unless defined $content;

    return $content
        unless defined $encoding
            && length $encoding;

    require Encode;

    my $encoded = eval {
        Encode::encode(
            $encoding,
            $content,
            Encode::FB_CROAK(),
        );
    };

    if ($@) {
        my $error = $@;
        chomp $error;
        _error('encode text', $path, $error);
    }

    return $encoded;
}

# ---------------------------------------------------------------------------
# Reading
# ---------------------------------------------------------------------------

sub read_bytes {
    my ($path, $options) = @_;

    $options = _normalize_options($options);

    my $maximum_size = $options->{max_bytes};
    my $chunk_size   = $options->{chunk_size} // 64 * 1024;

    if ($chunk_size <= 0) {
        _error(
            'read options',
            $path,
            'chunk_size must be greater than zero',
        );
    }

    my $handle = _open_for_read($path);
    my $content = '';

    while (1) {
        my $chunk = '';
        my $read  = sysread($handle, $chunk, $chunk_size);

        if (!defined $read) {
            next if $!{EINTR};

            my $error = "$!";
            close($handle);
            _error('read', $path, $error);
        }

        last if $read == 0;

        $content .= $chunk;

        if (
            defined $maximum_size
            && _byte_length($content) > $maximum_size
        ) {
            close($handle);

            _error(
                'read',
                $path,
                "file exceeds maximum size of $maximum_size bytes",
            );
        }
    }

    _close($handle, $path);

    return $content;
}

sub read_text {
    my ($path, $options) = @_;

    $options = _normalize_options($options);

    my $content = read_bytes($path, $options);
    my $encoding = $options->{encoding} // 'UTF-8';

    return _decode_text(
        $content,
        $encoding,
        $path,
    );
}

# ---------------------------------------------------------------------------
# Direct writing
# ---------------------------------------------------------------------------

sub write_bytes {
    my ($path, $content, $options) = @_;

    $options = _normalize_options($options);
    $content = '' unless defined $content;

    _ensure_parent_directory($path);

    my $mode   = $options->{mode} // 0666;
    my $handle = _open_for_write($path, $mode);

    my $written;

    eval {
        $written = _write_all(
            $handle,
            $path,
            $content,
        );

        if ($options->{sync}) {
            _sync_file($handle, $path);
        }
        else {
            _flush($handle, $path);
        }

        _close($handle, $path);
        1;
    } or do {
        my $error = $@ || 'unknown write error';

        close($handle);

        chomp $error;
        die "$error\n";
    };

    if (defined $options->{mode}) {
        _apply_mode($path, $options->{mode});
    }

    return $written;
}

sub write_text {
    my ($path, $content, $options) = @_;

    $options = _normalize_options($options);

    my $encoding = $options->{encoding} // 'UTF-8';
    my $encoded = _encode_text(
        $content,
        $encoding,
        $path,
    );

    return write_bytes(
        $path,
        $encoded,
        $options,
    );
}

# ---------------------------------------------------------------------------
# Atomic writing
# ---------------------------------------------------------------------------

sub atomic_write_bytes {
    my ($path, $content, $options) = @_;

    $options = _normalize_options($options);
    $content = '' unless defined $content;

    my $directory   = _ensure_parent_directory($path);
    my $target_name = basename($path);

    my $existing_mode = $options->{preserve_mode}
        ? _existing_mode($path)
        : undef;

    my $requested_mode = defined $options->{mode}
        ? $options->{mode}
        : defined $existing_mode
            ? $existing_mode
            : 0666;

    my ($handle, $temporary_path) = _create_temporary_file(
        $directory,
        $target_name,
        0600,
    );

    my $written;
    my $completed = 0;

    eval {
        $written = _write_all(
            $handle,
            $temporary_path,
            $content,
        );

        if ($options->{sync}) {
            _sync_file(
                $handle,
                $temporary_path,
            );
        }
        else {
            _flush(
                $handle,
                $temporary_path,
            );
        }

        _close(
            $handle,
            $temporary_path,
        );

        _apply_mode(
            $temporary_path,
            $requested_mode,
        );

        if ($options->{create_new} && -e $path) {
            _error(
                'atomic replace',
                $path,
                'target already exists',
            );
        }

        rename($temporary_path, $path)
            or _error(
                'atomic rename',
                $path,
                "cannot rename '$temporary_path': $!",
            );

        $completed = 1;
        1;
    } or do {
        my $error = $@ || 'unknown atomic-write error';

        close($handle)
            if defined fileno($handle);

        if (!$completed && -e $temporary_path) {
            unlink($temporary_path);
        }

        chomp $error;
        die "$error\n";
    };

    return $written;
}

sub atomic_write {
    my ($path, $content, $options) = @_;

    $options = _normalize_options($options);

    my $encoding = $options->{encoding} // 'UTF-8';
    my $encoded = _encode_text(
        $content,
        $encoding,
        $path,
    );

    return atomic_write_bytes(
        $path,
        $encoded,
        $options,
    );
}

1;

__END__

=head1 NAME

Vitte::City::Athens::Atomic - Safe and atomic filesystem IO

=head1 VERSION

Version 0.1.0

=head1 SYNOPSIS

    use Vitte::City::Athens::Atomic qw(
        atomic_write
        read_text
        write_text
    );

    write_text(
        'build/output.txt',
        "direct write\n",
    );

    atomic_write(
        'build/config.txt',
        "stable content\n",
        {
            encoding      => 'UTF-8',
            mode          => 0644,
            preserve_mode => 1,
            sync          => 1,
        },
    );

    my $content = read_text(
        'build/config.txt',
        {
            encoding  => 'UTF-8',
            max_bytes => 1024 * 1024,
        },
    );

=head1 DESCRIPTION

This module provides direct and atomic filesystem operations for the Athens
package.

Atomic writes create a temporary file in the destination directory, write and
optionally synchronize its contents, apply the requested permissions, and then
replace the destination through a same-filesystem rename.

The destination is therefore never exposed as a partially written file.

=head1 FUNCTIONS

=head2 read_text

    my $content = read_text($path);
    my $content = read_text($path, \%options);

Reads and decodes a text file.

Supported options:

=over 4

=item * C<encoding>

Character encoding. Defaults to C<UTF-8>.

=item * C<max_bytes>

Maximum accepted file size.

=item * C<chunk_size>

Read-buffer size. Defaults to 65536 bytes.

=back

=head2 read_bytes

    my $bytes = read_bytes($path);
    my $bytes = read_bytes($path, \%options);

Reads a file without text decoding.

=head2 write_text

    my $written = write_text($path, $content);
    my $written = write_text($path, $content, \%options);

Writes text directly to the destination.

=head2 write_bytes

    my $written = write_bytes($path, $bytes);
    my $written = write_bytes($path, $bytes, \%options);

Writes raw bytes directly to the destination.

=head2 atomic_write

    my $written = atomic_write($path, $content);
    my $written = atomic_write($path, $content, \%options);

Writes encoded text through a temporary file and atomically replaces the
destination.

=head2 atomic_write_bytes

    my $written = atomic_write_bytes($path, $bytes);
    my $written = atomic_write_bytes($path, $bytes, \%options);

Performs an atomic raw-byte write.

=head1 WRITE OPTIONS

=over 4

=item * C<encoding>

Text encoding. Defaults to C<UTF-8>.

=item * C<mode>

Permissions applied to the resulting file.

=item * C<preserve_mode>

Preserves the existing destination permissions when possible.

=item * C<sync>

Flushes and synchronizes the temporary file before replacement.

=item * C<create_new>

Rejects the operation when the destination already exists.

=back

=head1 ATOMICITY

The temporary file is created in the same directory as the destination. This
allows the final rename to remain on the same filesystem.

The rename protects readers from observing partially written content. It does
not provide multi-file transactions or distributed locking.

=head1 SECURITY

Temporary files are created with exclusive creation and restrictive initial
permissions.

The module never follows a predictable fixed temporary filename without using
C<O_EXCL>.

Callers must still validate untrusted paths and prevent writes outside their
authorized directory roots.

=head1 ERRORS

Filesystem failures raise exceptions containing:

=over 4

=item * the Athens module name;

=item * the failed operation;

=item * the affected path;

=item * the operating-system error.

=back

=head1 AUTHOR

Vitte Project

=cut
