package Vitte::Crates::Axum;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(route router dispatch response);

sub response { my ($status,$body)=@_; return { status=>$status, body=>$body }; }
sub router { return { routes => {} }; }
sub route { my ($router,$method,$path,$handler)=@_; $router->{routes}{uc($method).' '.$path}=$handler; return $router; }
sub dispatch { my ($router,$method,$path)=@_; my $h=$router->{routes}{uc($method).' '.$path}; return $h ? $h->() : response(404,'not found'); }

1;
