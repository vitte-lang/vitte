use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Lyon qw(ok err and_then map_result unwrap_or);

sub parse_number {
    my ($value) = @_;
    return $value =~ /\A-?[0-9]+\z/ ? ok(0 + $value) : err("not an integer: $value");
}

my $result = and_then(parse_number($ARGV[0] // '41'), sub {
    my ($value) = @_;
    return ok($value + 1);
});

print unwrap_or(map_result($result, sub { "value=$_[0]\n" }), "value=0\n");
