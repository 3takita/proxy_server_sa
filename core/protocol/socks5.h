#ifndef PROXY_SERVER_CORE_PROTOCOL_SOCKS5_H_
#define PROXY_SERVER_CORE_PROTOCOL_SOCKS5_H_

#include "protocol.h"

#include <cstdint>
#include <string>

namespace core::protocol {
    
    // https://en.wikipedia.org/wiki/SOCKS#SOCKS5
    class Socks5 final : public Protocol {
    public:
        enum class Phase : uint8_t {
            Greeting = 0,
            MethodSelected,
            AwaitingRequest,
            Connected
        };

        enum class Auth : uint16_t {
            NoAuth                                  = 0x00,
            GSSAPI                                  = 0x01, // https://www.rfc-editor.org/rfc/rfc1961
            UsernamePassword                        = 0x02, // https://www.rfc-editor.org/rfc/rfc1929
            ChallengeHandshakeAuthenticationMethod  = 0x03,
            Unassigned                              = 0x04,
            ChallengeResponseAuthenticationProtocol = 0x05,
            SecureSocketsLayer                      = 0x06,
            NDSAuthentication                       = 0x07,
            MultiAuthenticationFramework            = 0x08,
            JSONParameterBlock                      = 0x09
            // Everything else is unassigned or reserved
        };

        const char* Name() const override {return "SOCKS5"; }

        void OnReadable(Connection& connection, EventPollerIdentifier poller) override;
        void OnWritable(Connection& connection, EventPollerIdentifier poller) override;
    
    private:
        Phase phase_{Phase::Greeting};
        std::string destination_host_;
        uint16_t destination_port_{0};
    };

} // namespace core::protocol

#endif // PROXY_SERVER_CORE_PROTOCOL_SOCKS5_H_