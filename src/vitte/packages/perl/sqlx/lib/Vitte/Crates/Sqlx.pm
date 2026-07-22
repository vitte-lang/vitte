package Vitte::Crates::Sqlx;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(query bind to_sql row);

sub query { my ($sql)=@_; return { sql=>$sql, binds=>[] }; }
sub bind { my ($query,$value)=@_; push @{$query->{binds}}, $value; return $query; }
sub to_sql { my ($query)=@_; return $query->{sql}; }
sub row { my (%cols)=@_; return \%cols; }

1;
