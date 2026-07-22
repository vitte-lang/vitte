package Vitte::Crates::Tokio;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(task runtime spawn block_on);

sub runtime { return { tasks => [] }; }
sub task { my ($name,$code)=@_; return { name=>$name, code=>$code }; }
sub spawn { my ($runtime,$task)=@_; push @{$runtime->{tasks}}, $task; return scalar @{$runtime->{tasks}}; }
sub block_on { my ($task)=@_; return $task->{code}->(); }

1;
