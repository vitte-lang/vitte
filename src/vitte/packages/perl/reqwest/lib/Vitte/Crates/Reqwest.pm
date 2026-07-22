package Vitte::Crates::Reqwest;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(request get post header);

sub request { my (%args)=@_; return { method=>$args{method}||'GET', url=>$args{url}||'', headers=>$args{headers}||{}, body=>$args{body}||'' }; }
sub get { request(method=>'GET', url=>$_[0]) }
sub post { request(method=>'POST', url=>$_[0], body=>$_[1]||'') }
sub header { my ($req,$name,$value)=@_; $req->{headers}{$name}=$value; return $req; }

1;
