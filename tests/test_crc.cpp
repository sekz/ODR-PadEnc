/*
    Tests for the CRC used to protect DAB data groups
*/

#include "check.h"
#include "crc.h"
#include <cstdint>
#include <cstring>
#include <vector>

static void test_crc16_check_value()
{
    // CRC-16/CCITT-FALSE check value for "123456789"
    const char* msg = "123456789";
    CHECK(odr::crc16(0xFFFF, msg, strlen(msg)) == 0x29B1);
}

static void test_data_group_crc_residue()
{
    // A data group is protected with the inverted CRC16, appended big endian
    // (see DATA_GROUP::AppendCRC). Running the CRC over data and CRC gives
    // a constant residue.
    std::vector<uint8_t> data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint16_t crc = ~odr::crc16(0xFFFF, data.data(), data.size());
    data.push_back((crc & 0xFF00) >> 8);
    data.push_back(crc & 0x00FF);
    CHECK(odr::crc16(0xFFFF, data.data(), data.size()) == 0x1D0F);

    // A corrupted byte is detected
    data[3] ^= 0x01;
    CHECK(odr::crc16(0xFFFF, data.data(), data.size()) != 0x1D0F);
}

int main()
{
    test_crc16_check_value();
    test_data_group_crc_residue();
    return 0;
}
