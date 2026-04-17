#include "SerialPort.hpp"

#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

class SerialPortPosix : public ISerialPort
{
public:
    SerialPortPosix(int comFd, struct termios oldtio):comFd_(comFd), oldtio_(oldtio) {}
    ~SerialPortPosix();

    void flush() override;
    int write(const void* buf, int len) override;
    int read(void* buf, int len) override;
    int gpio(SerialPortGPIO gpio, bool level) override;
    void lock() override { mutex_.lock(); }
    void unlock() override { mutex_.unlock(); };

private:
    int comFd_;
    struct termios oldtio_;
    std::mutex mutex_;
};

SerialPortPosix::~SerialPortPosix()
{
    flush();
    tcsetattr(comFd_, TCSANOW, &oldtio_);
    lockf(comFd_, F_ULOCK, 0);
    close(comFd_);
}

void SerialPortPosix::flush()
{
    tcflush(comFd_, TCIFLUSH);
}

int SerialPortPosix::write(const void* buf, int len)
{
    ssize_t r;
    const uint8_t* pos = (const uint8_t*)buf;
    int origLen = len;

    while(len)
    {
        r = ::write(comFd_, pos, len);
        if (r < 1)
            return -1;

        len -= r;
        pos += r;
    }

    return origLen - len;
}

int SerialPortPosix::read(void* buf, int len)
{
    ssize_t r;
    uint8_t* pos = (uint8_t*)buf;

    while(len)
    {
        r = ::read(comFd_, pos, len);
        if (r == 0)
            return pos - (uint8_t*)buf;

        if (r < 0)
            return -1;

        len -= r;
        pos += r;
    }

    return pos - (uint8_t*)buf;
}

int SerialPortPosix::gpio(SerialPortGPIO gpio, bool level)
{
    int bit;
    int lines;

    switch(gpio)
    {
        case SerialPortGPIO::RTS:
            bit = TIOCM_RTS;
            break;

        case SerialPortGPIO::DTR:
            bit = TIOCM_DTR;
            break;

        case SerialPortGPIO::BRK:
            if(level == 0)
                return 0;

            if(tcsendbreak(comFd_, 1))
                return -1;

            return 0;

        default:
            return -1;
    }

    // handle RTS/DTR
    if(ioctl(comFd_, TIOCMGET, &lines))
        return -1;

    lines = level ? lines | bit : lines & ~bit;
    if(ioctl(comFd_, TIOCMSET, &lines))
        return -1;

    return 0;
}

std::vector<std::string> SerialPortFactory::listDevices()
{
    std::vector<std::string> devices;

    DIR* dir;
    struct dirent* ent;

    if((dir = opendir("/dev/")) != NULL)
    {
        // print all the files and directories within directory
        while((ent = readdir(dir)) != NULL)
        {
            if(strstr(ent->d_name, "ttyUSB") || strstr(ent->d_name, "ttyAMA") || strstr(ent->d_name, "ttyS") || strstr(ent->d_name, "serial"))
            {
                devices.push_back(std::string("/dev/") + std::string(ent->d_name));
            }
        }
        closedir(dir);
    }

    return devices;
}

static bool configureTermios(struct termios* pTermios, const SerialPortOptions& options)
{
    speed_t port_baud;
    tcflag_t port_bits;
    tcflag_t port_parity;
    tcflag_t port_stop;

    switch(options.baudRate)
    {
        case SerialPortOptions::Rate::RATE_1200:    port_baud = B1200; break;
        case SerialPortOptions::Rate::RATE_1800:    port_baud = B1800; break;
        case SerialPortOptions::Rate::RATE_2400:    port_baud = B2400; break;
        case SerialPortOptions::Rate::RATE_4800:    port_baud = B4800; break;
        case SerialPortOptions::Rate::RATE_9600:    port_baud = B9600; break;
        case SerialPortOptions::Rate::RATE_19200:   port_baud = B19200; break;
        case SerialPortOptions::Rate::RATE_38400:   port_baud = B38400; break;
        case SerialPortOptions::Rate::RATE_57600:   port_baud = B57600; break;
        case SerialPortOptions::Rate::RATE_115200:  port_baud = B115200; break;
        case SerialPortOptions::Rate::RATE_230400:  port_baud = B230400; break;
#ifdef B460800
        case SerialPortOptions::Rate::RATE_460800:  port_baud = B460800; break;
#endif /* B460800 */
#ifdef B921600
        case SerialPortOptions::Rate::RATE_921600:  port_baud = B921600; break;
#endif /* B921600 */
#ifdef B500000
        case SerialPortOptions::Rate::RATE_500000:  port_baud = B500000; break;
#endif /* B500000 */
#ifdef B576000
        case SerialPortOptions::Rate::RATE_576000:  port_baud = B576000; break;
#endif /* B576000 */
#ifdef B1000000
        case SerialPortOptions::Rate::RATE_1000000: port_baud = B1000000; break;
#endif /* B1000000 */
#ifdef B1500000
        case SerialPortOptions::Rate::RATE_1500000: port_baud = B1500000; break;
#endif /* B1500000 */
#ifdef B2000000
        case SerialPortOptions::Rate::RATE_2000000: port_baud = B2000000; break;
#endif /* B2000000 */
        default: return false;
    }

    switch(options.dataBits)
    {
        case SerialPortOptions::DataBits::D5: port_bits = CS5; break;
        case SerialPortOptions::DataBits::D6: port_bits = CS6; break;
        case SerialPortOptions::DataBits::D7: port_bits = CS7; break;
        case SerialPortOptions::DataBits::D8: port_bits = CS8; break;
        default: return false;
    }

    switch(options.parity)
    {
        case SerialPortOptions::Parity::NONE: port_parity = 0; break;
        case SerialPortOptions::Parity::EVEN: port_parity = PARENB; break;
        case SerialPortOptions::Parity::ODD:  port_parity = PARENB | PARODD; break;
        default: return false;
    }

    switch(options.stopBits)
    {
        case SerialPortOptions::StopBits::S1: port_stop = 0;      break;
        case SerialPortOptions::StopBits::S2: port_stop = CSTOPB; break;
        default: return false;
    }

    /* reset the settings */
#ifndef __sun       /* Used by GNU and BSD. Ignore __SVR4 in test. */
cfmakeraw(pTermios);
#else /* __sun */
    pTermios->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR
                   | IGNCR | ICRNL | IXON);
    pTermios->c_oflag &= ~OPOST;
    pTermios->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    pTermios->c_cflag &= ~(CSIZE | PARENB);
    pTermios->c_cflag |= CS8;
#endif /* __sun */
#ifdef __QNXNTO__
    pTermios->c_cflag &= ~(CSIZE | IHFLOW | OHFLOW);
#else
    pTermios->c_cflag &= ~(CSIZE | CRTSCTS);
#endif
    pTermios->c_cflag &= ~(CSIZE | CRTSCTS);
    pTermios->c_iflag &= ~(IXON | IXOFF | IXANY | IGNPAR);
    pTermios->c_lflag &= ~(ECHOK | ECHOCTL | ECHOKE);
    pTermios->c_oflag &= ~(OPOST | ONLCR);

    /* setup the new settings */
    cfsetispeed(pTermios, port_baud);
    cfsetospeed(pTermios, port_baud);
    pTermios->c_cflag |=
        port_parity |
        port_bits   |
        port_stop   |
        CLOCAL      |
        CREAD;

    if(port_parity != 0)
        pTermios->c_iflag |= INPCK;

    pTermios->c_cc[VMIN] = 0;
    pTermios->c_cc[VTIME] = 0; // in units of 0.1s

    return true;
}

std::shared_ptr<ISerialPort> SerialPortFactory::open(std::string device, SerialPortOptions options)
{
    int comFd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if(comFd < 0)
        return nullptr;

    if(lockf(comFd, F_TLOCK, 0) != 0)
        return nullptr;

    fcntl(comFd, F_SETFL, 0);

    struct termios oldtio;
    struct termios newtio;

    tcgetattr(comFd, &oldtio);
    tcgetattr(comFd, &newtio);

    bool validOptions = configureTermios(&newtio, options);
    if(!validOptions)
    {
        lockf(comFd, F_ULOCK, 0);
        close(comFd);

        return nullptr;
    }

    // set the settings
    tcflush(comFd, TCIFLUSH);
    if(tcsetattr(comFd, TCSANOW, &newtio) != 0)
    {
        lockf(comFd, F_ULOCK, 0);
        close(comFd);

        return nullptr;
    }

    return std::make_shared<SerialPortPosix>(comFd, oldtio);
}
