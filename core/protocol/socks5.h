#ifndef PROXY_SERVER_CORE_PROTOCOL_SOCKS5_H_
#define PROXY_SERVER_CORE_PROTOCOL_SOCKS5_H_

    #include "protocol.h"

    #include <cstdint>
    #include <string>

    namespace core::protocol {
        
        class Socks5 final : public Protocol {
        public:
            enum class Phase : uint8_t {
                Greeting = 0,
                MethodSelected,
                AwaitingRequest,
                Connected
            };

            const char* Name() const override {return "SOCKS5"; }

            void OnReadable(Connection& connection,
                            EventPollerIdentifier poller) override;

            void OnWritable(Connection& connection,
                            EventPollerIdentifier poller) override;
        
        private:
            Phase phase_{Phase::Greeting};
            std::string destination_host_;
            uint16_t destination_port_{0};
        };

    } // namespace core::protocol

#endif // PROXY_SERVER_CORE_PROTOCOL_SOCKS5_H_