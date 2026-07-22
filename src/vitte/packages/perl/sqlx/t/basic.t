use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Sqlx qw(query bind to_sql row);

Test::More::is($Vitte::Crates::Sqlx::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { to_sql(bind(query("select ?"),1)) eq "select ?" }, 'basic behavior works');
