package Vitte::Crates::Thiserror;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(error_new error_message error_code error_with_source);

sub error_new { my ($code,$message)=@_; return { code=>$code, message=>$message, source=>undef }; }
sub error_message { $_[0]->{message} }
sub error_code { $_[0]->{code} }
sub error_with_source { my ($code,$message,$source)=@_; return { code=>$code, message=>$message, source=>$source }; }

1;
