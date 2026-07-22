use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Serde qw(serialize_value deserialize_value is_serialized);

Test::More::is($Vitte::Crates::Serde::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { serialize_value({name => "vitte"}) =~ /name/ && deserialize_value("42") == 42 }, 'basic behavior works');
