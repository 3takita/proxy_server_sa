#ifndef PROXY_SERVER_CORE_PROTOCOL_PROTOCOL_H_
#define PROXY_SERVER_CORE_PROTOCOL_PROTOCOL_H_

#include "network.h"

// Forward declarations
namespace core {
    class Connection;
}


namespace core::protocol {

    class Protocol {
    public:
        virtual ~Protocol() = default;

        // A short name 
        virtual const char* Name() const = 0;

        // Called when the socket associated with connection is readable
        // Implementation may consume bytes from connection.receive_buffer, and/or
        // append bytes to connection.send_buffer and set connection.want_write = true.
        // They may also register/unregister/update interests via poller as needed
        virtual void OnReadable(Connection& connection,
                                EventPollerIdentifier poller) = 0;

        // Called when the socket associated with connection is writable.
        // Implementation relys on the server to drain the send_buffer
        virtual void OnWritable(Connection& connection,
                                EventPollerIdentifier poller) = 0;
    };

} // namespace core::protocol

#endif // PROXY_SERVER_CORE_PROTOCOL_PROTOCOL_H_