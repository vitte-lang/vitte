#pragma once
#include <cstddef>
#include <cstdint>

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

struct VitteString {
    const char* data;
    std::size_t len;
};

template <typename T>
struct VitteSlice {
    T* data;
    std::size_t len;
};

struct VitteUnit {
    std::uint8_t _dummy;
};

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

struct VitteJsonValue;

struct VitteJsonMember {
    VitteString key;
    VitteJsonValue value;
};

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
    VitteString stdout;
    VitteString stderr;
};

struct VitteFswatchWatcher {
    VitteString path;
};

struct VitteFswatchEvent {
    VitteString path;
};

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
VitteOptionString os_home_dir();

// Process
VitteResult<VitteProcessResult> process_run(VitteString cmd);

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

// File system watch
VitteResult<VitteFswatchWatcher> fswatch_watch(VitteString path);
VitteResult<VitteFswatchEvent> fswatch_poll(VitteFswatchWatcher* w);

} // extern "C"
