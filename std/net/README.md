# Network Standard Library

The network standard library provides TCP/IP stack functionality and protocol implementations.

## Modules

### Core Module (`net`)
Main network initialization and configuration.

**Key Types:**
- `Error` - Network error enumeration
- `Stats` - Network statistics structure

**Key Functions:**
- `init()` - Initialize network stack
- `stats()` - Get network statistics
- `configure_dhcp()` - DHCP configuration
- `configure_static(ip, netmask, gateway)` - Static configuration
- `get_ip()` - Get current IP address
- `dns_resolve(hostname)` - Resolve hostname
- `ping(host)` - Ping host
- `shutdown()` - Shutdown network

### Socket Module (`net.socket`)
TCP and UDP socket operations.

**Key Types:**
- `SocketType` - TCP or UDP
- `SocketState` - Socket state enumeration
- `SocketId` - Socket identifier

**Key Functions:**
- `tcp_listen(port)` - Create listening socket
- `udp_bind(port)` - Create UDP socket
- `connect(socket_id, host, port)` - Connect to host
- `accept(socket_id)` - Accept connection
- `send(socket_id, data)` - Send data
- `receive(socket_id, buffer_size)` - Receive data
- `close(socket_id)` - Close socket
- `set_timeout(socket_id, timeout_ms)` - Set timeout
- `get_state(socket_id)` - Get socket state

### Address Module (`net.address`)
IP address handling and parsing.

**Key Types:**
- `AddressFamily` - IPv4 or IPv6
- `IPv4Addr` - IPv4 address structure
- `IPv6Addr` - IPv6 address structure
- `SocketAddr` - Socket address union

**Key Functions:**
- `parse_ipv4(addr_str)` - Parse IPv4 address
- `parse_ipv6(addr_str)` - Parse IPv6 address
- `to_string(addr)` - Convert address to string
- `is_localhost(addr)` - Check localhost
- `is_private(addr)` - Check private address
- `family(addr)` - Get address family

### Protocol Module (`net.protocol`)
HTTP and other protocol utilities.

**Key Types:**
- `HttpMethod` - HTTP methods (GET, POST, etc)
- `HttpStatus` - HTTP status codes
- `HttpRequest` - HTTP request structure
- `HttpResponse` - HTTP response structure

**Key Functions:**
- `http_request(request)` - Send HTTP request
- `parse_url(url)` - Parse URL
- `url_encode(param)` - URL encode
- `url_decode(encoded)` - URL decode
- `base64_encode(data)` - Encode to base64
- `base64_decode(encoded)` - Decode from base64

## Example Usage

```vitte
use std/net
use std/net.{socket, address, protocol}

entry main at core/app {
  net.init()
  net.configure_dhcp()
  
  let ip = net.get_ip()
  
  let sock = socket.tcp_listen(8080)
  
  return 0
}
```
