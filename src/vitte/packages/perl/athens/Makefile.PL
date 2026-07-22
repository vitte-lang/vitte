use strict;
use warnings;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME => 'Vitte::City::Athens',
    VERSION_FROM => 'lib/Vitte/City/Athens.pm',
    ABSTRACT => 'Portable filesystem, cleanup, manifest, permission, atomic IO, and structured error helpers',
    AUTHOR => 'Vitte Project',
    LICENSE => 'mit',
    MIN_PERL_VERSION => '5.010',
    PREREQ_PM => {
        'Exporter' => 0,
        'Fcntl' => 0,
        'File::Path' => 0,
        'IO::Handle' => 0,
        'POSIX' => 0,
        'bytes' => 0,
    },
    TEST_REQUIRES => {
        'File::Temp' => 0,
        'FindBin' => 0,
        'JSON::PP' => 0,
        'Test::More' => 0,
    },
);
