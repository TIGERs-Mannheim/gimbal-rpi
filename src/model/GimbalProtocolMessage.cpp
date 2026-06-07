#include "GimbalProtocolMessage.hpp"
#include "cobs.h"
#include "crc.h"

namespace gimbal_protocol
{

Message::Message(const GimbalMsgHeader& header, const std::vector<uint8_t>& payload)
:isValid_(true),
 header_(header),
 data_(payload)
{
    createTime_ = clock_t::now();
}

Message::Message(uint16_t type, uint16_t version)
:isValid_(true)
{
    createTime_ = clock_t::now();

    header_.version = version;
    header_.type = type;
}

Message::Message(uint16_t type, const std::vector<uint8_t>& payload, uint16_t version)
:isValid_(true),
 data_(payload)
{
    createTime_ = clock_t::now();

    header_.version = version;
    header_.type = type;
}

Message Message::fromWireFormat(const std::vector<uint8_t>& wire)
{
    uint32_t decodedSize = 0;
    int16_t result = COBSDecode(wire.data(), wire.size(), 0, 0, &decodedSize);

    std::vector<uint8_t> decoded(decodedSize);

    // COBS frame complete => decode
    result = COBSDecode(wire.data(), wire.size(), decoded.data(), decoded.size(), &decodedSize);
    if(result || decodedSize < sizeof(GimbalMsgHeader) + sizeof(GimbalMsgTrailer))
    {
        // decoding failed, data is lost
        return Message();
    }

    const auto pHeader = reinterpret_cast<const GimbalMsgHeader*>(decoded.data());
    const auto pTrailer = reinterpret_cast<const GimbalMsgTrailer*>(decoded.data() + decodedSize - sizeof(GimbalMsgTrailer));

    uint32_t crc = CRC32CalcChecksum(decoded.data(), decodedSize - sizeof(GimbalMsgTrailer));
    if(crc != pTrailer->checksum)
    {
        // invalid checksum
        return Message();
    }

    return Message(*pHeader, std::vector<uint8_t>(decoded.begin() + sizeof(GimbalMsgHeader), decoded.begin() + decodedSize - sizeof(GimbalMsgTrailer)));
}

std::vector<uint8_t> Message::toWireFormat() const
{
    std::vector<uint8_t> packed(data_.size() + sizeof(GimbalMsgHeader) + sizeof(GimbalMsgTrailer));

    *reinterpret_cast<GimbalMsgHeader*>(packed.data()) = header_;
    std::ranges::copy(data_, packed.begin() + sizeof(GimbalMsgHeader));

    auto pTrailer = reinterpret_cast<GimbalMsgTrailer*>(packed.data() + data_.size() + sizeof(GimbalMsgHeader));

    pTrailer->checksum = CRC32CalcChecksum(packed.data(), data_.size() + sizeof(GimbalMsgHeader));

    std::vector<uint8_t> wire(COBSMaxStuffedSize(packed.size())+1);

    uint32_t bytesWritten = 0;
    COBSEncode(packed.data(), packed.size(), wire.data(), wire.size()-1, &bytesWritten);

    wire[bytesWritten++] = 0;

    wire.resize(bytesWritten);

    return wire;
}

}
