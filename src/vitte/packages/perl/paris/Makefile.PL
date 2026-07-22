use strict;
use warnings;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME             => 'Vitte::City::Paris',
    VERSION_FROM     => 'lib/Vitte/City/Paris.pm',
    ABSTRACT         => 'Vitte deterministic command-line parser toolkit',
    AUTHOR           => 'Vitte contributors',
    MIN_PERL_VERSION => '5.010',
    PREREQ_PM        => {
        'Exporter' => 0,
    },
    TEST_REQUIRES    => {
        'FindBin'    => 0,
        'JSON::PP'   => 0,
        'Test::More' => 0,
    },
);
