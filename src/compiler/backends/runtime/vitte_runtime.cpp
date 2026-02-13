#include "vitte_runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <cstring>
#include <string>
#include <vector>
#include <cctype>
#include <chrono>
#include <thread>
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <curl/curl.h>

#if defined(_WIN32)
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#  define vitte_popen _popen
#  define vitte_pclose _pclose
#else
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <netinet/in.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <errno.h>
#  if defined(__APPLE__)
#    include <mach-o/dyld.h>
#  endif
#  if defined(__linux__)
#    include <sys/inotify.h>
#  elif defined(__APPLE__) || defined(__FreeBSD__)
#    include <sys/event.h>
#    include <sys/time.h>
#  endif
#  define vitte_popen popen
#  define vitte_pclose pclose
#endif

namespace vitte::runtime {

/* -------------------------------------------------
 * Allocation
 * ------------------------------------------------- */

void* alloc(std::size_t size) {
    void* p = std::malloc(size);
    if (!p) {
        panic("allocation failed");
    }
    return p;
}

void dealloc(void* ptr) {
    std::free(ptr);
}

/* -------------------------------------------------
 * Panic / abort
 * ------------------------------------------------- */

[[noreturn]]
void panic(const char* msg) {
    std::fprintf(stderr, "[vitte panic] %s\n", msg);
    std::fflush(stderr);
    std::abort();
}

/* -------------------------------------------------
 * Assertions (used by lowered MIR)
 * ------------------------------------------------- */

void assert_true(bool cond, const char* msg) {
    if (!cond) {
        panic(msg);
    }
}

/* -------------------------------------------------
 * Unreachable
 * ------------------------------------------------- */

[[noreturn]]
void unreachable() {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#else
    std::abort();
#endif
}

/* -------------------------------------------------
 * Exception boundary (optional)
 * ------------------------------------------------- */

void terminate() {
    panic("terminate called");
}

void print_i32(int32_t value) {
    std::printf("%d\n", value);
}

} // namespace vitte::runtime

/* -------------------------------------------------
 * C ABI stubs (stdlib bindings)
 * ------------------------------------------------- */

static std::vector<VitteString> g_cli_args;

static VitteString vitte_make_string(const std::string& s) {
    if (s.empty()) {
        return VitteString{nullptr, 0};
    }
    void* mem = vitte::runtime::alloc(s.size());
    std::memcpy(mem, s.data(), s.size());
    return VitteString{static_cast<const char*>(mem), s.size()};
}

static std::string vitte_to_string(VitteString s) {
    if (!s.data || s.len == 0) {
        return std::string();
    }
    return std::string(s.data, s.len);
}

static std::vector<std::string> vitte_to_string_vec(VitteSlice<VitteString> args) {
    std::vector<std::string> out;
    out.reserve(args.len);
    for (std::size_t i = 0; i < args.len; ++i) {
        out.push_back(vitte_to_string(args.data[i]));
    }
    return out;
}

static VitteSlice<VitteString> vitte_make_string_slice(const std::vector<std::string>& items) {
    if (items.empty()) {
        return VitteSlice<VitteString>{nullptr, 0};
    }
    void* mem = vitte::runtime::alloc(sizeof(VitteString) * items.size());
    auto* data = static_cast<VitteString*>(mem);
    for (std::size_t i = 0; i < items.size(); ++i) {
        data[i] = vitte_make_string(items[i]);
    }
    return VitteSlice<VitteString>{data, items.size()};
}

static VitteSlice<std::uint8_t> vitte_make_u8_slice(const std::uint8_t* data, std::size_t len) {
    if (!data || len == 0) {
        return VitteSlice<std::uint8_t>{nullptr, 0};
    }
    void* mem = vitte::runtime::alloc(len);
    std::memcpy(mem, data, len);
    return VitteSlice<std::uint8_t>{static_cast<std::uint8_t*>(mem), len};
}

VitteSlice<std::int32_t> vitte_empty_slice_i32() {
    return VitteSlice<std::int32_t>{nullptr, 0};
}

VitteSlice<VitteString> vitte_empty_slice_string() {
    return VitteSlice<VitteString>{nullptr, 0};
}

VitteSlice<std::int32_t> vitte_slice_push_i32(VitteSlice<std::int32_t> base, std::int32_t value) {
    std::size_t next_len = base.len + 1;
    void* mem = vitte::runtime::alloc(sizeof(std::int32_t) * next_len);
    auto* out = static_cast<std::int32_t*>(mem);
    for (std::size_t i = 0; i < base.len; ++i) {
        out[i] = base.data[i];
    }
    out[base.len] = value;
    return VitteSlice<std::int32_t>{out, next_len};
}

VitteSlice<VitteString> vitte_slice_push_string(VitteSlice<VitteString> base, VitteString value) {
    std::size_t next_len = base.len + 1;
    void* mem = vitte::runtime::alloc(sizeof(VitteString) * next_len);
    auto* out = static_cast<VitteString*>(mem);
    for (std::size_t i = 0; i < base.len; ++i) {
        out[i] = base.data[i];
    }
    out[base.len] = value;
    return VitteSlice<VitteString>{out, next_len};
}

VitteSlice<std::int32_t> vitte__vitte_empty_slice_i32() {
    return vitte_empty_slice_i32();
}

VitteSlice<VitteString> vitte__vitte_empty_slice_string() {
    return vitte_empty_slice_string();
}

VitteSlice<std::int32_t> vitte__vitte_slice_push_i32(VitteSlice<std::int32_t> base, std::int32_t value) {
    return vitte_slice_push_i32(base, value);
}

VitteSlice<VitteString> vitte__vitte_slice_push_string(VitteSlice<VitteString> base, VitteString value) {
    return vitte_slice_push_string(base, value);
}

VitteString vitte_string_concat(VitteString a, VitteString b) {
    std::string sa = vitte_to_string(a);
    std::string sb = vitte_to_string(b);
    return vitte_make_string(sa + sb);
}

VitteString vitte_i32_to_string(std::int32_t v) {
    return vitte_make_string(std::to_string(v));
}

VitteString vitte__vitte_string_concat(VitteString a, VitteString b) {
    return vitte_string_concat(a, b);
}

VitteString vitte__vitte_i32_to_string(std::int32_t v) {
    return vitte_i32_to_string(v);
}

static VitteOptionString vitte_none_string() {
    VitteOptionString opt{};
    opt.tag = 0;
    opt.value = VitteString{nullptr, 0};
    return opt;
}

static VitteOptionString vitte_some_string(const std::string& value) {
    VitteOptionString opt{};
    opt.tag = 1;
    opt.value = vitte_make_string(value);
    return opt;
}

static VitteOptionRegexMatch vitte_none_match() {
    VitteOptionRegexMatch opt{};
    opt.tag = 0;
    opt.value = VitteRegexMatch{0, 0, VitteString{nullptr, 0}};
    return opt;
}

static VitteOptionRegexMatch vitte_some_match(const VitteRegexMatch& m) {
    VitteOptionRegexMatch opt{};
    opt.tag = 1;
    opt.value = m;
    return opt;
}

static VitteIoError vitte_io_error(VitteIoErrorKind kind) {
    VitteIoError err{};
    err.kind = kind;
    err.message = vitte_none_string();
    return err;
}

template <typename T>
static VitteResultIo<T> vitte_io_err(VitteIoErrorKind kind) {
    VitteResultIo<T> res{};
    res.tag = 1;
    res.err = vitte_io_error(kind);
    return res;
}

template <typename T>
static VitteResultIo<T> vitte_io_ok(const T& value) {
    VitteResultIo<T> res{};
    res.tag = 0;
    res.ok = value;
    return res;
}

template <typename T>
static VitteResult<T> vitte_err_string(const char* msg) {
    VitteResult<T> res{};
    res.tag = 1;
    res.err = VitteString{msg, std::strlen(msg)};
    return res;
}

template <typename T>
static VitteResult<T> vitte_err_string_alloc(const std::string& msg) {
    VitteResult<T> res{};
    res.tag = 1;
    res.err = vitte_make_string(msg);
    return res;
}
template <typename T>
static VitteResult<T> vitte_ok(const T& value) {
    VitteResult<T> res{};
    res.tag = 0;
    res.ok = value;
    return res;
}

static void ensure_curl() {
    static bool initialized = false;
    if (!initialized) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        initialized = true;
    }
}

#if defined(_WIN32)
static bool ensure_winsock() {
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            return false;
        }
        initialized = true;
    }
    return true;
}
#endif

struct FswatchState {
    enum class Kind : std::uint8_t {
        Poll,
        Inotify,
        Kqueue
    };
    Kind kind;
    std::string path;
    std::filesystem::file_time_type ts;
#if defined(__linux__)
    int fd = -1;
    int wd = -1;
#elif defined(__APPLE__) || defined(__FreeBSD__)
    int kq = -1;
    int fd = -1;
#endif
};

static std::unordered_map<std::string, FswatchState> g_fswatch_states;
static std::mutex g_fswatch_mutex;

static void fswatch_close(FswatchState& state) {
#if defined(__linux__)
    if (state.fd >= 0) {
        ::close(state.fd);
        state.fd = -1;
        state.wd = -1;
    }
#elif defined(__APPLE__) || defined(__FreeBSD__)
    if (state.kq >= 0) {
        ::close(state.kq);
        state.kq = -1;
    }
    if (state.fd >= 0) {
        ::close(state.fd);
        state.fd = -1;
    }
#else
    (void)state;
#endif
}

struct ProcessState {
    std::int64_t id = 0;
#if defined(_WIN32)
    HANDLE process = nullptr;
    HANDLE out_read = nullptr;
    HANDLE err_read = nullptr;
#else
    pid_t pid = -1;
    int out_fd = -1;
    int err_fd = -1;
#endif
    bool done = false;
    int exit_code = 0;
    std::string out;
    std::string err;
};

static std::unordered_map<std::int64_t, ProcessState> g_processes;
static std::mutex g_process_mutex;
static std::int64_t g_next_process_id = 1;

struct DbState {
    std::string path;
    std::unordered_map<std::string, std::string> kv;
    bool in_tx = false;
    std::unordered_map<std::string, std::string> tx_put;
    std::unordered_set<std::string> tx_del;
};

static std::unordered_map<std::int64_t, DbState> g_dbs;
static std::mutex g_db_mutex;
static std::int64_t g_next_db_id = 1;

static VitteIoErrorKind map_io_error() {
#if defined(_WIN32)
    int err = WSAGetLastError();
    switch (err) {
        case WSAEWOULDBLOCK: return VitteIoErrorKind::WouldBlock;
        case WSAECONNREFUSED: return VitteIoErrorKind::ConnectionRefused;
        case WSAECONNRESET: return VitteIoErrorKind::ConnectionReset;
        case WSAETIMEDOUT: return VitteIoErrorKind::TimedOut;
        case WSAEADDRINUSE: return VitteIoErrorKind::AddrInUse;
        case WSAEADDRNOTAVAIL: return VitteIoErrorKind::AddrNotAvailable;
        case WSAENOTCONN: return VitteIoErrorKind::NotConnected;
        default: return VitteIoErrorKind::Other;
    }
#else
    switch (errno) {
        case EWOULDBLOCK: return VitteIoErrorKind::WouldBlock;
        case ECONNREFUSED: return VitteIoErrorKind::ConnectionRefused;
        case ECONNRESET: return VitteIoErrorKind::ConnectionReset;
        case ETIMEDOUT: return VitteIoErrorKind::TimedOut;
        case EADDRINUSE: return VitteIoErrorKind::AddrInUse;
        case EADDRNOTAVAIL: return VitteIoErrorKind::AddrNotAvailable;
        case ENOTCONN: return VitteIoErrorKind::NotConnected;
        default: return VitteIoErrorKind::Other;
    }
#endif
}

static VitteResultIo<VitteUnit> set_nonblocking_fd(std::size_t fd, bool enabled) {
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(fd);
    u_long mode = enabled ? 1 : 0;
    if (ioctlsocket(s, FIONBIO, &mode) != 0) {
        return vitte_io_err<VitteUnit>(map_io_error());
    }
#else
    int s = static_cast<int>(fd);
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) {
        return vitte_io_err<VitteUnit>(map_io_error());
    }
    if (enabled) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(s, F_SETFL, flags) != 0) {
        return vitte_io_err<VitteUnit>(map_io_error());
    }
#endif
    VitteUnit u{};
    return vitte_io_ok(u);
}

static VitteResultIo<VitteUnit> set_timeout_fd(std::size_t fd, int opt, std::uint32_t ms) {
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(fd);
    DWORD timeout = static_cast<DWORD>(ms);
    if (setsockopt(s, SOL_SOCKET, opt, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) != 0) {
        return vitte_io_err<VitteUnit>(map_io_error());
    }
#else
    int s = static_cast<int>(fd);
    timeval tv{};
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    if (setsockopt(s, SOL_SOCKET, opt, &tv, sizeof(tv)) != 0) {
        return vitte_io_err<VitteUnit>(map_io_error());
    }
#endif
    VitteUnit u{};
    return vitte_io_ok(u);
}

static bool socket_addr_to_native(VitteSocketAddr addr, sockaddr_storage& out, socklen_t& out_len) {
    std::memset(&out, 0, sizeof(out));
    if (addr.ip.tag == 0) {
        auto* in = reinterpret_cast<sockaddr_in*>(&out);
        in->sin_family = AF_INET;
        in->sin_port = htons(addr.port);
        std::uint32_t ip =
            (static_cast<std::uint32_t>(addr.ip.v4.a) << 24) |
            (static_cast<std::uint32_t>(addr.ip.v4.b) << 16) |
            (static_cast<std::uint32_t>(addr.ip.v4.c) << 8) |
            (static_cast<std::uint32_t>(addr.ip.v4.d));
        in->sin_addr.s_addr = htonl(ip);
        out_len = sizeof(sockaddr_in);
        return true;
    }
    if (addr.ip.tag == 1) {
        auto* in6 = reinterpret_cast<sockaddr_in6*>(&out);
        in6->sin6_family = AF_INET6;
        in6->sin6_port = htons(addr.port);
        std::uint16_t words[8] = {
            htons(addr.ip.v6.a), htons(addr.ip.v6.b),
            htons(addr.ip.v6.c), htons(addr.ip.v6.d),
            htons(addr.ip.v6.e), htons(addr.ip.v6.f),
            htons(addr.ip.v6.g), htons(addr.ip.v6.h),
        };
        std::memcpy(in6->sin6_addr.s6_addr, words, sizeof(words));
        out_len = sizeof(sockaddr_in6);
        return true;
    }
    return false;
}

static VitteSocketAddr native_to_socket_addr(const sockaddr_storage& in) {
    VitteSocketAddr out{};
    if (in.ss_family == AF_INET) {
        const auto* addr = reinterpret_cast<const sockaddr_in*>(&in);
        std::uint32_t ip = ntohl(addr->sin_addr.s_addr);
        out.ip.tag = 0;
        out.ip.v4.a = static_cast<std::uint8_t>((ip >> 24) & 0xff);
        out.ip.v4.b = static_cast<std::uint8_t>((ip >> 16) & 0xff);
        out.ip.v4.c = static_cast<std::uint8_t>((ip >> 8) & 0xff);
        out.ip.v4.d = static_cast<std::uint8_t>(ip & 0xff);
        out.port = ntohs(addr->sin_port);
        return out;
    }
    if (in.ss_family == AF_INET6) {
        const auto* addr = reinterpret_cast<const sockaddr_in6*>(&in);
        out.ip.tag = 1;
        const std::uint16_t* words = reinterpret_cast<const std::uint16_t*>(addr->sin6_addr.s6_addr);
        out.ip.v6.a = ntohs(words[0]);
        out.ip.v6.b = ntohs(words[1]);
        out.ip.v6.c = ntohs(words[2]);
        out.ip.v6.d = ntohs(words[3]);
        out.ip.v6.e = ntohs(words[4]);
        out.ip.v6.f = ntohs(words[5]);
        out.ip.v6.g = ntohs(words[6]);
        out.ip.v6.h = ntohs(words[7]);
        out.port = ntohs(addr->sin6_port);
        return out;
    }
    out.ip.tag = 0;
    out.ip.v4 = VitteIpV4{127, 0, 0, 1};
    out.port = 0;
    return out;
}

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: {
                unsigned char uc = static_cast<unsigned char>(c);
                if (uc < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", uc);
                    out += buf;
                } else {
                    out.push_back(c);
                }
                break;
            }
        }
    }
    return out;
}

static std::string json_to_string(const VitteJsonValue& v) {
    switch (v.tag) {
        case 0: return "null";
        case 1: return v.boolean ? "true" : "false";
        case 2: {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.17g", v.number);
            return buf;
        }
        case 3: {
            std::string s(v.string.data ? v.string.data : "", v.string.len);
            return "\"" + json_escape(s) + "\"";
        }
        case 4: {
            std::string out = "[";
            for (std::size_t i = 0; i < v.array.len; ++i) {
                if (i) out += ", ";
                out += json_to_string(v.array.data[i]);
            }
            out += "]";
            return out;
        }
        case 5: {
            std::string out = "{";
            for (std::size_t i = 0; i < v.object.len; ++i) {
                if (i) out += ", ";
                const auto& m = v.object.data[i];
                std::string key(m.key.data ? m.key.data : "", m.key.len);
                out += "\"" + json_escape(key) + "\": ";
                out += json_to_string(m.value);
            }
            out += "}";
            return out;
        }
        default:
            return "null";
    }
}

struct CurlBuffer {
    std::string data;
};

static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t total = size * nmemb;
    if (!userdata) return 0;
    auto* buf = static_cast<CurlBuffer*>(userdata);
    buf->data.append(ptr, total);
    return total;
}

class JsonParser {
public:
    explicit JsonParser(const char* data, std::size_t len)
        : data_(data), len_(len) {}

    bool parse(VitteJsonValue& out, std::string& err) {
        skip_ws();
        if (!parse_value(out, err)) {
            return false;
        }
        skip_ws();
        if (pos_ != len_) {
            err = "trailing characters";
            return false;
        }
        return true;
    }

private:
    const char* data_;
    std::size_t len_;
    std::size_t pos_ = 0;

    void skip_ws() {
        while (pos_ < len_ && std::isspace(static_cast<unsigned char>(data_[pos_]))) {
            pos_++;
        }
    }

    bool match(char c) {
        if (pos_ < len_ && data_[pos_] == c) {
            pos_++;
            return true;
        }
        return false;
    }

    bool parse_value(VitteJsonValue& out, std::string& err) {
        skip_ws();
        if (pos_ >= len_) {
            err = "unexpected end";
            return false;
        }
        char c = data_[pos_];
        if (c == 'n') return parse_keyword("null", 0, out, err);
        if (c == 't') return parse_keyword("true", 1, out, err, true);
        if (c == 'f') return parse_keyword("false", 1, out, err, false);
        if (c == '"') return parse_string(out, err);
        if (c == '[') return parse_array(out, err);
        if (c == '{') return parse_object(out, err);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number(out, err);
        err = "invalid json";
        return false;
    }

    bool parse_keyword(const char* kw, std::uint8_t tag, VitteJsonValue& out, std::string& err, bool boolean = false) {
        std::size_t n = std::strlen(kw);
        if (pos_ + n > len_ || std::strncmp(data_ + pos_, kw, n) != 0) {
            err = "invalid json";
            return false;
        }
        pos_ += n;
        out.tag = tag;
        if (tag == 1) {
            out.boolean = boolean;
        }
        return true;
    }

    bool parse_string(VitteJsonValue& out, std::string& err) {
        if (!match('"')) {
            err = "expected string";
            return false;
        }
        std::string s;
        while (pos_ < len_) {
            char c = data_[pos_++];
            if (c == '"') {
                out.tag = 3;
                out.string = vitte_make_string(s);
                return true;
            }
            if (c == '\\') {
                if (pos_ >= len_) {
                    err = "invalid escape";
                    return false;
                }
                char e = data_[pos_++];
                switch (e) {
                    case '"': s.push_back('"'); break;
                    case '\\': s.push_back('\\'); break;
                    case '/': s.push_back('/'); break;
                    case 'b': s.push_back('\b'); break;
                    case 'f': s.push_back('\f'); break;
                    case 'n': s.push_back('\n'); break;
                    case 'r': s.push_back('\r'); break;
                    case 't': s.push_back('\t'); break;
                    case 'u': {
                        if (pos_ + 4 > len_) {
                            err = "invalid unicode escape";
                            return false;
                        }
                        auto hex = [&](char ch) -> int {
                            if (ch >= '0' && ch <= '9') return ch - '0';
                            if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
                            if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
                            return -1;
                        };
                        int h0 = hex(data_[pos_]);
                        int h1 = hex(data_[pos_ + 1]);
                        int h2 = hex(data_[pos_ + 2]);
                        int h3 = hex(data_[pos_ + 3]);
                        if (h0 < 0 || h1 < 0 || h2 < 0 || h3 < 0) {
                            err = "invalid unicode escape";
                            return false;
                        }
                        std::uint32_t code = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
                        pos_ += 4;
                        if (code >= 0xD800 && code <= 0xDBFF) {
                            if (pos_ + 6 > len_ || data_[pos_] != '\\' || data_[pos_ + 1] != 'u') {
                                err = "invalid unicode surrogate";
                                return false;
                            }
                            pos_ += 2;
                            int l0 = hex(data_[pos_]);
                            int l1 = hex(data_[pos_ + 1]);
                            int l2 = hex(data_[pos_ + 2]);
                            int l3 = hex(data_[pos_ + 3]);
                            if (l0 < 0 || l1 < 0 || l2 < 0 || l3 < 0) {
                                err = "invalid unicode surrogate";
                                return false;
                            }
                            std::uint32_t low = (l0 << 12) | (l1 << 8) | (l2 << 4) | l3;
                            pos_ += 4;
                            if (low < 0xDC00 || low > 0xDFFF) {
                                err = "invalid unicode surrogate";
                                return false;
                            }
                            code = 0x10000 + (((code - 0xD800) << 10) | (low - 0xDC00));
                        } else if (code >= 0xDC00 && code <= 0xDFFF) {
                            err = "invalid unicode surrogate";
                            return false;
                        }
                        auto append_utf8 = [&](std::uint32_t cp) {
                            if (cp <= 0x7F) {
                                s.push_back(static_cast<char>(cp));
                            } else if (cp <= 0x7FF) {
                                s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                                s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                            } else if (cp <= 0xFFFF) {
                                s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                                s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                                s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                            } else {
                                s.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                                s.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                                s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                                s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                            }
                        };
                        append_utf8(code);
                        break;
                    }
                    default:
                        err = "invalid escape";
                        return false;
                }
            } else {
                s.push_back(c);
            }
        }
        err = "unterminated string";
        return false;
    }

    bool parse_number(VitteJsonValue& out, std::string& err) {
        std::size_t start = pos_;
        if (data_[pos_] == '-') pos_++;
        while (pos_ < len_ && std::isdigit(static_cast<unsigned char>(data_[pos_]))) pos_++;
        if (pos_ < len_ && data_[pos_] == '.') {
            pos_++;
            while (pos_ < len_ && std::isdigit(static_cast<unsigned char>(data_[pos_]))) pos_++;
        }
        if (pos_ < len_ && (data_[pos_] == 'e' || data_[pos_] == 'E')) {
            pos_++;
            if (pos_ < len_ && (data_[pos_] == '+' || data_[pos_] == '-')) pos_++;
            while (pos_ < len_ && std::isdigit(static_cast<unsigned char>(data_[pos_]))) pos_++;
        }
        std::string num(data_ + start, pos_ - start);
        char* endptr = nullptr;
        double v = std::strtod(num.c_str(), &endptr);
        if (!endptr || *endptr != '\0') {
            err = "invalid number";
            return false;
        }
        out.tag = 2;
        out.number = v;
        return true;
    }

    bool parse_array(VitteJsonValue& out, std::string& err) {
        if (!match('[')) {
            err = "expected array";
            return false;
        }
        skip_ws();
        std::vector<VitteJsonValue> items;
        if (match(']')) {
            out.tag = 4;
            out.array = VitteSlice<VitteJsonValue>{nullptr, 0};
            return true;
        }
        while (true) {
            VitteJsonValue v{};
            if (!parse_value(v, err)) {
                return false;
            }
            items.push_back(v);
            skip_ws();
            if (match(']')) break;
            if (!match(',')) {
                err = "expected ',' in array";
                return false;
            }
        }
        std::size_t n = items.size();
        VitteJsonValue* data = nullptr;
        if (n > 0) {
            data = static_cast<VitteJsonValue*>(vitte::runtime::alloc(sizeof(VitteJsonValue) * n));
            std::memcpy(data, items.data(), sizeof(VitteJsonValue) * n);
        }
        out.tag = 4;
        out.array = VitteSlice<VitteJsonValue>{data, n};
        return true;
    }

    bool parse_object(VitteJsonValue& out, std::string& err) {
        if (!match('{')) {
            err = "expected object";
            return false;
        }
        skip_ws();
        std::vector<VitteJsonMember> members;
        if (match('}')) {
            out.tag = 5;
            out.object = VitteSlice<VitteJsonMember>{nullptr, 0};
            return true;
        }
        while (true) {
            VitteJsonValue key_val{};
            if (!parse_string(key_val, err)) {
                return false;
            }
            skip_ws();
            if (!match(':')) {
                err = "expected ':' in object";
                return false;
            }
            VitteJsonValue value{};
            if (!parse_value(value, err)) {
                return false;
            }
            VitteJsonMember m{};
            m.key = key_val.string;
            m.value = value;
            members.push_back(m);
            skip_ws();
            if (match('}')) break;
            if (!match(',')) {
                err = "expected ',' in object";
                return false;
            }
        }
        std::size_t n = members.size();
        VitteJsonMember* data = nullptr;
        if (n > 0) {
            data = static_cast<VitteJsonMember*>(vitte::runtime::alloc(sizeof(VitteJsonMember) * n));
            std::memcpy(data, members.data(), sizeof(VitteJsonMember) * n);
        }
        out.tag = 5;
        out.object = VitteSlice<VitteJsonMember>{data, n};
        return true;
    }
};

extern "C" {

void vitte_builtin_trap(VitteString msg) {
    if (msg.data) {
        std::string s(msg.data, msg.len);
        vitte::runtime::panic(s.c_str());
    }
    vitte::runtime::panic("trap");
}

void vitte_set_args(int argc, const char** argv) {
    g_cli_args.clear();
    if (!argv || argc <= 0) {
        return;
    }
    g_cli_args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        const char* s = argv[i] ? argv[i] : "";
        g_cli_args.push_back(VitteString{s, std::strlen(s)});
    }
}

VitteSlice<VitteString> cli_args() {
    VitteSlice<VitteString> out{};
    out.data = g_cli_args.empty() ? nullptr : g_cli_args.data();
    out.len = g_cli_args.size();
    return out;
}

std::uint64_t time_now_ms() {
    using namespace std::chrono;
    auto now = steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(duration_cast<milliseconds>(now).count());
}

VitteUnit time_sleep_ms(std::uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    VitteUnit u{};
    return u;
}

VitteOptionString env_get(VitteString name) {
    if (!name.data) {
        return vitte_none_string();
    }
    std::string key(name.data, name.len);
    const char* val = std::getenv(key.c_str());
    if (!val) {
        return vitte_none_string();
    }
    VitteOptionString out{};
    out.tag = 1;
    out.value = vitte_make_string(std::string(val));
    return out;
}

bool env_set(VitteString name, VitteString value) {
    if (!name.data) {
        return false;
    }
    std::string key(name.data, name.len);
    std::string val = value.data ? std::string(value.data, value.len) : std::string();
#if defined(_WIN32)
    std::string pair = key + "=" + val;
    return _putenv(pair.c_str()) == 0;
#else
    return ::setenv(key.c_str(), val.c_str(), 1) == 0;
#endif
}

VitteString os_platform() {
#if defined(_WIN32)
    return vitte_make_string("windows");
#elif defined(__APPLE__)
    return vitte_make_string("macos");
#elif defined(__linux__)
    return vitte_make_string("linux");
#elif defined(__FreeBSD__)
    return vitte_make_string("freebsd");
#else
    return vitte_make_string("unknown");
#endif
}

VitteOptionString os_home_dir() {
#if defined(_WIN32)
    const char* home = std::getenv("USERPROFILE");
    if (!home) {
        const char* drive = std::getenv("HOMEDRIVE");
        const char* path = std::getenv("HOMEPATH");
        if (drive && path) {
            return vitte_some_string(std::string(drive) + path);
        }
        return vitte_none_string();
    }
    return vitte_some_string(std::string(home));
#else
    const char* home = std::getenv("HOME");
    if (!home) {
        return vitte_none_string();
    }
    return vitte_some_string(std::string(home));
#endif
}

VitteString os_arch() {
#if defined(__x86_64__) || defined(_M_X64)
    return vitte_make_string("x86_64");
#elif defined(__aarch64__) || defined(_M_ARM64)
    return vitte_make_string("arm64");
#elif defined(__i386__) || defined(_M_IX86)
    return vitte_make_string("x86");
#else
    return vitte_make_string("unknown");
#endif
}

VitteString os_temp_dir() {
    std::error_code ec;
    auto p = std::filesystem::temp_directory_path(ec);
    if (ec) {
        return vitte_make_string("/tmp");
    }
    return vitte_make_string(p.string());
}

VitteResult<VitteString> os_current_dir() {
    std::error_code ec;
    auto p = std::filesystem::current_path(ec);
    if (ec) {
        return vitte_err_string<VitteString>("failed to get current dir");
    }
    return vitte_ok(vitte_make_string(p.string()));
}

bool os_set_current_dir(VitteString path) {
    std::error_code ec;
    std::filesystem::current_path(vitte_to_string(path), ec);
    return !ec;
}

VitteResult<VitteString> os_exe_path() {
#if defined(_WIN32)
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0) {
        return vitte_err_string<VitteString>("failed to get exe path");
    }
    return vitte_ok(vitte_make_string(std::string(buf, len)));
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string path(size, '\0');
    if (_NSGetExecutablePath(path.data(), &size) != 0) {
        return vitte_err_string<VitteString>("failed to get exe path");
    }
    path.resize(std::strlen(path.c_str()));
    return vitte_ok(vitte_make_string(path));
#else
    char buf[4096];
    ssize_t len = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len <= 0) {
        return vitte_err_string<VitteString>("failed to get exe path");
    }
    buf[len] = '\0';
    return vitte_ok(vitte_make_string(std::string(buf, len)));
#endif
}

VitteString os_path_sep() {
#if defined(_WIN32)
    return vitte_make_string("\\");
#else
    return vitte_make_string("/");
#endif
}

static void read_fd_all(int fd, std::string& out) {
    char buffer[4096];
    while (true) {
        ssize_t n = ::read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            out.append(buffer, n);
            continue;
        }
        break;
    }
}

#if defined(_WIN32)
static void read_handle_all(HANDLE h, std::string& out) {
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(h, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        out.append(buffer, buffer + read);
    }
}

static std::string quote_win_arg(const std::string& s) {
    if (s.find_first_of(" \t\"") == std::string::npos) {
        return s;
    }
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') {
            out += "\\\"";
        } else {
            out += c;
        }
    }
    out += "\"";
    return out;
}
#endif

static VitteResult<VitteProcessChild> process_spawn_internal(const std::string& cmd, const std::vector<std::string>& args, bool shell) {
    if (cmd.empty()) {
        return vitte_err_string<VitteProcessChild>("empty command");
    }
    ProcessState state{};
    std::lock_guard<std::mutex> lock(g_process_mutex);
    state.id = g_next_process_id++;
#if defined(_WIN32)
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE out_read = nullptr;
    HANDLE out_write = nullptr;
    HANDLE err_read = nullptr;
    HANDLE err_write = nullptr;
    if (!CreatePipe(&out_read, &out_write, &sa, 0)) {
        return vitte_err_string<VitteProcessChild>("create pipe failed");
    }
    if (!CreatePipe(&err_read, &err_write, &sa, 0)) {
        CloseHandle(out_read);
        CloseHandle(out_write);
        return vitte_err_string<VitteProcessChild>("create pipe failed");
    }
    SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(err_read, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = out_write;
    si.hStdError = err_write;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    std::string cmdline;
    if (shell) {
        cmdline = "cmd.exe /C " + cmd;
    } else {
        cmdline = quote_win_arg(cmd);
        for (const auto& a : args) {
            cmdline += " ";
            cmdline += quote_win_arg(a);
        }
    }

    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(
        nullptr,
        cmdline.data(),
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );
    CloseHandle(out_write);
    CloseHandle(err_write);
    if (!ok) {
        CloseHandle(out_read);
        CloseHandle(err_read);
        return vitte_err_string<VitteProcessChild>("failed to spawn process");
    }
    CloseHandle(pi.hThread);
    state.process = pi.hProcess;
    state.out_read = out_read;
    state.err_read = err_read;
#else
    int out_pipe[2];
    int err_pipe[2];
    if (::pipe(out_pipe) != 0 || ::pipe(err_pipe) != 0) {
        return vitte_err_string<VitteProcessChild>("pipe failed");
    }
    pid_t pid = ::fork();
    if (pid == 0) {
        ::dup2(out_pipe[1], STDOUT_FILENO);
        ::dup2(err_pipe[1], STDERR_FILENO);
        ::close(out_pipe[0]);
        ::close(out_pipe[1]);
        ::close(err_pipe[0]);
        ::close(err_pipe[1]);
        if (shell) {
            execl("/bin/sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
        } else {
            std::vector<char*> argv;
            argv.reserve(args.size() + 2);
            argv.push_back(const_cast<char*>(cmd.c_str()));
            for (const auto& a : args) {
                argv.push_back(const_cast<char*>(a.c_str()));
            }
            argv.push_back(nullptr);
            execvp(cmd.c_str(), argv.data());
        }
        _exit(127);
    }
    if (pid < 0) {
        ::close(out_pipe[0]);
        ::close(out_pipe[1]);
        ::close(err_pipe[0]);
        ::close(err_pipe[1]);
        return vitte_err_string<VitteProcessChild>("failed to spawn process");
    }
    ::close(out_pipe[1]);
    ::close(err_pipe[1]);
    state.pid = pid;
    state.out_fd = out_pipe[0];
    state.err_fd = err_pipe[0];
#endif
    g_processes.emplace(state.id, std::move(state));
    VitteProcessChild child{state.id};
    return vitte_ok(child);
}

static void process_collect(ProcessState& state) {
    if (state.done) {
        return;
    }
#if defined(_WIN32)
    WaitForSingleObject(state.process, INFINITE);
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(state.process, &exit_code)) {
        exit_code = 1;
    }
    read_handle_all(state.out_read, state.out);
    read_handle_all(state.err_read, state.err);
    CloseHandle(state.out_read);
    CloseHandle(state.err_read);
    CloseHandle(state.process);
    state.exit_code = static_cast<int>(exit_code);
#else
    int status = 0;
    ::waitpid(state.pid, &status, 0);
    read_fd_all(state.out_fd, state.out);
    read_fd_all(state.err_fd, state.err);
    ::close(state.out_fd);
    ::close(state.err_fd);
    if (WIFEXITED(status)) {
        state.exit_code = WEXITSTATUS(status);
    } else {
        state.exit_code = 1;
    }
#endif
    state.done = true;
}

VitteResult<VitteProcessResult> process_run(VitteString cmd) {
    auto res = process_spawn_internal(vitte_to_string(cmd), {}, true);
    if (res.tag != 0) {
        return vitte_err_string_alloc<VitteProcessResult>(vitte_to_string(res.err));
    }
    VitteProcessChild child = res.ok;
    auto status = process_wait(&child);
    if (status.tag != 0) {
        return vitte_err_string_alloc<VitteProcessResult>(vitte_to_string(status.err));
    }
    auto out = process_stdout(&child);
    auto err = process_stderr(&child);
    VitteProcessResult pr{};
    pr.status = status.ok.code;
    pr.out = out.tag == 0 ? out.ok : vitte_make_string(std::string());
    pr.err = err.tag == 0 ? err.ok : vitte_make_string(std::string());
    return vitte_ok(pr);
}

VitteResult<VitteProcessResult> process_run_args(VitteString cmd, VitteSlice<VitteString> args) {
    auto res = process_spawn_internal(vitte_to_string(cmd), vitte_to_string_vec(args), false);
    if (res.tag != 0) {
        return vitte_err_string_alloc<VitteProcessResult>(vitte_to_string(res.err));
    }
    VitteProcessChild child = res.ok;
    auto status = process_wait(&child);
    if (status.tag != 0) {
        return vitte_err_string_alloc<VitteProcessResult>(vitte_to_string(status.err));
    }
    auto out = process_stdout(&child);
    auto err = process_stderr(&child);
    VitteProcessResult pr{};
    pr.status = status.ok.code;
    pr.out = out.tag == 0 ? out.ok : vitte_make_string(std::string());
    pr.err = err.tag == 0 ? err.ok : vitte_make_string(std::string());
    return vitte_ok(pr);
}

VitteResult<VitteProcessResult> process_run_shell(VitteString cmdline) {
    return process_run(cmdline);
}

VitteResult<VitteProcessChild> process_spawn(VitteString cmd, VitteSlice<VitteString> args) {
    return process_spawn_internal(vitte_to_string(cmd), vitte_to_string_vec(args), false);
}

VitteResult<VitteExitStatus> process_wait(VitteProcessChild* child) {
    if (!child) {
        return vitte_err_string<VitteExitStatus>("invalid child");
    }
    std::lock_guard<std::mutex> lock(g_process_mutex);
    auto it = g_processes.find(child->id);
    if (it == g_processes.end()) {
        return vitte_err_string<VitteExitStatus>("unknown child");
    }
    process_collect(it->second);
    VitteExitStatus st{it->second.exit_code};
    return vitte_ok(st);
}

VitteResult<VitteUnit> process_kill(VitteProcessChild* child) {
    if (!child) {
        return vitte_err_string<VitteUnit>("invalid child");
    }
    std::lock_guard<std::mutex> lock(g_process_mutex);
    auto it = g_processes.find(child->id);
    if (it == g_processes.end()) {
        return vitte_err_string<VitteUnit>("unknown child");
    }
#if defined(_WIN32)
    TerminateProcess(it->second.process, 1);
#else
    ::kill(it->second.pid, SIGKILL);
#endif
    VitteUnit u{};
    return vitte_ok(u);
}

VitteResult<VitteString> process_stdout(VitteProcessChild* child) {
    if (!child) {
        return vitte_err_string<VitteString>("invalid child");
    }
    std::lock_guard<std::mutex> lock(g_process_mutex);
    auto it = g_processes.find(child->id);
    if (it == g_processes.end()) {
        return vitte_err_string<VitteString>("unknown child");
    }
    process_collect(it->second);
    return vitte_ok(vitte_make_string(it->second.out));
}

VitteResult<VitteString> process_stderr(VitteProcessChild* child) {
    if (!child) {
        return vitte_err_string<VitteString>("invalid child");
    }
    std::lock_guard<std::mutex> lock(g_process_mutex);
    auto it = g_processes.find(child->id);
    if (it == g_processes.end()) {
        return vitte_err_string<VitteString>("unknown child");
    }
    process_collect(it->second);
    return vitte_ok(vitte_make_string(it->second.err));
}

VitteResult<VitteJsonValue> json_parse(VitteString text) {
    if (!text.data) {
        return vitte_err_string<VitteJsonValue>("null json");
    }
    std::string err;
    VitteJsonValue value{};
    JsonParser parser(text.data, text.len);
    if (!parser.parse(value, err)) {
        return vitte_err_string_alloc<VitteJsonValue>(err);
    }
    return vitte_ok(value);
}

VitteString json_stringify(VitteJsonValue value) {
    std::string out = json_to_string(value);
    return vitte_make_string(out);
}

VitteResult<VitteHttpResponse> http_request(VitteHttpRequest req) {
    ensure_curl();
    CURL* curl = curl_easy_init();
    if (!curl) {
        return vitte_err_string<VitteHttpResponse>("curl init failed");
    }

    std::string url(req.url.data ? req.url.data : "", req.url.len);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, req.follow_redirects ? 1L : 0L);
    if (req.timeout_ms > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(req.timeout_ms));
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(req.timeout_ms));
    }
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, req.http2 ? CURL_HTTP_VERSION_2TLS : CURL_HTTP_VERSION_1_1);

    switch (req.method) {
        case VitteHttpMethod::Get:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
        case VitteHttpMethod::Post: {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            std::string body(req.body.data ? req.body.data : "", req.body.len);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
            break;
        }
        case VitteHttpMethod::Put: {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            std::string body(req.body.data ? req.body.data : "", req.body.len);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
            break;
        }
        case VitteHttpMethod::Delete:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        default:
            break;
    }

    struct curl_slist* headers = nullptr;
    for (std::size_t i = 0; i < req.headers.len; ++i) {
        const auto& h = req.headers.data[i];
        std::string name(h.name.data ? h.name.data : "", h.name.len);
        std::string value(h.value.data ? h.value.data : "", h.value.len);
        std::string line = name + ": " + value;
        headers = curl_slist_append(headers, line.c_str());
    }
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CurlBuffer buffer;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    CURLcode rc = curl_easy_perform(curl);
    if (headers) {
        curl_slist_free_all(headers);
    }
    if (rc != CURLE_OK) {
        curl_easy_cleanup(curl);
        return vitte_err_string_alloc<VitteHttpResponse>(curl_easy_strerror(rc));
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);

    VitteHttpResponse resp{};
    resp.status = static_cast<std::int32_t>(status);
    resp.body = vitte_make_string(buffer.data);
    return vitte_ok(resp);
}

VitteResult<VitteSlice<std::uint8_t>> crypto_sha256(VitteSlice<std::uint8_t> data) {
    if (!data.data && data.len != 0) {
        return vitte_err_string<VitteSlice<std::uint8_t>>("invalid input");
    }
    std::uint8_t digest[SHA256_DIGEST_LENGTH];
    if (!SHA256(data.data, data.len, digest)) {
        return vitte_err_string<VitteSlice<std::uint8_t>>("sha256 failed");
    }
    return vitte_ok(vitte_make_u8_slice(digest, SHA256_DIGEST_LENGTH));
}

VitteResult<VitteSlice<std::uint8_t>> crypto_sha1(VitteSlice<std::uint8_t> data) {
    if (!data.data && data.len != 0) {
        return vitte_err_string<VitteSlice<std::uint8_t>>("invalid input");
    }
    std::uint8_t digest[SHA_DIGEST_LENGTH];
    if (!SHA1(data.data, data.len, digest)) {
        return vitte_err_string<VitteSlice<std::uint8_t>>("sha1 failed");
    }
    return vitte_ok(vitte_make_u8_slice(digest, SHA_DIGEST_LENGTH));
}

VitteResult<VitteSlice<std::uint8_t>> crypto_hmac_sha256(
    VitteSlice<std::uint8_t> key,
    VitteSlice<std::uint8_t> data
) {
    if ((!key.data && key.len != 0) || (!data.data && data.len != 0)) {
        return vitte_err_string<VitteSlice<std::uint8_t>>("invalid input");
    }
    unsigned int out_len = 0;
    unsigned char* out = HMAC(
        EVP_sha256(),
        key.data,
        static_cast<int>(key.len),
        data.data,
        data.len,
        nullptr,
        &out_len
    );
    if (!out || out_len == 0) {
        return vitte_err_string<VitteSlice<std::uint8_t>>("hmac_sha256 failed");
    }
    return vitte_ok(vitte_make_u8_slice(out, out_len));
}

VitteResult<VitteSlice<std::uint8_t>> crypto_rand_bytes(std::size_t len) {
    if (len == 0) {
        return vitte_ok(VitteSlice<std::uint8_t>{nullptr, 0});
    }
    std::vector<std::uint8_t> buf(len);
    if (RAND_bytes(buf.data(), static_cast<int>(len)) != 1) {
        return vitte_err_string<VitteSlice<std::uint8_t>>("rand_bytes failed");
    }
    return vitte_ok(vitte_make_u8_slice(buf.data(), len));
}

VitteResultIo<VitteTcpStream> tcp_connect(VitteSocketAddr addr) {
#if defined(_WIN32)
    if (!ensure_winsock()) {
        return vitte_io_err<VitteTcpStream>(VitteIoErrorKind::Other);
    }
#endif
    sockaddr_storage native{};
    socklen_t native_len = 0;
    if (!socket_addr_to_native(addr, native, native_len)) {
        return vitte_io_err<VitteTcpStream>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = ::socket(native.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        return vitte_io_err<VitteTcpStream>(map_io_error());
    }
    if (::connect(s, reinterpret_cast<sockaddr*>(&native), native_len) != 0) {
        ::closesocket(s);
        return vitte_io_err<VitteTcpStream>(map_io_error());
    }
    VitteTcpStream out{static_cast<std::size_t>(s)};
    return vitte_io_ok(out);
#else
    int s = ::socket(native.ss_family, SOCK_STREAM, 0);
    if (s < 0) {
        return vitte_io_err<VitteTcpStream>(map_io_error());
    }
    if (::connect(s, reinterpret_cast<sockaddr*>(&native), native_len) != 0) {
        ::close(s);
        return vitte_io_err<VitteTcpStream>(map_io_error());
    }
    VitteTcpStream out{static_cast<std::size_t>(s)};
    return vitte_io_ok(out);
#endif
}

VitteResultIo<VitteTcpListener> tcp_bind(VitteSocketAddr addr) {
#if defined(_WIN32)
    if (!ensure_winsock()) {
        return vitte_io_err<VitteTcpListener>(VitteIoErrorKind::Other);
    }
#endif
    sockaddr_storage native{};
    socklen_t native_len = 0;
    if (!socket_addr_to_native(addr, native, native_len)) {
        return vitte_io_err<VitteTcpListener>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = ::socket(native.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        return vitte_io_err<VitteTcpListener>(map_io_error());
    }
    if (::bind(s, reinterpret_cast<sockaddr*>(&native), native_len) != 0) {
        ::closesocket(s);
        return vitte_io_err<VitteTcpListener>(map_io_error());
    }
    if (::listen(s, 128) != 0) {
        ::closesocket(s);
        return vitte_io_err<VitteTcpListener>(map_io_error());
    }
    VitteTcpListener out{static_cast<std::size_t>(s)};
    return vitte_io_ok(out);
#else
    int s = ::socket(native.ss_family, SOCK_STREAM, 0);
    if (s < 0) {
        return vitte_io_err<VitteTcpListener>(map_io_error());
    }
    int yes = 1;
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (::bind(s, reinterpret_cast<sockaddr*>(&native), native_len) != 0) {
        ::close(s);
        return vitte_io_err<VitteTcpListener>(map_io_error());
    }
    if (::listen(s, 128) != 0) {
        ::close(s);
        return vitte_io_err<VitteTcpListener>(map_io_error());
    }
    VitteTcpListener out{static_cast<std::size_t>(s)};
    return vitte_io_ok(out);
#endif
}

VitteResultIo<VitteTcpStream> tcp_accept(VitteTcpListener* listener) {
    if (!listener) {
        return vitte_io_err<VitteTcpStream>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(listener->fd);
    SOCKET c = ::accept(s, nullptr, nullptr);
    if (c == INVALID_SOCKET) {
        return vitte_io_err<VitteTcpStream>(map_io_error());
    }
    VitteTcpStream out{static_cast<std::size_t>(c)};
    return vitte_io_ok(out);
#else
    int s = static_cast<int>(listener->fd);
    int c = ::accept(s, nullptr, nullptr);
    if (c < 0) {
        return vitte_io_err<VitteTcpStream>(map_io_error());
    }
    VitteTcpStream out{static_cast<std::size_t>(c)};
    return vitte_io_ok(out);
#endif
}

VitteResultIo<std::size_t> tcp_read(VitteTcpStream* stream, VitteSlice<std::uint8_t> buf) {
    if (!stream || !buf.data) {
        return vitte_io_err<std::size_t>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(stream->fd);
    int n = ::recv(s, reinterpret_cast<char*>(buf.data), static_cast<int>(buf.len), 0);
    if (n < 0) {
        return vitte_io_err<std::size_t>(map_io_error());
    }
    return vitte_io_ok<std::size_t>(static_cast<std::size_t>(n));
#else
    int s = static_cast<int>(stream->fd);
    ssize_t n = ::recv(s, buf.data, buf.len, 0);
    if (n < 0) {
        return vitte_io_err<std::size_t>(map_io_error());
    }
    return vitte_io_ok<std::size_t>(static_cast<std::size_t>(n));
#endif
}

VitteResultIo<std::size_t> tcp_write(VitteTcpStream* stream, VitteSlice<std::uint8_t> buf) {
    if (!stream || !buf.data) {
        return vitte_io_err<std::size_t>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(stream->fd);
    int n = ::send(s, reinterpret_cast<const char*>(buf.data), static_cast<int>(buf.len), 0);
    if (n < 0) {
        return vitte_io_err<std::size_t>(map_io_error());
    }
    return vitte_io_ok<std::size_t>(static_cast<std::size_t>(n));
#else
    int s = static_cast<int>(stream->fd);
    ssize_t n = ::send(s, buf.data, buf.len, 0);
    if (n < 0) {
        return vitte_io_err<std::size_t>(map_io_error());
    }
    return vitte_io_ok<std::size_t>(static_cast<std::size_t>(n));
#endif
}

VitteResultIo<VitteUnit> tcp_close(VitteTcpStream* stream) {
    if (!stream) {
        return vitte_io_err<VitteUnit>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(stream->fd);
    ::closesocket(s);
#else
    int s = static_cast<int>(stream->fd);
    ::close(s);
#endif
    VitteUnit u{};
    return vitte_io_ok(u);
}

VitteResultIo<VitteUnit> tcp_set_nonblocking(VitteTcpStream* stream, bool enabled) {
    if (!stream) {
        return vitte_io_err<VitteUnit>(VitteIoErrorKind::InvalidInput);
    }
    return set_nonblocking_fd(stream->fd, enabled);
}

VitteResultIo<VitteUnit> tcp_set_read_timeout(VitteTcpStream* stream, std::uint32_t ms) {
    if (!stream) {
        return vitte_io_err<VitteUnit>(VitteIoErrorKind::InvalidInput);
    }
    return set_timeout_fd(stream->fd, SO_RCVTIMEO, ms);
}

VitteResultIo<VitteUnit> tcp_set_write_timeout(VitteTcpStream* stream, std::uint32_t ms) {
    if (!stream) {
        return vitte_io_err<VitteUnit>(VitteIoErrorKind::InvalidInput);
    }
    return set_timeout_fd(stream->fd, SO_SNDTIMEO, ms);
}

VitteResultIo<VitteUdpSocket> udp_bind(VitteSocketAddr addr) {
#if defined(_WIN32)
    if (!ensure_winsock()) {
        return vitte_io_err<VitteUdpSocket>(VitteIoErrorKind::Other);
    }
#endif
    sockaddr_storage native{};
    socklen_t native_len = 0;
    if (!socket_addr_to_native(addr, native, native_len)) {
        return vitte_io_err<VitteUdpSocket>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = ::socket(native.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        return vitte_io_err<VitteUdpSocket>(map_io_error());
    }
    if (::bind(s, reinterpret_cast<sockaddr*>(&native), native_len) != 0) {
        ::closesocket(s);
        return vitte_io_err<VitteUdpSocket>(map_io_error());
    }
    VitteUdpSocket out{static_cast<std::size_t>(s)};
    return vitte_io_ok(out);
#else
    int s = ::socket(native.ss_family, SOCK_DGRAM, 0);
    if (s < 0) {
        return vitte_io_err<VitteUdpSocket>(map_io_error());
    }
    if (::bind(s, reinterpret_cast<sockaddr*>(&native), native_len) != 0) {
        ::close(s);
        return vitte_io_err<VitteUdpSocket>(map_io_error());
    }
    VitteUdpSocket out{static_cast<std::size_t>(s)};
    return vitte_io_ok(out);
#endif
}

VitteResultIo<VitteUdpRecv> udp_recv_from(VitteUdpSocket* sock, VitteSlice<std::uint8_t> buf) {
    if (!sock || !buf.data) {
        return vitte_io_err<VitteUdpRecv>(VitteIoErrorKind::InvalidInput);
    }
    sockaddr_storage native{};
    socklen_t native_len = sizeof(native);
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(sock->fd);
    int n = ::recvfrom(
        s,
        reinterpret_cast<char*>(buf.data),
        static_cast<int>(buf.len),
        0,
        reinterpret_cast<sockaddr*>(&native),
        &native_len
    );
    if (n < 0) {
        return vitte_io_err<VitteUdpRecv>(map_io_error());
    }
#else
    int s = static_cast<int>(sock->fd);
    ssize_t n = ::recvfrom(
        s,
        buf.data,
        buf.len,
        0,
        reinterpret_cast<sockaddr*>(&native),
        &native_len
    );
    if (n < 0) {
        return vitte_io_err<VitteUdpRecv>(map_io_error());
    }
#endif
    VitteUdpRecv out{};
    out.addr = native_to_socket_addr(native);
    out.data = VitteSlice<std::uint8_t>{buf.data, static_cast<std::size_t>(n)};
    out.size = static_cast<std::size_t>(n);
    return vitte_io_ok(out);
}

VitteResultIo<std::size_t> udp_send_to(VitteUdpSocket* sock, VitteSlice<std::uint8_t> buf, VitteSocketAddr addr) {
    if (!sock || !buf.data) {
        return vitte_io_err<std::size_t>(VitteIoErrorKind::InvalidInput);
    }
    sockaddr_storage native{};
    socklen_t native_len = 0;
    if (!socket_addr_to_native(addr, native, native_len)) {
        return vitte_io_err<std::size_t>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(sock->fd);
    int n = ::sendto(
        s,
        reinterpret_cast<const char*>(buf.data),
        static_cast<int>(buf.len),
        0,
        reinterpret_cast<sockaddr*>(&native),
        native_len
    );
    if (n < 0) {
        return vitte_io_err<std::size_t>(map_io_error());
    }
    return vitte_io_ok<std::size_t>(static_cast<std::size_t>(n));
#else
    int s = static_cast<int>(sock->fd);
    ssize_t n = ::sendto(
        s,
        buf.data,
        buf.len,
        0,
        reinterpret_cast<sockaddr*>(&native),
        native_len
    );
    if (n < 0) {
        return vitte_io_err<std::size_t>(map_io_error());
    }
    return vitte_io_ok<std::size_t>(static_cast<std::size_t>(n));
#endif
}

VitteResultIo<VitteUnit> udp_close(VitteUdpSocket* sock) {
    if (!sock) {
        return vitte_io_err<VitteUnit>(VitteIoErrorKind::InvalidInput);
    }
#if defined(_WIN32)
    SOCKET s = static_cast<SOCKET>(sock->fd);
    ::closesocket(s);
#else
    int s = static_cast<int>(sock->fd);
    ::close(s);
#endif
    VitteUnit u{};
    return vitte_io_ok(u);
}

VitteResultIo<VitteUnit> udp_set_nonblocking(VitteUdpSocket* sock, bool enabled) {
    if (!sock) {
        return vitte_io_err<VitteUnit>(VitteIoErrorKind::InvalidInput);
    }
    return set_nonblocking_fd(sock->fd, enabled);
}

VitteResultIo<VitteUnit> udp_set_read_timeout(VitteUdpSocket* sock, std::uint32_t ms) {
    if (!sock) {
        return vitte_io_err<VitteUnit>(VitteIoErrorKind::InvalidInput);
    }
    return set_timeout_fd(sock->fd, SO_RCVTIMEO, ms);
}

VitteResultIo<VitteUnit> udp_set_write_timeout(VitteUdpSocket* sock, std::uint32_t ms) {
    if (!sock) {
        return vitte_io_err<VitteUnit>(VitteIoErrorKind::InvalidInput);
    }
    return set_timeout_fd(sock->fd, SO_SNDTIMEO, ms);
}

VitteResult<VitteRegex> regex_compile(VitteString pat) {
    std::string pattern = vitte_to_string(pat);
    try {
        std::regex re(pattern);
        VitteRegex out{};
        out.pattern = vitte_make_string(pattern);
        return vitte_ok(out);
    } catch (const std::regex_error& err) {
        return vitte_err_string_alloc<VitteRegex>(err.what());
    }
}

bool regex_is_match(VitteRegex re, VitteString text) {
    std::string pattern = vitte_to_string(re.pattern);
    std::string input = vitte_to_string(text);
    try {
        std::regex compiled(pattern);
        return std::regex_search(input, compiled);
    } catch (const std::regex_error&) {
        return false;
    }
}

VitteOptionRegexMatch regex_find(VitteRegex re, VitteString text) {
    std::string pattern = vitte_to_string(re.pattern);
    std::string input = vitte_to_string(text);
    try {
        std::regex compiled(pattern);
        std::smatch m;
        if (!std::regex_search(input, m, compiled)) {
            return vitte_none_match();
        }
        VitteRegexMatch out{};
        out.start = static_cast<std::size_t>(m.position());
        out.end = out.start + static_cast<std::size_t>(m.length());
        out.text = vitte_make_string(m.str());
        return vitte_some_match(out);
    } catch (const std::regex_error&) {
        return vitte_none_match();
    }
}

VitteString regex_replace(VitteRegex re, VitteString text, VitteString with) {
    std::string pattern = vitte_to_string(re.pattern);
    std::string input = vitte_to_string(text);
    std::string repl = vitte_to_string(with);
    try {
        std::regex compiled(pattern);
        return vitte_make_string(std::regex_replace(input, compiled, repl));
    } catch (const std::regex_error&) {
        return vitte_make_string(input);
    }
}

VitteSlice<VitteString> regex_split(VitteRegex re, VitteString text) {
    std::string pattern = vitte_to_string(re.pattern);
    std::string input = vitte_to_string(text);
    try {
        std::regex compiled(pattern);
        std::sregex_token_iterator it(input.begin(), input.end(), compiled, -1);
        std::sregex_token_iterator end;
        std::vector<std::string> parts;
        for (; it != end; ++it) {
            parts.push_back(*it);
        }
        return vitte_make_string_slice(parts);
    } catch (const std::regex_error&) {
        return vitte_make_string_slice(std::vector<std::string>{input});
    }
}

VitteResult<VitteFswatchWatcher> fswatch_watch(VitteString path) {
    std::string p = vitte_to_string(path);
    if (p.empty()) {
        return vitte_err_string<VitteFswatchWatcher>("empty path");
    }
    FswatchState state{};
    state.path = p;
    state.kind = FswatchState::Kind::Poll;
    std::error_code ec;
    state.ts = std::filesystem::file_time_type::min();
    if (std::filesystem::exists(p, ec)) {
        state.ts = std::filesystem::last_write_time(p, ec);
    }
#if defined(__linux__)
    int fd = ::inotify_init1(IN_NONBLOCK);
    if (fd >= 0) {
        int wd = ::inotify_add_watch(fd, p.c_str(), IN_MODIFY | IN_ATTRIB | IN_MOVE_SELF | IN_DELETE_SELF | IN_CLOSE_WRITE);
        if (wd >= 0) {
            state.kind = FswatchState::Kind::Inotify;
            state.fd = fd;
            state.wd = wd;
        } else {
            ::close(fd);
        }
    }
#elif defined(__APPLE__) || defined(__FreeBSD__)
#if defined(__APPLE__)
    int fd = ::open(p.c_str(), O_EVTONLY);
#else
    int fd = ::open(p.c_str(), O_RDONLY);
#endif
    if (fd >= 0) {
        int kq = ::kqueue();
        if (kq >= 0) {
            struct kevent ev{};
            EV_SET(&ev, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE | NOTE_ATTRIB | NOTE_RENAME | NOTE_DELETE, 0, nullptr);
            if (::kevent(kq, &ev, 1, nullptr, 0, nullptr) == 0) {
                state.kind = FswatchState::Kind::Kqueue;
                state.kq = kq;
                state.fd = fd;
            } else {
                ::close(kq);
                ::close(fd);
            }
        } else {
            ::close(fd);
        }
    }
#endif
    {
        std::lock_guard<std::mutex> lock(g_fswatch_mutex);
        auto it = g_fswatch_states.find(p);
        if (it != g_fswatch_states.end()) {
            fswatch_close(it->second);
            it->second = state;
        } else {
            g_fswatch_states.emplace(p, state);
        }
    }
    VitteFswatchWatcher out{};
    out.path = vitte_make_string(p);
    return vitte_ok(out);
}

VitteResult<VitteFswatchEvent> fswatch_poll(VitteFswatchWatcher* w) {
    if (!w || !w->path.data) {
        return vitte_err_string<VitteFswatchEvent>("invalid watcher");
    }
    std::string p = vitte_to_string(w->path);
    FswatchState state{};
    {
        std::lock_guard<std::mutex> lock(g_fswatch_mutex);
        auto it = g_fswatch_states.find(p);
        if (it == g_fswatch_states.end()) {
            return vitte_err_string<VitteFswatchEvent>("watcher not found");
        }
        state = it->second;
    }
    bool changed = false;
    VitteFswatchEventKind kind = VitteFswatchEventKind::Modified;
#if defined(__linux__)
    if (state.kind == FswatchState::Kind::Inotify && state.fd >= 0) {
        char buffer[4096];
        ssize_t n = ::read(state.fd, buffer, sizeof(buffer));
        if (n > 0) {
            changed = true;
            const auto* ev = reinterpret_cast<const inotify_event*>(buffer);
            if (ev->mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
                kind = (ev->mask & IN_DELETE_SELF) ? VitteFswatchEventKind::Deleted : VitteFswatchEventKind::Renamed;
            } else {
                kind = VitteFswatchEventKind::Modified;
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return vitte_err_string<VitteFswatchEvent>("fswatch read failed");
        }
    }
#elif defined(__APPLE__) || defined(__FreeBSD__)
    if (state.kind == FswatchState::Kind::Kqueue && state.kq >= 0) {
        struct kevent ev{};
        struct timespec ts{};
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        int n = ::kevent(state.kq, nullptr, 0, &ev, 1, &ts);
        if (n > 0) {
            if (ev.flags & EV_ERROR) {
                return vitte_err_string<VitteFswatchEvent>("fswatch error");
            }
            changed = true;
            if (ev.fflags & NOTE_DELETE) {
                kind = VitteFswatchEventKind::Deleted;
            } else if (ev.fflags & NOTE_RENAME) {
                kind = VitteFswatchEventKind::Renamed;
            } else {
                kind = VitteFswatchEventKind::Modified;
            }
        }
    }
#endif
    if (state.kind == FswatchState::Kind::Poll) {
        std::error_code ec;
        if (!std::filesystem::exists(p, ec)) {
            return vitte_err_string<VitteFswatchEvent>("path not found");
        }
        auto ts = std::filesystem::last_write_time(p, ec);
        if (ec) {
            return vitte_err_string<VitteFswatchEvent>("fswatch stat failed");
        }
        {
            std::lock_guard<std::mutex> lock(g_fswatch_mutex);
            auto it = g_fswatch_states.find(p);
            if (it != g_fswatch_states.end()) {
                if (it->second.ts != ts) {
                    it->second.ts = ts;
                    changed = true;
                }
            }
        }
    }
    if (!changed) {
        return vitte_err_string<VitteFswatchEvent>("no events");
    }
    VitteFswatchEvent ev{};
    ev.path = vitte_make_string(p);
    ev.kind = kind;
    return vitte_ok(ev);
}

VitteResult<VitteUnit> fswatch_close(VitteFswatchWatcher* w) {
    if (!w || !w->path.data) {
        return vitte_err_string<VitteUnit>("invalid watcher");
    }
    std::string p = vitte_to_string(w->path);
    std::lock_guard<std::mutex> lock(g_fswatch_mutex);
    auto it = g_fswatch_states.find(p);
    if (it == g_fswatch_states.end()) {
        return vitte_err_string<VitteUnit>("watcher not found");
    }
    fswatch_close(it->second);
    g_fswatch_states.erase(it);
    VitteUnit u{};
    return vitte_ok(u);
}

static bool db_load(DbState& db, std::string& err) {
    db.kv.clear();
    if (db.path.empty()) {
        err = "empty db path";
        return false;
    }
    std::ifstream in(db.path);
    if (!in.is_open()) {
        return true;
    }
    std::string line;
    while (std::getline(in, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        db.kv[key] = val;
    }
    return true;
}

static bool db_flush(const DbState& db, std::string& err) {
    std::ofstream out(db.path, std::ios::trunc);
    if (!out.is_open()) {
        err = "failed to write db file";
        return false;
    }
    for (const auto& kv : db.kv) {
        out << kv.first << "=" << kv.second << "\n";
    }
    return true;
}

VitteResult<VitteDbHandle> db_open(VitteString path) {
    DbState state{};
    state.path = vitte_to_string(path);
    std::string err;
    if (!db_load(state, err)) {
        return vitte_err_string_alloc<VitteDbHandle>(err);
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    std::int64_t id = g_next_db_id++;
    g_dbs.emplace(id, std::move(state));
    VitteDbHandle handle{id};
    return vitte_ok(handle);
}

VitteResult<VitteUnit> db_close(VitteDbHandle* db) {
    if (!db) {
        return vitte_err_string<VitteUnit>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<VitteUnit>("unknown db");
    }
    std::string err;
    if (!db_flush(it->second, err)) {
        return vitte_err_string_alloc<VitteUnit>(err);
    }
    g_dbs.erase(it);
    VitteUnit u{};
    return vitte_ok(u);
}

VitteResult<VitteUnit> db_set(VitteDbHandle* db, VitteString key, VitteString value) {
    if (!db) {
        return vitte_err_string<VitteUnit>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<VitteUnit>("unknown db");
    }
    auto k = vitte_to_string(key);
    auto v = vitte_to_string(value);
    if (it->second.in_tx) {
        it->second.tx_put[k] = v;
        it->second.tx_del.erase(k);
    } else {
        it->second.kv[k] = v;
        std::string err;
        if (!db_flush(it->second, err)) {
            return vitte_err_string_alloc<VitteUnit>(err);
        }
    }
    VitteUnit u{};
    return vitte_ok(u);
}

VitteResult<VitteOptionString> db_get(VitteDbHandle* db, VitteString key) {
    if (!db) {
        return vitte_err_string<VitteOptionString>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<VitteOptionString>("unknown db");
    }
    auto k = vitte_to_string(key);
    if (it->second.in_tx) {
        if (it->second.tx_del.count(k) > 0) {
            return vitte_ok(vitte_none_string());
        }
        auto tp = it->second.tx_put.find(k);
        if (tp != it->second.tx_put.end()) {
            return vitte_ok(vitte_some_string(tp->second));
        }
    }
    auto kv = it->second.kv.find(k);
    if (kv == it->second.kv.end()) {
        return vitte_ok(vitte_none_string());
    }
    return vitte_ok(vitte_some_string(kv->second));
}

VitteResult<bool> db_delete(VitteDbHandle* db, VitteString key) {
    if (!db) {
        return vitte_err_string<bool>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<bool>("unknown db");
    }
    auto k = vitte_to_string(key);
    bool existed = false;
    if (it->second.in_tx) {
        existed = it->second.kv.count(k) > 0 || it->second.tx_put.count(k) > 0;
        it->second.tx_put.erase(k);
        it->second.tx_del.insert(k);
        return vitte_ok(existed);
    }
    existed = it->second.kv.erase(k) > 0;
    std::string err;
    if (!db_flush(it->second, err)) {
        return vitte_err_string_alloc<bool>(err);
    }
    return vitte_ok(existed);
}

VitteResult<VitteSlice<VitteString>> db_keys(VitteDbHandle* db) {
    if (!db) {
        return vitte_err_string<VitteSlice<VitteString>>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<VitteSlice<VitteString>>("unknown db");
    }
    std::unordered_set<std::string> seen;
    std::vector<std::string> keys;
    keys.reserve(it->second.kv.size());
    for (const auto& kv : it->second.kv) {
        seen.insert(kv.first);
    }
    if (it->second.in_tx) {
        for (const auto& kv : it->second.tx_put) {
            seen.insert(kv.first);
        }
        for (const auto& del : it->second.tx_del) {
            seen.erase(del);
        }
    }
    keys.assign(seen.begin(), seen.end());
    return vitte_ok(vitte_make_string_slice(keys));
}

VitteResult<VitteSlice<VitteString>> db_keys_prefix(VitteDbHandle* db, VitteString prefix) {
    auto res = db_keys(db);
    if (res.tag != 0) {
        return res;
    }
    std::string pref = vitte_to_string(prefix);
    std::vector<std::string> out;
    for (std::size_t i = 0; i < res.ok.len; ++i) {
        std::string k = vitte_to_string(res.ok.data[i]);
        if (k.rfind(pref, 0) == 0) {
            out.push_back(k);
        }
    }
    return vitte_ok(vitte_make_string_slice(out));
}

VitteResult<VitteUnit> db_batch_put(VitteDbHandle* db, VitteSlice<VitteDbEntry> entries) {
    if (!db) {
        return vitte_err_string<VitteUnit>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<VitteUnit>("unknown db");
    }
    for (std::size_t i = 0; i < entries.len; ++i) {
        auto k = vitte_to_string(entries.data[i].key);
        auto v = vitte_to_string(entries.data[i].value);
        if (it->second.in_tx) {
            it->second.tx_put[k] = v;
            it->second.tx_del.erase(k);
        } else {
            it->second.kv[k] = v;
        }
    }
    if (!it->second.in_tx) {
        std::string err;
        if (!db_flush(it->second, err)) {
            return vitte_err_string_alloc<VitteUnit>(err);
        }
    }
    VitteUnit u{};
    return vitte_ok(u);
}

VitteResult<VitteUnit> db_begin(VitteDbHandle* db) {
    if (!db) {
        return vitte_err_string<VitteUnit>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<VitteUnit>("unknown db");
    }
    it->second.in_tx = true;
    it->second.tx_put.clear();
    it->second.tx_del.clear();
    VitteUnit u{};
    return vitte_ok(u);
}

VitteResult<VitteUnit> db_commit(VitteDbHandle* db) {
    if (!db) {
        return vitte_err_string<VitteUnit>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<VitteUnit>("unknown db");
    }
    for (const auto& kv : it->second.tx_put) {
        it->second.kv[kv.first] = kv.second;
    }
    for (const auto& del : it->second.tx_del) {
        it->second.kv.erase(del);
    }
    it->second.tx_put.clear();
    it->second.tx_del.clear();
    it->second.in_tx = false;
    std::string err;
    if (!db_flush(it->second, err)) {
        return vitte_err_string_alloc<VitteUnit>(err);
    }
    VitteUnit u{};
    return vitte_ok(u);
}

VitteResult<VitteUnit> db_rollback(VitteDbHandle* db) {
    if (!db) {
        return vitte_err_string<VitteUnit>("invalid db");
    }
    std::lock_guard<std::mutex> lock(g_db_mutex);
    auto it = g_dbs.find(db->id);
    if (it == g_dbs.end()) {
        return vitte_err_string<VitteUnit>("unknown db");
    }
    it->second.tx_put.clear();
    it->second.tx_del.clear();
    it->second.in_tx = false;
    VitteUnit u{};
    return vitte_ok(u);
}

// Host stubs for Arduino API so non-arduino builds can link examples.
void arduino_gpio_pin_mode(std::uint8_t, std::int32_t) {}
void arduino_gpio_digital_write(std::uint8_t, std::int32_t) {}
std::int32_t arduino_gpio_digital_read(std::uint8_t) { return 0; }
void arduino_gpio_analog_write(std::uint8_t, std::uint16_t) {}
std::uint16_t arduino_gpio_analog_read(std::uint8_t) { return 0; }

void arduino_serial_begin(std::uint32_t) {}
std::uint32_t arduino_serial_available() { return 0; }
std::int32_t arduino_serial_read() { return -1; }
std::size_t arduino_serial_write(std::uint8_t) { return 1; }
void arduino_serial_flush() {}

void arduino_i2c_begin() {}
void arduino_i2c_begin_transmission(std::uint8_t) {}
std::size_t arduino_i2c_write(std::uint8_t) { return 1; }
std::uint8_t arduino_i2c_end_transmission() { return 0; }
std::uint8_t arduino_i2c_request_from(std::uint8_t, std::uint8_t) { return 0; }
std::int32_t arduino_i2c_read() { return 0; }

void arduino_spi_begin() {}
std::uint8_t arduino_spi_transfer(std::uint8_t byte) { return byte; }
void arduino_spi_end() {}

void arduino_delay_ms(std::uint32_t) {}
void arduino_delay_us(std::uint32_t) {}
std::uint32_t arduino_millis() { return 0; }
std::uint32_t arduino_micros() { return 0; }

} // extern "C"
