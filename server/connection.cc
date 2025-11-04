#include "server/connection.h"

#include <sys/socket.h>

namespace server {

    void AcceptNewConnections(core::SocketIdentifier listen_socket_id,
                              core::EventPollerIdentifier poller,
                              ConnectionMap& clients);
    

    void OnClientRead(core::SocketIdentifier id, ConnectionMap& clients);

    void CloseAndRemove(core::SocketIdentifier id, ConnectionMap& clients);


} // namespace server