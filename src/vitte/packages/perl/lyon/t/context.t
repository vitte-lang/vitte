use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon qw(
    context context_get context_set context_has context_delete context_keys
    context_values context_merge context_push context_chain context_to_hash
);

my $ctx = context(target => 'native', phase => 'parse');
Test::More::is(context_get($ctx, 'target'), 'native', 'context_get existing key');
Test::More::is(context_get($ctx, 'missing', 'fallback'), 'fallback', 'context_get default');
Test::More::ok(context_has($ctx, 'phase'), 'context_has');

context_set($ctx, file => 'main.vit');
Test::More::is(context_get($ctx, 'file'), 'main.vit', 'context_set');
Test::More::is(context_delete($ctx, 'phase'), 'parse', 'context_delete returns removed value');
Test::More::is_deeply(context_keys($ctx), [qw(file target)], 'context_keys are stable');
Test::More::is_deeply(context_values($ctx), [qw(main.vit native)], 'context_values follows sorted keys');

my $extra = context(target => 'wasm', profile => 'debug');
context_push($extra, 'typeck', code => 'LYON_E_TYPE');
my $merged = context_merge($ctx, $extra);
Test::More::is(context_get($merged, 'target'), 'wasm', 'context_merge last context wins');
Test::More::is(context_get($merged, 'profile'), 'debug', 'context_merge carries new values');
Test::More::is(context_chain($merged)->[0]{label}, 'typeck', 'context_merge carries chain');
Test::More::is_deeply(context_to_hash($merged)->{values}{profile}, 'debug', 'context_to_hash clones values');

Test::More::done_testing();
