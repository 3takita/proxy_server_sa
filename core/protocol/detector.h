#ifndef PROXY_SERVER_CORE_PROTOCOL_DETECTOR_H_
#define PROXY_SERVER_CORE_PROTOCOL_DETECTOR_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace core::protocol {

    enum class ProtocolType : uint8_t {
        Unknown = 0,
        Socks4,
        Socks4a,
        Socks5,
        Http,
        Unsupported
    };

    class Detector {
    public:

        bool ProbeProtocol(const std::vector<std::byte>& buf, ProtocolType* out_type) const;

    private:
        
        inline bool IsSocks4(const std::vector<std::byte>& buf) const;
        inline bool IsSocks4a(const std::vector<std::byte>& buf) const;
        inline bool IsSocks5(const std::vector<std::byte>& buf) const;

        bool IsHttp(const std::vector<std::byte>& buf) const;

    };

    // This attempts to figure out which protocol the client sent to the server
    // It returns true if buf has enough bytes, and false if buf does not have
    // enough data
    

} // namespace protocol


#endif // PROXY_SERVER_CORE_PROTOCOL_DETECTOR_H_