use strict;
use warnings;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME => 'Vitte::City::Cairo',
    VERSION_FROM => 'lib/Vitte/City/Cairo.pm',
    ABSTRACT => 'Structured scalar, record, list, table, schema, codec, and error helpers',
    AUTHOR => 'Vitte Project',
    LICENSE => 'mit',
    MIN_PERL_VERSION => '5.010',
    PREREQ_PM => {
        'Digest::SHA' => 0,
        'Exporter' => 0,
        'JSON::PP' => 0,
    },
    TEST_REQUIRES => {
        'FindBin' => 0,
        'Test::More' => 0,
    },
);
