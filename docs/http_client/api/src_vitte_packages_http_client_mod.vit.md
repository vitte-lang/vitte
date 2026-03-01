# src/vitte/packages/http_client/mod.vit

## usage
- use http_client/mod.vit as *_pkg

## contre-exemple
- avoid insecure redirects/tls

## symbols
pick ClientMethod {
pick ClientError {
pick ClientResult[T] {
form ClientHeader {
form ClientHeaders {
form RetryPolicy {
form CircuitState {
form ClientPool {
form ClientConfig {
form ClientRequest {
form ClientResponse {
form ClientState {
proc ok[T](value: T) -> ClientResult[T] {
proc err[T](code: string, message: string) -> ClientResult[T] {
proc assert_or_err(condition: bool, code: string, message: string) -> ClientResult[bool] {
proc panic_guard[T](r: ClientResult[T], fallback: T) -> T {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc method_name(m: ClientMethod) -> string {
proc default_headers() -> ClientHeaders {
proc header_add(headers: ClientHeaders, key: string, value: string) -> ClientHeaders {
proc header_get(headers: ClientHeaders, key: string) -> string {
proc default_retry() -> RetryPolicy {
proc default_config() -> ClientConfig {
proc default_state() -> ClientState {
proc request(method: ClientMethod, url: string, body: string) -> ClientRequest {
proc request_with_header(req: ClientRequest, key: string, value: string) -> ClientRequest {
proc request_with_body(req: ClientRequest, body: string) -> ClientRequest {
proc request_with_query(req: ClientRequest, query: string) -> ClientRequest {
proc classify_error(resp: ClientResponse) -> ClientError {
proc validate_request(state: ClientState, req: ClientRequest) -> ClientResult[bool] {
proc pool_checkout(state: ClientState) -> ClientResult[ClientState] {
proc pool_release(state: ClientState) -> ClientState {
proc circuit_check(state: ClientState) -> ClientResult[bool] {
proc is_idempotent(method: ClientMethod) -> bool {
proc response(status: int, body: string, elapsed_ms: int) -> ClientResponse {
proc perform_once(state: ClientState, req: ClientRequest) -> ClientResult[ClientResponse] {
proc should_retry(state: ClientState, req: ClientRequest, resp: ClientResponse, attempt: int) -> bool {
proc perform(state: ClientState, req: ClientRequest) -> ClientResult[ClientResponse] {
proc observe(state: ClientState, elapsed_ms: int, error_happened: bool) -> ClientState {
proc security_redirect_allowed(state: ClientState, from_url: string, to_url: string) -> bool {
proc structured_log(req: ClientRequest, resp: ClientResponse) -> string {
proc analyze_report_json_stub() -> string {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
