use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Athens qw(read_text temp_file);

my $path = temp_file('/tmp', 'athens-example', '.txt', "hello\n");
print read_text($path);
