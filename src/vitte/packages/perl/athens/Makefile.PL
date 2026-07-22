use strict;
use warnings;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME => 'Vitte::City::Athens',
    VERSION_FROM => 'lib/Vitte/City/Athens.pm',
    ABSTRACT => 'Temporary paths, cleanup manifests, and atomic file helpers',
    AUTHOR => 'Vitte Project',
    LICENSE => 'mit',
    MIN_PERL_VERSION => '5.010',
    PREREQ_PM => {
        'Exporter' => 0,
    },
    TEST_REQUIRES => {
        'File::Temp' => 0,
        'FindBin' => 0,
        'Test::More' => 0,
    },
);
