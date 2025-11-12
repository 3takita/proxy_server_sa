#include "socks5.h"
#include "connection.h"
#include <cstring>


namespace core::protocol {


    void Socks5::OnReadable(Connection& connection, EventPollerIdentifier poller) {
        (void)connection;
        (void)poller;
    }

    void Socks5::OnWritable(Connection& connection, EventPollerIdentifier poller) {
        (void)connection;
        (void)poller;
    }

} // namespace core::protocol