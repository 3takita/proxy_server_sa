#ifndef PROXY_SERVER_CORE_PROTOCOL_PROTOCOL_H_
#define PROXY_SERVER_CORE_PROTOCOL_PROTOCOL_H_

#include "network.h"

// Forward declarations
namespace core {
    class Connection;
}


namespace core::protocol {

    enum ProtocolType : uint8_t {
        kUnknown = 0,
        kSocks4,
        kSocks4a,
        kSocks5,
        kHttp,
        kUnsupported
    };

    class Protocol {
    public:
        virtual ~Protocol() = default;

        virtual ProtocolType type() const noexcept = 0;

        // Called when the socket associated with connection is readable
        // Implementation may consume bytes from connection.receive_buffer, and/or
        // append bytes to connection.send_buffer and set connection.want_write = true.
        // They may also register/unregister/update interests via poller as needed
        // Self is the connection that triggered the event
        // Peer is paired with self, this can be nullptr
        virtual void OnReadable(Connection* self, Connection* peer, EventPollerIdentifier poller) = 0;

        // Called when the socket associated with connection is writable.
        // Implementation relys on the server to drain the send_buffer
        // Self is the connection that triggered the event
        // Peer is paired with self, this can be nullptr
        virtual void OnWritable(Connection* self, Connection* peer, EventPollerIdentifier poller) = 0;
    };

} // namespace core::protocol

#endif // PROXY_SERVER_CORE_PROTOCOL_PROTOCOL_H_