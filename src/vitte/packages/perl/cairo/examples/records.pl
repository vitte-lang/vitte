use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Cairo qw(decode_record encode_record record_merge);

my $text = encode_record(record_merge({ name => 'cairo' }, { version => '0.1.0' }));
my $record = decode_record($text);
print "$record->{name} $record->{version}\n";
