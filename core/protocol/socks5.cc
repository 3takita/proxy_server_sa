#include "socks5.h"
#include "connection.h"
#include <cstring>


namespace core::protocol {


    void Socks5::OnReadable(Connection& connection, ConnectionMap& all, EventPollerIdentifier poller) {
        (void)connection;
        (void)all;
        (void)poller;
    }

    void Socks5::OnWritable(Connection& connection, ConnectionMap& all, EventPollerIdentifier poller) {
        (void)connection;
        (void)all;
        (void)poller;
    }

} // namespace core::protocol