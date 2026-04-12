#include "SerialPort.hpp"
#include <stdexcept>

unsigned int SerialPortOptions::getRateUInt() const
{
    switch(baudRate)
    {
        case RATE_1200:    return 1200;
        case RATE_1800:    return 1800;
        case RATE_2400:    return 2400;
        case RATE_4800:    return 4800;
        case RATE_9600:    return 9600;
        case RATE_19200:   return 19200;
        case RATE_38400:   return 38400;
        case RATE_57600:   return 57600;
        case RATE_115200:  return 115200;
        case RATE_128000:  return 128000;
        case RATE_230400:  return 230400;
        case RATE_256000:  return 256000;
        case RATE_460800:  return 460800;
        case RATE_500000:  return 500000;
        case RATE_576000:  return 576000;
        case RATE_921600:  return 921600;
        case RATE_1000000: return 1000000;
        case RATE_1500000: return 1500000;
        case RATE_2000000: return 2000000;
        default: return 0;
    }
}

std::string SerialPortOptions::getMode() const
{
    return std::string(1, getDataBitsChar()) + std::string(1, getParityChar()) + std::string(1, getStopBitsChar());
}

char SerialPortOptions::getDataBitsChar() const
{
    switch(dataBits)
    {
        case D5: return '5';
        case D6: return '6';
        case D7: return '7';
        case D8: return '8';
        default: return '?';
    }
}

char SerialPortOptions::getParityChar() const
{
    switch(parity)
    {
        case NONE: return 'N';
        case EVEN: return 'E';
        case ODD: return 'O';
        default: return '?';
    }
}

char SerialPortOptions::getStopBitsChar() const
{
    switch(stopBits)
    {
        case S1: return '1';
        case S2: return '2';
        default: return '?';
    }
}

bool SerialPortOptions::getModeFromString(std::string mode, DataBits& dataBits, Parity& parity, StopBits& stopBits)
{
    if(mode.size() < 3)
        return false;

    return getDataBitsFromChar(mode[0], dataBits) && getParityFromChar(mode[1], parity) && getStopBitsFromChar(mode[2], stopBits);
}

bool SerialPortOptions::getRateFromString(std::string rateString, Rate& rateEnum)
{
    try
    {
        unsigned int rateNumber = std::stoul(rateString);
        return getRateFromNumber(rateNumber, rateEnum);
    }
    catch(std::logic_error&)
    {
        return false;
    }
}

bool SerialPortOptions::getRateFromNumber(unsigned int rateNumber, Rate& rateEnum)
{
    switch(rateNumber)
    {
        case 1200:    rateEnum = RATE_1200; break;
        case 1800:    rateEnum = RATE_1800; break;
        case 2400:    rateEnum = RATE_2400; break;
        case 4800:    rateEnum = RATE_4800; break;
        case 9600:    rateEnum = RATE_9600; break;
        case 19200:   rateEnum = RATE_19200; break;
        case 38400:   rateEnum = RATE_38400; break;
        case 57600:   rateEnum = RATE_57600; break;
        case 115200:  rateEnum = RATE_115200; break;
        case 128000:  rateEnum = RATE_128000; break;
        case 230400:  rateEnum = RATE_230400; break;
        case 256000:  rateEnum = RATE_256000; break;
        case 460800:  rateEnum = RATE_460800; break;
        case 500000:  rateEnum = RATE_500000; break;
        case 576000:  rateEnum = RATE_576000; break;
        case 921600:  rateEnum = RATE_921600; break;
        case 1000000: rateEnum = RATE_1000000; break;
        case 1500000: rateEnum = RATE_1500000; break;
        case 2000000: rateEnum = RATE_2000000; break;
        default: return false;
    }

    return true;
}

bool SerialPortOptions::getParityFromChar(char c, Parity& parity)
{
    switch(c)
    {
        case 'n':
        case 'N':
            parity = NONE;
            return true;
        case 'e':
        case 'E':
            parity = EVEN;
            return true;
        case 'o':
        case 'O':
            parity = ODD;
            return true;
    }

    return false;
}

bool SerialPortOptions::getDataBitsFromChar(char c, DataBits& dataBits)
{
    switch(c)
    {
        case '5': dataBits = D5; return true;
        case '6': dataBits = D6; return true;
        case '7': dataBits = D7; return true;
        case '8': dataBits = D8; return true;
    }

    return false;
}

bool SerialPortOptions::getStopBitsFromChar(char c, StopBits& stopBits)
{
    switch(c)
    {
        case '1': stopBits = S1; return true;
        case '2': stopBits = S2; return true;
    }

    return false;
}
