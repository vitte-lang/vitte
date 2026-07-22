use strict;
use warnings;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME          => 'Vitte::City::Lyon',
    VERSION_FROM  => 'lib/Vitte/City/Lyon.pm',
    ABSTRACT      => 'Vitte result, option, error, recovery, and report toolkit',
    AUTHOR        => 'Vitte contributors',
    LICENSE       => 'open_source',
    MIN_PERL_VERSION => '5.010',
    PREREQ_PM     => {
        'Exporter' => 0,
    },
    TEST_REQUIRES => {
        'FindBin'    => 0,
        'JSON::PP'   => 0,
        'Test::More' => 0,
    },
);
