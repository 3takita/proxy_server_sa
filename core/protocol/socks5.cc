#include "socks5.h"
#include "connection.h"
#include <cstring>


namespace core::protocol {


    void Socks5::OnReadable(Connection* self, Connection* peer, EventPollerIdentifier poller) {
        (void)self;
        (void)peer;
        (void)poller;
    }

    void Socks5::OnWritable(Connection* self, Connection* peer, EventPollerIdentifier poller) {
        (void)self;
        (void)peer;
        (void)poller;
    }

} // namespace core::protocol