# src/vitte/packages/http/mod.vit

## usage
- use http/mod.vit as *_pkg

## contre-exemple
- avoid CRLF/header injection

## symbols
pick HttpMethod {
form HttpError {
pick HttpResult[T] {
form HttpHeader {
form HttpHeaders {
form HttpRequest {
form HttpResponse {
form HttpRoute {
form HttpMiddleware {
form HttpServerConfig {
form HttpServerState {
proc error(code: string, message: string, context: string) -> HttpError {
proc ok[T](value: T) -> HttpResult[T] {
proc err[T](code: string, message: string) -> HttpResult[T] {
proc err_ctx[T](code: string, message: string, context: string) -> HttpResult[T] {
proc assert_or_err(condition: bool, code: string, message: string) -> HttpResult[bool] {
proc panic_guard[T](r: HttpResult[T], fallback: T) -> T {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc method_name(m: HttpMethod) -> string {
proc method_from_name(name: string) -> HttpResult[HttpMethod] {
proc default_headers() -> HttpHeaders {
proc header_add(headers: HttpHeaders, key: string, value: string) -> HttpHeaders {
proc header_get(headers: HttpHeaders, key: string) -> string {
proc request(method: HttpMethod, path: string, body: string) -> HttpRequest {
proc request_with_query(req: HttpRequest, query: string) -> HttpRequest {
proc request_with_header(req: HttpRequest, key: string, value: string) -> HttpRequest {
proc request_with_chunked(req: HttpRequest, enabled: bool) -> HttpRequest {
proc request_with_cancel(req: HttpRequest, token: string) -> HttpRequest {
proc response(status: int, headers: HttpHeaders, body: string, keep_alive: bool) -> HttpResponse {
proc parse_request_line(line: string) -> HttpResult[HttpRequest] {
proc validate_request(state: HttpServerState, req: HttpRequest) -> HttpResult[bool] {
proc validate_response(resp: HttpResponse) -> HttpResult[bool] {
proc route(method: HttpMethod, pattern: string, target: string, priority: int, secure: bool, body_limit: int) -> HttpRoute {
proc route_conflict(routes: [HttpRoute], candidate: HttpRoute) -> bool {
proc default_server_config() -> HttpServerConfig {
proc default_server_state() -> HttpServerState {
proc add_route(state: HttpServerState, r: HttpRoute) -> HttpResult[HttpServerState] {
proc add_middleware(state: HttpServerState, name: string) -> HttpServerState {
proc apply_middleware(state: HttpServerState, req: HttpRequest) -> HttpResult[HttpRequest] {
proc with_timeouts(state: HttpServerState, read_ms: int, write_ms: int, idle_ms: int, header_ms: int, body_ms: int) -> HttpServerState {
proc with_rate_limit(state: HttpServerState, per_window: int, burst: int) -> HttpServerState {
proc with_cors(state: HttpServerState, allow_origin: string, preflight_s: int) -> HttpServerState {
proc with_csrf(state: HttpServerState, enabled: bool) -> HttpServerState {
proc find_route(routes: [HttpRoute], req: HttpRequest) -> HttpResult[HttpRoute] {
proc apply_security_gates(state: HttpServerState, req: HttpRequest, matched: HttpRoute) -> HttpResult[bool] {
proc apply_rate_limit(state: HttpServerState) -> HttpResult[HttpServerState] {
proc handle(state: HttpServerState, req: HttpRequest) -> HttpResult[HttpResponse] {
proc openapi_contract_snapshot(routes: [HttpRoute]) -> [string] {
proc openapi_contract_diff(old_snapshot: [string], new_snapshot: [string]) -> HttpResult[bool] {
proc analyze_report_json_stub() -> string {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
