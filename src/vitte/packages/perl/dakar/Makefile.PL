use strict;
use warnings;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME => 'Vitte::City::Dakar',
    VERSION_FROM => 'lib/Vitte/City/Dakar.pm',
    ABSTRACT => 'Structured logging, filtering, formatting, sinks, metrics, tracing, and error helpers',
    AUTHOR => 'Vitte Project',
    LICENSE => 'mit',
    MIN_PERL_VERSION => '5.010',
    PREREQ_PM => {
        'Exporter' => 0,
        'JSON::PP' => 0,
    },
    TEST_REQUIRES => {
        'FindBin' => 0,
        'Test::More' => 0,
    },
);
