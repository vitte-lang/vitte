# src/vitte/packages/db/mod.vit

## usage
- import with: use db/mod.vit as *_pkg

## contre-exemple
- avoid unprepared query paths

## symbols
pick DbError {
pick DbResult[T] {
pick DbBindValue {
form DbConfig {
form DbConn {
form DbTx {
form DbQuery {
form DbStmt {
form DbRow {
form DbRows {
form MigrationState {
proc ok[T](value: T) -> DbResult[T] {
proc err[T](code: string, message: string) -> DbResult[T] {
proc assert_or_err(condition: bool, code: string, message: string) -> DbResult[bool] {
proc panic_guard[T](r: DbResult[T], fallback: T) -> T {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc default_config() -> DbConfig {
proc connect(config: DbConfig) -> DbResult[DbConn] {
proc ping(conn: DbConn) -> DbResult[bool] {
proc close(conn: DbConn) -> DbConn {
proc query(sql: string) -> DbQuery {
proc query_with_timeout(q: DbQuery, timeout_ms: int) -> DbQuery {
proc query_with_cancel(q: DbQuery, token: string) -> DbQuery {
proc prepare(sql: string) -> DbStmt {
proc bind_int(stmt: DbStmt, value: int) -> DbStmt {
proc bind_string(stmt: DbStmt, value: string) -> DbStmt {
proc bind_bool(stmt: DbStmt, value: bool) -> DbStmt {
proc bind_time(stmt: DbStmt, value: int) -> DbStmt {
proc bind_bytes(stmt: DbStmt, value: [int]) -> DbStmt {
proc bind_null(stmt: DbStmt) -> DbStmt {
proc is_select(sql: string) -> bool {
proc is_mutation(sql: string) -> bool {
proc execute_prepared(conn: DbConn, stmt: DbStmt, timeout_ms: int, cancel_token: string) -> DbResult[DbRows] {
proc query_one(conn: DbConn, stmt: DbStmt, timeout_ms: int, cancel_token: string) -> DbResult[DbRow] {
proc query_many(conn: DbConn, stmt: DbStmt, timeout_ms: int, cancel_token: string) -> DbResult[DbRows] {
proc exec(conn: DbConn, stmt: DbStmt, timeout_ms: int, cancel_token: string) -> DbResult[int] {
proc begin(conn: DbConn) -> DbResult[DbTx] {
proc commit(tx: DbTx) -> DbResult[DbTx] {
proc rollback(tx: DbTx) -> DbResult[DbTx] {
proc map_row_nullable(row: DbRow, index: int) -> DbResult[DbBindValue] {
proc migration_lock(state: MigrationState) -> DbResult[MigrationState] {
proc migration_unlock(state: MigrationState) -> MigrationState {
proc migration_apply(state: MigrationState, next_version: int, checksum: string) -> DbResult[MigrationState] {
proc migration_compat_check(old_state: MigrationState, new_state: MigrationState) -> DbResult[bool] {
proc structured_query_log(sql: string, binds: [DbBindValue], latency_ms: int, ok: bool) -> string {
proc observe(conn: DbConn, latency_ms: int, error_happened: bool, retry_happened: bool) -> DbConn {
proc pool_checkout(conn: DbConn) -> DbResult[DbConn] {
proc pool_release(conn: DbConn) -> DbConn {
proc analyze_report_json_stub() -> string {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
