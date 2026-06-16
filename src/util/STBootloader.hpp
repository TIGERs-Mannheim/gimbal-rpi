#pragma once

#include "SerialPort.hpp"
#include <atomic>
#include <thread>

class STBootloader
{
public:
    enum Result
    {
        RESULT_OK = 0,
        RESULT_WRITE_ERROR,
        RESULT_READ_ERROR,
        RESULT_TIMEOUT,
        RESULT_NACK,
        RESULT_INVALID_ARGUMENT,
        RESULT_ABORTED,
    };

    struct FlashResult
    {
        bool isUpdateRequired = false;
        bool isUpdated = false;
        uint32_t programSize = 0;
        uint32_t numBlocks = 0;
        uint32_t time_us = 0;
        uint32_t programCrc = 0;
    };

    STBootloader(std::shared_ptr<ISerialPort> pSerialPort, const std::string& pinReset, const std::string& pinBoot0);
    ~STBootloader();

    void startUpdate(const std::string& fwFilename);
    bool isActive() { return runFlag_.load(); }

private:
    void doUpdate(std::string fwFilename);
    void enterBootloader();
    Result sendReceive(size_t txSize, size_t expectedRxSize, size_t attempts);
    Result startBootloader();
    uint8_t xorChecksum(const uint8_t* pData, size_t length);
    Result cmdGet(uint8_t& blVersion);
    Result cmdGetId(uint16_t& productId);
    Result cmdReadMemory(uint32_t address, uint8_t* pData, size_t length);
    Result cmdMassErase();
    Result cmdWriteMemory(uint32_t address, uint8_t* pData, size_t length);
    Result cmdGo(uint32_t address);
    Result flash(const uint8_t* pProgram, uint32_t programSize, uint32_t targetAddress, uint32_t targetSize, bool forceUpdate, FlashResult& flashResult);

    std::shared_ptr<ISerialPort> pSerialPort_;
    std::string pinReset_;
    std::string pinBoot0_;

    std::thread updateThread_;
    std::atomic<bool> runFlag_ = false;

    static constexpr size_t STBOOTLOADER_BUFFER_SIZE = 130;

    uint8_t tx_[STBOOTLOADER_BUFFER_SIZE]{};
    uint8_t rx_[STBOOTLOADER_BUFFER_SIZE]{};
    uint8_t dataBlock_[128]{};

    FlashResult flashResult_;
};
