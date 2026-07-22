package Vitte::City::Lyon;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Lyon::Context qw(
    context context_chain context_delete context_get context_has context_keys
    context_merge context_push context_set context_to_hash context_values
);
use Vitte::City::Lyon::Error qw(
    error_cause error_code error_context error_is error_message error_path
    error_to_hash error_to_string error_with_cause error_with_context lyon_error
);
use Vitte::City::Lyon::Option qw(
    is_none is_some none option_and_then option_filter option_from_value
    option_map option_match option_or option_or_else option_to_list
    option_to_result option_value some unwrap_option
);
use Vitte::City::Lyon::Recovery qw(
    all_ok attempt collect_errors collect_results combine_results fallback
    first_err first_ok partition_results recover retry
);
use Vitte::City::Lyon::Report qw(
    report report_add report_count report_errors report_ok report_summary
    report_to_text report_values
);
use Vitte::City::Lyon::Result qw(
    and_then err expect expect_err is_err is_ok is_result map_error map_result
    ok or_else result result_error result_flatten result_from_eval
    result_from_option result_match result_to_bool result_to_list result_value
    unwrap unwrap_err unwrap_or unwrap_or_else
);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    ok err result is_ok is_err is_result unwrap unwrap_err unwrap_or
    unwrap_or_else expect expect_err map_result map_error and_then or_else
    result_match result_value result_error result_to_bool result_to_list
    result_from_option result_from_eval result_flatten some none is_some is_none
    option_value unwrap_option option_or option_or_else option_map
    option_and_then option_filter option_match option_to_list option_to_result
    option_from_value lyon_error error_code error_message error_context error_path
    error_cause error_with_context error_with_cause error_to_hash error_to_string
    error_is context context_get context_set context_has context_delete
    context_keys context_values context_merge context_push context_chain
    context_to_hash attempt recover retry fallback collect_results collect_errors
    partition_results combine_results first_ok first_err all_ok report report_add
    report_count report_ok report_values report_errors report_summary report_to_text
);

1;
