use strict;
use warnings;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME => 'Vitte::City::Berlin',
    VERSION_FROM => 'lib/Vitte/City/Berlin.pm',
    ABSTRACT => 'Byte buffers, binary packing, codecs, and checksums',
    AUTHOR => 'Vitte Project',
    LICENSE => 'mit',
    MIN_PERL_VERSION => '5.010',
    PREREQ_PM => {
        'Exporter' => 0,
    },
    TEST_REQUIRES => {
        'FindBin' => 0,
        'Test::More' => 0,
    },
);
