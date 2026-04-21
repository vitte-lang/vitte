#pragma once
#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace vitte::runtime {

/* -------------------------------------------------
 * Memory management
 * ------------------------------------------------- */

/* Allocate `size` bytes or panic on failure */
void* alloc(std::size_t size);

/* Free previously allocated memory */
void dealloc(void* ptr);

/* -------------------------------------------------
 * Panic / error handling
 * ------------------------------------------------- */

/* Abort execution with an error message */
[[noreturn]]
void panic(const char* msg);

/* Runtime assertion helper (used by lowered MIR) */
void assert_true(bool cond, const char* msg);

/* Simple printing helpers (temporary) */
void print_i32(int32_t value);

/* -------------------------------------------------
 * Control flow helpers
 * ------------------------------------------------- */

/* Mark unreachable code paths */
[[noreturn]]
void unreachable();

/* Termination hook (optional ABI boundary) */
void terminate();

} // namespace vitte::runtime

/* -------------------------------------------------
 * C ABI for stdlib bindings (experimental)
 * ------------------------------------------------- */

#define VITTE_C_ABI_VERSION_MAJOR 1
#define VITTE_C_ABI_VERSION_MINOR 0
#define VITTE_C_ABI_VERSION_PATCH 0

struct VitteString {
    const char* data;
    std::size_t len;

    std::int64_t operator[](std::size_t idx) const;
    operator std::int32_t() const;
    operator std::int64_t() const;
};

bool operator==(VitteString a, VitteString b);
bool operator!=(VitteString a, VitteString b);
VitteString operator+(VitteString a, VitteString b);

template <typename T>
struct VitteSlice {
    T* data;
    std::size_t len;

    T& operator[](std::size_t idx) {
        return data[idx];
    }

    const T& operator[](std::size_t idx) const {
        return data[idx];
    }

    VitteSlice<T> push(T value) const {
        return vitte_slice_push<T>(*this, value);
    }
};

template <typename A, typename B>
struct VittePair {
    A first;
    B second;

    A& operator[](std::size_t idx) {
        if (idx == 0) {
            return first;
        }
        return second;
    }

    const A& operator[](std::size_t idx) const {
        if (idx == 0) {
            return first;
        }
        return second;
    }
};

template <typename T>
inline VitteSlice<T> list(T value) {
    void* mem = vitte::runtime::alloc(sizeof(T));
    auto* data = static_cast<T*>(mem);
    data[0] = value;
    return VitteSlice<T>{data, 1};
}

template <typename T>
inline VitteSlice<T> vitte_empty_slice() {
    return VitteSlice<T>{nullptr, 0};
}

template <typename A, typename B>
inline VittePair<A, B> vitte_pair(A first, B second) {
    return VittePair<A, B>{std::move(first), std::move(second)};
}

template <typename T>
inline VitteSlice<T> vitte_slice_push(VitteSlice<T> base, T value) {
    std::size_t next_len = base.len + 1;
    void* mem = vitte::runtime::alloc(sizeof(T) * next_len);
    auto* out = static_cast<T*>(mem);
    for (std::size_t i = 0; i < base.len; ++i) {
        out[i] = base.data[i];
    }
    out[base.len] = value;
    return VitteSlice<T>{out, next_len};
}

template <typename T>
inline VitteSlice<T> operator+(VitteSlice<T> lhs, VitteSlice<T> rhs) {
    std::size_t next_len = lhs.len + rhs.len;
    void* mem = vitte::runtime::alloc(sizeof(T) * next_len);
    auto* out = static_cast<T*>(mem);
    for (std::size_t i = 0; i < lhs.len; ++i) {
        out[i] = lhs.data[i];
    }
    for (std::size_t i = 0; i < rhs.len; ++i) {
        out[lhs.len + i] = rhs.data[i];
    }
    return VitteSlice<T>{out, next_len};
}

template <typename F>
struct VitteScopeExit {
    F fn;
    bool active = true;

    explicit VitteScopeExit(F f)
        : fn(std::move(f)) {}

    VitteScopeExit(VitteScopeExit&& other) noexcept
        : fn(std::move(other.fn)), active(other.active) {
        other.active = false;
    }

    VitteScopeExit(const VitteScopeExit&) = delete;
    VitteScopeExit& operator=(const VitteScopeExit&) = delete;

    ~VitteScopeExit() {
        if (active) {
            fn();
        }
    }
};

template <typename F>
inline VitteScopeExit<F> vitte_make_scope_exit(F fn) {
    return VitteScopeExit<F>(std::move(fn));
}

[[noreturn]]
inline void vitte_raise(VitteString msg) {
    throw std::runtime_error(std::string(msg.data ? msg.data : "", msg.len));
}

template <typename T>
inline VitteSlice<T> list(T first, T second) {
    return list(first) + list(second);
}

template <typename T, typename... Rest>
inline VitteSlice<T> list(T first, T second, Rest... rest) {
    return list(first, second) + list(rest...);
}

struct VitteUnit {
    std::uint8_t _dummy;
};

extern "C" void vitte_builtin_trap(VitteString msg);
extern "C" const char* vitte_c_abi_version();
VitteSlice<std::int32_t> vitte_empty_slice_i32();
VitteSlice<VitteString> vitte_empty_slice_string();
VitteSlice<VitteString> list();
VitteSlice<std::int32_t> vitte_slice_push_i32(VitteSlice<std::int32_t> base, std::int32_t value);
VitteSlice<VitteString> vitte_slice_push_string(VitteSlice<VitteString> base, VitteString value);
VitteSlice<std::int32_t> vitte__vitte_empty_slice_i32();
VitteSlice<VitteString> vitte__vitte_empty_slice_string();
VitteSlice<std::int32_t> vitte__vitte_slice_push_i32(VitteSlice<std::int32_t> base, std::int32_t value);
VitteSlice<VitteString> vitte__vitte_slice_push_string(VitteSlice<VitteString> base, VitteString value);
VitteString vitte_string_concat(VitteString a, VitteString b);
VitteString vitte_i32_to_string(std::int32_t v);
VitteString vitte__vitte_string_concat(VitteString a, VitteString b);
VitteString vitte__vitte_i32_to_string(std::int32_t v);

enum class VitteIoErrorKind : std::uint8_t {
    NotFound,
    PermissionDenied,
    ConnectionRefused,
    ConnectionReset,
    ConnectionAborted,
    NotConnected,
    AddrInUse,
    AddrNotAvailable,
    BrokenPipe,
    AlreadyExists,
    WouldBlock,
    InvalidInput,
    InvalidData,
    TimedOut,
    Interrupted,
    UnexpectedEof,
    Unsupported,
    Other
};

struct VitteOptionString {
    std::uint8_t tag; // 0 = None, 1 = Some
    VitteString value;
};


struct VitteIoError {
    VitteIoErrorKind kind;
    VitteOptionString message;
};

template <typename T>
struct VitteResultIo {
    std::uint8_t tag; // 0 = Ok, 1 = Err
    union {
        T ok;
        VitteIoError err;
    };
};

struct VitteIpV4 {
    std::uint8_t a;
    std::uint8_t b;
    std::uint8_t c;
    std::uint8_t d;
};

struct VitteIpV6 {
    std::uint16_t a;
    std::uint16_t b;
    std::uint16_t c;
    std::uint16_t d;
    std::uint16_t e;
    std::uint16_t f;
    std::uint16_t g;
    std::uint16_t h;
};

struct VitteIpAddr {
    std::uint8_t tag; // 0 = V4, 1 = V6
    union {
        VitteIpV4 v4;
        VitteIpV6 v6;
    };
};

struct VitteSocketAddr {
    VitteIpAddr ip;
    std::uint16_t port;
};

struct VitteTcpStream { std::size_t fd; };
struct VitteTcpListener { std::size_t fd; };
struct VitteUdpSocket { std::size_t fd; };

struct VitteUdpRecv {
    VitteSocketAddr addr;
    VitteSlice<std::uint8_t> data;
    std::size_t size;
};

struct VitteJsonMember;

struct VitteJsonValue {
    std::uint8_t tag; // 0 null,1 bool,2 number,3 string,4 array,5 object
    union {
        bool boolean;
        double number;
        VitteString string;
        VitteSlice<VitteJsonValue> array;
        VitteSlice<VitteJsonMember> object;
    };
};

struct VitteJsonMember {
    VitteString key;
    VitteJsonValue value;
};

template <typename T>
struct VitteResult {
    std::uint8_t tag; // 0 = Ok, 1 = Err
    union {
        T ok;
        VitteString err;
    };
};

struct VitteRegex {
    VitteString pattern;
};

struct VitteProcessResult {
    std::int32_t status;
    VitteString out;
    VitteString err;
};

struct VitteExitStatus {
    std::int32_t code;
};

struct VitteProcessChild {
    std::int64_t id;
};

struct VitteFswatchWatcher {
    VitteString path;
};

enum class VitteFswatchEventKind : std::uint8_t {
    Modified,
    Deleted,
    Renamed
};

struct VitteFswatchEvent {
    VitteString path;
    VitteFswatchEventKind kind;
};

struct VitteRegexMatch {
    std::size_t start;
    std::size_t end;
    VitteString text;
};

struct VitteOptionRegexMatch {
    std::uint8_t tag; // 0 = None, 1 = Some
    VitteRegexMatch value;
};

struct VitteDbHandle {
    std::int64_t id;
};

struct VitteDbEntry {
    VitteString key;
    VitteString value;
};

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif
extern "C" {

// CLI
void vitte_set_args(int argc, const char** argv);
VitteSlice<VitteString> cli_args();

// Time
std::uint64_t time_now_ms();
VitteUnit time_sleep_ms(std::uint32_t ms);

// Env
VitteOptionString env_get(VitteString name);
bool env_set(VitteString name, VitteString value);

// OS
VitteString os_platform();
VitteString os_arch();
VitteOptionString os_home_dir();
VitteString os_temp_dir();
VitteResult<VitteString> os_current_dir();
bool os_set_current_dir(VitteString path);
VitteResult<VitteString> os_exe_path();
VitteString os_path_sep();

// Process
VitteResult<VitteProcessResult> process_run(VitteString cmd);
VitteResult<VitteProcessResult> process_run_args(VitteString cmd, VitteSlice<VitteString> args);
VitteResult<VitteProcessResult> process_run_shell(VitteString cmdline);
VitteResult<VitteProcessChild> process_spawn(VitteString cmd, VitteSlice<VitteString> args);
VitteResult<VitteExitStatus> process_wait(VitteProcessChild* child);
VitteResult<VitteUnit> process_kill(VitteProcessChild* child);
VitteResult<VitteString> process_stdout(VitteProcessChild* child);
VitteResult<VitteString> process_stderr(VitteProcessChild* child);

// JSON
VitteResult<VitteJsonValue> json_parse(VitteString text);
VitteString json_stringify(VitteJsonValue value);

// HTTP
enum class VitteHttpMethod : std::uint8_t {
    Get,
    Post,
    Put,
    Delete
};

struct VitteHttpHeader {
    VitteString name;
    VitteString value;
};

struct VitteHttpRequest {
    VitteHttpMethod method;
    VitteString url;
    VitteString body;
    VitteSlice<VitteHttpHeader> headers;
    std::uint32_t timeout_ms;
    bool follow_redirects;
    bool http2;
};

struct VitteHttpResponse {
    std::int32_t status;
    VitteString body;
};

VitteResult<VitteHttpResponse> http_request(VitteHttpRequest req);

// Crypto
VitteResult<VitteSlice<std::uint8_t>> crypto_sha256(VitteSlice<std::uint8_t> data);
VitteResult<VitteSlice<std::uint8_t>> crypto_sha1(VitteSlice<std::uint8_t> data);
VitteResult<VitteSlice<std::uint8_t>> crypto_hmac_sha256(
    VitteSlice<std::uint8_t> key,
    VitteSlice<std::uint8_t> data
);
VitteResult<VitteSlice<std::uint8_t>> crypto_rand_bytes(std::size_t len);

// TCP
VitteResultIo<VitteTcpStream> tcp_connect(VitteSocketAddr addr);
VitteResultIo<VitteTcpListener> tcp_bind(VitteSocketAddr addr);
VitteResultIo<VitteTcpStream> tcp_accept(VitteTcpListener* listener);
VitteResultIo<std::size_t> tcp_read(VitteTcpStream* stream, VitteSlice<std::uint8_t> buf);
VitteResultIo<std::size_t> tcp_write(VitteTcpStream* stream, VitteSlice<std::uint8_t> buf);
VitteResultIo<VitteUnit> tcp_close(VitteTcpStream* stream);

// UDP
VitteResultIo<VitteUdpSocket> udp_bind(VitteSocketAddr addr);
VitteResultIo<VitteUdpRecv> udp_recv_from(VitteUdpSocket* sock, VitteSlice<std::uint8_t> buf);
VitteResultIo<std::size_t> udp_send_to(VitteUdpSocket* sock, VitteSlice<std::uint8_t> buf, VitteSocketAddr addr);
VitteResultIo<VitteUnit> udp_close(VitteUdpSocket* sock);
VitteResultIo<VitteUnit> udp_set_nonblocking(VitteUdpSocket* sock, bool enabled);
VitteResultIo<VitteUnit> udp_set_read_timeout(VitteUdpSocket* sock, std::uint32_t ms);
VitteResultIo<VitteUnit> udp_set_write_timeout(VitteUdpSocket* sock, std::uint32_t ms);

// TCP options
VitteResultIo<VitteUnit> tcp_set_nonblocking(VitteTcpStream* stream, bool enabled);
VitteResultIo<VitteUnit> tcp_set_read_timeout(VitteTcpStream* stream, std::uint32_t ms);
VitteResultIo<VitteUnit> tcp_set_write_timeout(VitteTcpStream* stream, std::uint32_t ms);

// Regex
VitteResult<VitteRegex> regex_compile(VitteString pat);
bool regex_is_match(VitteRegex re, VitteString text);
VitteOptionRegexMatch regex_find(VitteRegex re, VitteString text);
VitteString regex_replace(VitteRegex re, VitteString text, VitteString with);
VitteSlice<VitteString> regex_split(VitteRegex re, VitteString text);

// File system watch
VitteResult<VitteFswatchWatcher> fswatch_watch(VitteString path);
VitteResult<VitteFswatchEvent> fswatch_poll(VitteFswatchWatcher* w);
VitteResult<VitteUnit> fswatch_close(VitteFswatchWatcher* w);

// DB (simple kv)
VitteResult<VitteDbHandle> db_open(VitteString path);
VitteResult<VitteUnit> db_close(VitteDbHandle* db);
VitteResult<VitteUnit> db_set(VitteDbHandle* db, VitteString key, VitteString value);
VitteResult<VitteOptionString> db_get(VitteDbHandle* db, VitteString key);
VitteResult<bool> db_delete(VitteDbHandle* db, VitteString key);
VitteResult<VitteSlice<VitteString>> db_keys(VitteDbHandle* db);
VitteResult<VitteSlice<VitteString>> db_keys_prefix(VitteDbHandle* db, VitteString prefix);
VitteResult<VitteUnit> db_batch_put(VitteDbHandle* db, VitteSlice<VitteDbEntry> entries);
VitteResult<VitteUnit> db_begin(VitteDbHandle* db);
VitteResult<VitteUnit> db_commit(VitteDbHandle* db);
VitteResult<VitteUnit> db_rollback(VitteDbHandle* db);

} // extern "C"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
