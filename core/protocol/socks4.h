#ifndef PROXY_SERVER_CORE_PROTOCOL_SOCKS4_H_
#define PROXY_SERVER_CORE_PROTOCOL_SOCKS4_H_

#include "protocol.h"

#include <cstdint>
#include <cstddef>

namespace core::protocol {


    class Socks4 : public Protocol {
    public:

        enum class Command : uint8_t {
            StreamConnection = 0x01,
            PortBinding = 0x02
        };

        enum class State : uint8_t {
            Init = 0,
            Established,
            Failed
        };

        const char* Name() const override {return "SOCKS4"; }

        void OnReadable(Connection& connection, EventPollerIdentifier poller) override;
        void OnWritable(Connection& connection, EventPollerIdentifier poller) override;

        bool IsEstablished() const;

    private:
        State state_{State::Init};

        uint16_t destination_port_; // network byte order
        std::byte destination_ip_[4]; // network byte order
        std::string user_id_;
    };

} // namespace core::protocol

#endif // PROXY_SERVER_CORE_PROTOCOL_SOCKS4_H_