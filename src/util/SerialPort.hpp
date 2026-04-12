#pragma once

#include <vector>
#include <string>
#include <memory>
#include "IoPort.hpp"

enum class SerialPortGPIO
{
    RTS = 1,
    DTR,
    BRK,
};

class ISerialPort : public IIoPort
{
public:
    virtual ~ISerialPort() {}

    virtual void flush() =0;
    virtual int write(const void* buf, int len) =0;
    virtual int read(void* buf, int len) =0;
    virtual int gpio(SerialPortGPIO gpio, bool level) =0;
};

class SerialPortOptions
{
public:
    enum Parity { NONE, EVEN, ODD };
    enum DataBits { D5, D6, D7, D8 };

    enum Rate {
        RATE_1200,
        RATE_1800,
        RATE_2400,
        RATE_4800,
        RATE_9600,
        RATE_19200,
        RATE_38400,
        RATE_57600,
        RATE_115200,
        RATE_128000,
        RATE_230400,
        RATE_256000,
        RATE_460800,
        RATE_500000,
        RATE_576000,
        RATE_921600,
        RATE_1000000,
        RATE_1500000,
        RATE_2000000,
    };

    enum StopBits { S1, S2 };

    Parity parity;
    DataBits dataBits;
    Rate baudRate;
    StopBits stopBits;

    unsigned int getRateUInt() const;
    std::string getMode() const;
    char getDataBitsChar() const;
    char getParityChar() const;
    char getStopBitsChar() const;

    static bool getModeFromString(std::string mode, DataBits& dataBits, Parity& parity, StopBits& stopBits); // a string like "8N1"
    static bool getRateFromString(std::string rateString, Rate& rateEnum); // string must only be a number and match enum exactly
    static bool getRateFromNumber(unsigned int rateNumber, Rate& rateEnum); // Number must match an enum exactly
    static bool getParityFromChar(char c, Parity& parity); // 'N', 'E', or 'O'
    static bool getDataBitsFromChar(char c, DataBits& dataBits); // '5', '6', '7', or '8'
    static bool getStopBitsFromChar(char c, StopBits& stopBits); // '1' or '2'
};

class SerialPortFactory
{
public:
    static std::vector<std::string> listDevices();
    static std::shared_ptr<ISerialPort> open(std::string device, SerialPortOptions options);
};
