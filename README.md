# Forward Proxy Server
A lightweight C++ proxy server supporting SOCKS4 and SOCKS4a. Designed for performance and OS portablity, with plans to support SOCKS5 and HTTP/HTTPS in future releases.

## Contributors
- Chris Manlove
- Ellie Winter
- Stephen Anaba
- Benjamin Lac
- Yordy Raya Sanchez

## Language 
C++ 20

## Supported Operating Systems
- Linux
- MacOS
- Windows (Future plans)

## Build & Run Instructions

```
git clone https://github.com/DrChrisHax/Proxy_Server.git
cd Proxy_Server
make run
```

This will start the server on port 8080.

## Server Argument Info

--port N	TCP port to listen on (default: 8080)
--backlog N	Maximum number of pending connections (default: 128)
--max-events N	Maximum number of events returned by epoll (default: 16)
--bind-host IP	IP address/interface to bind (default: all interfaces)
--verbose	Enable verbose logging
--help	Show usage information

## About
The proxy listens on a TCP socket, detects the incoming protocol, and establishes an upstream connection for traffic relay.

### Supported:
  - Socks4 / Socks4a
### Planned:
  - Socks5
  - HTTP/HTTPS
  - Windows support

## Additional Info
This project follows the App-Core model. Code related specifically to the proxy server is in a library called Server. This library is OS agnostic and links against the Core library. Core contains OS specific code as well as common functions that could potentially be user in a proxy server client. The Core library does not link against the Server library.


