#include "STBootloader.hpp"
#include "RPiGPIO.hpp"
#include "crc.h"
#include "easylogging++.h"

#include <cstring>
#include <fstream>

using namespace std::chrono_literals;

STBootloader::STBootloader(std::shared_ptr<ISerialPort> pSerialPort, const std::string& pinReset, const std::string& pinBoot0)
: pSerialPort_(pSerialPort),
  pinReset_(pinReset),
  pinBoot0_(pinBoot0)
{
}

STBootloader::~STBootloader()
{
    runFlag_ = false;
    if(updateThread_.joinable())
        updateThread_.join();
}

void STBootloader::startUpdate(const std::string& fwFilename)
{
    if(isActive())
        return;

    runFlag_ = true;
    updateThread_ = std::thread(&STBootloader::doUpdate, this, fwFilename);
}

void STBootloader::doUpdate(std::string fwFilename)
{
    std::ifstream file(fwFilename, std::ios::binary);

    if(!file)
    {
        LOG(ERROR) << "Failed to open firmware file: " << fwFilename;
        runFlag_ = false;
        return;
    }

    auto fwData = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
    file.close();

    LOG(INFO) << "Checking firmware against: " << fwFilename;

    auto result = flash(fwData.data(), fwData.size(), 0x08000000, 256*1024, false, flashResult_);
    if(result == RESULT_OK)
    {
        if(flashResult_.isUpdated)
            LOG(INFO) << "Firmware update successful.";
        else
            LOG(INFO) << "Firmware already up-to-date.";
    }
    else
    {
        LOG(ERROR) << "Firmware update failed. Result: " << result;
    }

    runFlag_ = false;
}

void STBootloader::enterBootloader()
{
    {
        // Reset target and wait to ensure GPIOs are released
        RPiGPIO nreset(pinReset_.c_str(), RPiGPIO::OUTPUT);
        nreset.write(false);

        std::this_thread::sleep_for(50ms);

        // Set BOOT0 high
        RPiGPIO boot(pinBoot0_.c_str(), RPiGPIO::OUTPUT);
        boot.write(true);

        std::this_thread::sleep_for(50ms);

        // Release reset, target should run from system memory (embedded bootloader)
        nreset.write(true);

        std::this_thread::sleep_for(50ms);
    }

    // Briefly configure both pins as input to return them to default high-Z state
    RPiGPIO nreset(pinReset_.c_str(), RPiGPIO::INPUT);
    RPiGPIO boot(pinBoot0_.c_str(), RPiGPIO::INPUT);
}

STBootloader::Result STBootloader::sendReceive(size_t txSize, size_t expectedRxSize, size_t attempts)
{
    Result result = RESULT_OK;

    for(size_t attempt = 0; attempt < attempts; attempt++)
    {
        // Empty read/write queue
        pSerialPort_->flush();
        while(pSerialPort_->read(rx_, sizeof(rx_)) > 0)
        {
        }

        // Write data
        if(pSerialPort_->write(tx_, txSize) != txSize)
            return RESULT_WRITE_ERROR;

        if(expectedRxSize == 0)
            return RESULT_OK;

        // Polling read with timeout
        auto tStart = std::chrono::high_resolution_clock::now();
        size_t bytesRead = 0;
        while(std::chrono::high_resolution_clock::now() - tStart < 50ms)
        {
            auto readResult = pSerialPort_->read(rx_ + bytesRead, sizeof(rx_) - bytesRead);
            if(readResult < 0)
                return RESULT_READ_ERROR;

            bytesRead += readResult;

            if(bytesRead >= expectedRxSize)
                break;

            std::this_thread::sleep_for(1ms);
        }

        // Timeout and ACK check
        if(bytesRead < expectedRxSize)
        {
            result = RESULT_TIMEOUT;
            std::this_thread::sleep_for(2ms);
            continue;
        }

        if(rx_[0] != 0x79)
        {
            result = RESULT_NACK;
            continue;
        }

        return RESULT_OK;
    }

    return result;
}

STBootloader::Result STBootloader::startBootloader()
{
    auto result = RESULT_OK;

    // autobaud magic byte
    tx_[0] = 0x7F;

    for(uint8_t attempt = 0; attempt < 5; attempt++)
    {
        // Empty read/write queue
        pSerialPort_->flush();
        while(pSerialPort_->read(rx_, sizeof(rx_)) > 0)
        {
        }

        enterBootloader();

        // wait until remote bootloader is ready
        std::this_thread::sleep_for(10ms);

        result = sendReceive(1, 1, 1);
        if(result)
        {
            continue;
        }

        return RESULT_OK;
    }

    return result;
}

STBootloader::Result STBootloader::cmdGet(uint8_t& blVersion)
{
    tx_[0] = 0x00;
    tx_[1] = 0xFF;
    auto result = sendReceive(2, 15, 5);
    if(result)
        return result;

    blVersion = rx_[2];

    if(rx_[14] != 0x79)
        return RESULT_NACK;

    return RESULT_OK;
}

STBootloader::Result STBootloader::cmdGetId(uint16_t& productId)
{
    tx_[0] = 0x02;
    tx_[1] = 0xFD;
    auto result = sendReceive(2, 5, 5);
    if(result)
        return result;

    if(rx_[4] != 0x79)
        return RESULT_NACK;

    productId = ((uint16_t)rx_[2]) << 8 | rx_[3];

    return RESULT_OK;
}

uint8_t STBootloader::xorChecksum(const uint8_t* pData, size_t length)
{
    uint8_t checksum = 0;
    for(size_t i = 0; i < length; i++)
        checksum ^= pData[i];

    return checksum;
}

STBootloader::Result STBootloader::cmdReadMemory(uint32_t address, uint8_t* pData, size_t length)
{
    if(length > 128)
        return RESULT_INVALID_ARGUMENT;

    // send command
    tx_[0] = 0x11;
    tx_[1] = 0xEE;
    auto result = sendReceive(2, 1, 1);
    if(result)
        return result;

    // send address
    tx_[0] = (address >> 24) & 0xFF;
    tx_[1] = (address >> 16) & 0xFF;
    tx_[2] = (address >> 8) & 0xFF;
    tx_[3] = address & 0xFF;
    tx_[4] = xorChecksum(tx_, 4);
    result = sendReceive(5, 1, 1);
    if(result)
        return result;

    // send length
    tx_[0] = (length - 1);
    tx_[1] = (length - 1) ^ 0xFF;
    result = sendReceive(2, length + 1, 1);
    if(result)
        return result;

    if(rx_[0] != 0x79)
        return RESULT_NACK;

    memcpy(pData, &rx_[1], length);

    return RESULT_OK;
}

STBootloader::Result STBootloader::cmdMassErase()
{
    tx_[0] = 0x44;
    tx_[1] = 0xBB;
    auto result = sendReceive(2, 1, 1);
    if(result)
        return result;

    tx_[0] = 0xFF;
    tx_[1] = 0xFF;
    tx_[2] = 0x00;
    result = sendReceive(3, 1, 1);
    if(result)
        return result;

    return RESULT_OK;
}

STBootloader::Result STBootloader::cmdWriteMemory(uint32_t address, uint8_t* pData, size_t length)
{
    if(length > 128)
        return RESULT_INVALID_ARGUMENT;

    tx_[0] = 0x31;
    tx_[1] = 0xCE;
    auto result = sendReceive(2, 1, 1);
    if(result)
        return result;

    tx_[0] = (address >> 24) & 0xFF;
    tx_[1] = (address >> 16) & 0xFF;
    tx_[2] = (address >> 8) & 0xFF;
    tx_[3] = address & 0xFF;
    tx_[4] = xorChecksum(tx_, 4);
    result = sendReceive(5, 1, 1);
    if(result)
        return result;

    tx_[0] = length - 1;
    memcpy(&tx_[1], pData, length);
    tx_[length + 1] = xorChecksum(tx_, length + 1);
    result = sendReceive(length + 2, 1, 1);
    if(result)
        return result;

    return RESULT_OK;
}

STBootloader::Result STBootloader::cmdGo(uint32_t address)
{
    tx_[0] = 0x21;
    tx_[1] = 0xDE;
    auto result = sendReceive(2, 1, 1);
    if(result)
        return result;

    tx_[0] = (address >> 24) & 0xFF;
    tx_[1] = (address >> 16) & 0xFF;
    tx_[2] = (address >> 8) & 0xFF;
    tx_[3] = address & 0xFF;
    tx_[4] = xorChecksum(tx_, 4);
    result = sendReceive(5, 1, 1);
    if(result)
        return result;

    return RESULT_OK;
}

STBootloader::Result STBootloader::flash(const uint8_t* pProgram, uint32_t programSize, uint32_t targetAddress, uint32_t targetSize, bool forceUpdate, FlashResult& flashResult)
{
    Result result = RESULT_OK;

    auto tStart = std::chrono::high_resolution_clock::now();

    flashResult.isUpdateRequired = forceUpdate;
    flashResult.isUpdated = false;
    flashResult.programSize = 0;
    flashResult.numBlocks = 0;
    flashResult.time_us = 0;

    if(targetSize < programSize + 4)
        return RESULT_INVALID_ARGUMENT;

    result = startBootloader();
    if(result)
        return result;

    uint8_t blVersion = 0;
    result = cmdGet(blVersion);
    if(result)
        return result;

    uint16_t productId = 0;
    result = cmdGetId(productId);
    if(result)
        return result;

    if(!runFlag_)
        return RESULT_ABORTED;

    flashResult.programSize = programSize;
    flashResult.numBlocks = (programSize + 127) / 128;

    const uint32_t programCRC = CRC32CalcChecksum(pProgram, programSize);
    flashResult.programCrc = programCRC;

    if(!forceUpdate)
    {
        uint32_t targetCRC;
        result = cmdReadMemory(targetAddress + targetSize - 4, (uint8_t*)&targetCRC, 4);
        if(result)
            return result;

        flashResult.isUpdateRequired = programCRC != targetCRC;

        if(!flashResult.isUpdateRequired)
        {
            result = cmdGo(targetAddress);
            if(result)
                return result;

            flashResult.time_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - tStart).count();

            return RESULT_OK;
        }
    }

    result = cmdMassErase();
    if(result)
        return result;

    if(!runFlag_)
        return RESULT_ABORTED;

    uint32_t bytesLeft = programSize;

    while(bytesLeft > 0)
    {
        const uint32_t bytesWritten = programSize - bytesLeft;

        uint32_t writeSize = bytesLeft;
        if(writeSize > sizeof(dataBlock_))
            writeSize = sizeof(dataBlock_);
        else
            memset(dataBlock_, 0xFF, sizeof(dataBlock_));

        memcpy(dataBlock_, pProgram + bytesWritten, writeSize);

        result = cmdWriteMemory(targetAddress + bytesWritten, dataBlock_, sizeof(dataBlock_));
        if(result)
            return result;

        bytesLeft -= writeSize;

        if(!runFlag_)
            return RESULT_ABORTED;
    }

    result = cmdWriteMemory(targetAddress + targetSize - 4, (uint8_t*)&programCRC, 4);
    if(result)
        return result;

    result = cmdGo(targetAddress);
    if(result)
        return result;

    flashResult.isUpdated = true;
    flashResult.time_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - tStart).count();

    return RESULT_OK;
}
