#include "TMC2209.hpp"
#include "easylogging++.h"
#include <thread>

namespace tmc2209
{

Driver::Driver(std::shared_ptr<ISerialPort> pSerialPort, uint8_t address)
: pSerialPort_(pSerialPort),
  address_(address)
{
    velDepConf_.irun = 4; // => 0.28A
    velDepConf_.ihold = 4; // => 0.28A
    velDepConf_.iholddelay = 1;
    velDepConf_.tpowerdown = 20;
    velDepConf_.tpwmthrs = 0;

    chopConf_.dedge = 1;
    chopConf_.intpol = 1;
    chopConf_.mres = 4; // 16 microsteps per STEP pule (16 pulses for one full step)
    chopConf_.vsense = 0;
    chopConf_.tbl = 1; // 24 clock cycles blanking time
    chopConf_.hend = 0;
    chopConf_.hstart = 4;
    chopConf_.toff = 3;
    // chopConf_.toff = 0; // TODO: testing, disable all bridges

    pwmConf_.pwmlim = 12;
    pwmConf_.pwmreg = 8;
    pwmConf_.freewheel = 0;
    pwmConf_.pwmautograd = 1;
    pwmConf_.pwmautoscale = 1;
    pwmConf_.pwmfreq = 1; // f_pwm = 35.1kHz
    pwmConf_.pwmgrad = 30;
    pwmConf_.pwmofs = 250;

    ioThread_ = std::jthread(std::bind_front(&Driver::ioThread, this));
}

void Driver::ioThread(std::stop_token st)
{
    using namespace std::literals;

    while(!st.stop_requested())
    {
        // Wait for trigger or run every 100ms
        runTrigger_.try_acquire_for(100ms);

        if(!isConnected_)
        {
            writeReg(REG_NODECONF, (3 << 8)); // Set send delay to 3*8 bit times for multi-node mode
        }

        // Connect/Disconnect/Setup procedure
        uint32_t gstat;
        int16_t result = readReg(REG_GSTAT, &gstat);
        if (isConnected_ && result)
        {
            LOG(WARNING) << "TMC2209-" << static_cast<uint16_t>(address_) << ": Connection lost";
            isConnected_ = false;
        }

        if (!isConnected_ && result == 0)
        {
            LOG(INFO) << "TMC2209-" << static_cast<uint16_t>(address_) << ": Connected";

            if (gstat & GSTAT_RESET)
            {
                LOG(INFO) << "TMC2209-" << static_cast<uint16_t>(address_) << ": Was reset.";
            }

            LOG(INFO) << "TMC2209-" << static_cast<uint16_t>(address_) << ": Performing setup";
            setup();

            isConnected_ = true;
        }

        // Async high/low power setting
        if(doSetHighPower_)
        {
            doSetHighPower(true);
            doSetHighPower_ = false;
        }

        if(doSetLowPower_)
        {
            doSetHighPower(false);
            doSetLowPower_ = false;
        }

        // Regular reads
        if (isConnected_ && result == 0)
        {
            uint32_t pwmauto;
            result = readReg(REG_PWM_AUTO, &pwmauto);
            if (!result)
            {
                pwmAuto_.ofs = (pwmauto >> 0) & 0xFF;
                pwmAuto_.grad = (pwmauto >> 16) & 0xFF;
            }

            DrvStatus drvStatus;
            readDrvStatus(drvStatus);
            drvStatus_ = drvStatus;

            readPwmScale(pwmScale_);
        }
    }
}

void Driver::setHighPower(bool highPower)
{
    if(highPower)
        doSetHighPower_ = true;
    else
        doSetLowPower_ = true;

    runTrigger_.release();
}

uint8_t Driver::calcCrc(const uint8_t* pData, uint8_t datagramSize)
{
    uint8_t crc = 0;

    for(uint8_t i = 0; i < (datagramSize - 1); ++i)
    {
        uint8_t byte = pData[i];
        for(uint8_t j = 0; j < 8; ++j)
        {
            if((crc >> 7) ^ (byte & 0x01))
            {
                crc = (crc << 1) ^ 0x07;
            }
            else
            {
                crc = crc << 1;
            }

            byte = byte >> 1;
        }
    }

    return crc;
}

void Driver::writeReg(uint8_t reg, uint32_t value)
{
    uint8_t tx[8];

    tx[0] = 0x05;
    tx[1] = address_; // Node address, depends on MS pins
    tx[2] = reg | 0x80;
    tx[3] = (value >> 24) & 0xFF;
    tx[4] = (value >> 16) & 0xFF;
    tx[5] = (value >> 8) & 0xFF;
    tx[6] = value & 0xFF;
    tx[7] = calcCrc(tx, 8);

    SerialPortLock lock(pSerialPort_);

    pSerialPort_->write(tx, 8);
    pSerialPort_->flush();
}

int16_t Driver::readReg(uint8_t reg, uint32_t* pValue)
{
    uint8_t tx[4];
    uint8_t rx[12] = {};

    SerialPortLock lock(pSerialPort_);

    // Flush RX data
    while(pSerialPort_->read(rx, 1) > 0)
        ;

    *pValue = 0;

    // If there was a reception previously we need to wait until TMC releases UART line
    std::this_thread::sleep_for(std::chrono::microseconds(1000));

    // Prepare TX data and send
    tx[0] = 0x05;
    tx[1] = address_; // Node address, depends on MS pins
    tx[2] = reg;
    tx[3] = calcCrc(tx, 4);

    pSerialPort_->write(tx, 4);
    pSerialPort_->flush();

    // Read TX echo and response
    int bytesRead = readBytes(rx, 12);
    if(bytesRead < 12)
    {
        numRxTimeouts_++;
        return 1;
    }

    if(calcCrc(rx + 4, 8) != rx[11])
    {
        numRxCrcErrors_++;
        return 2;
    }

    *pValue = (uint32_t)rx[7] << 24 | (uint32_t)rx[8] << 16 | (uint32_t)rx[9] << 8 | (uint32_t)rx[10];

    return 0;
}

int Driver::readBytes(uint8_t* pData, int length, std::chrono::milliseconds timeout)
{
    auto tStart = clock_t::now();

    int bytesReceived = 0;

    while(clock_t::now() - tStart < timeout)
    {
        int bytesRead = pSerialPort_->read(pData + bytesReceived, length - bytesReceived);
        if(bytesRead < 0)
            return bytesRead;

        if(bytesRead == 0)
            continue;

        bytesReceived += bytesRead;

        if(bytesReceived == length)
            return bytesReceived;
    }

    return -1;
}

void Driver::doSetHighPower(bool highPower)
{
    if(highPower)
    {
        velDepConf_.irun = 9; // => 0.55A
        velDepConf_.ihold = 4; // => 0.28A
        // velDepConf_.irun = 13; // => 0.77A
        // velDepConf_.ihold = 6; // => 0.38A
    }
    else
    {
        velDepConf_.irun = 4; // => 0.28A
        velDepConf_.ihold = 4; // => 0.28A
    }

    uint32_t iHoldRun = velDepConf_.ihold << 0 | velDepConf_.irun << 8 | velDepConf_.iholddelay << 16;
    writeReg(REG_IHOLD_IRUN, iHoldRun);
}

void Driver::setup()
{
    auto getIfcnt = [&]() -> uint8_t
    {
        uint32_t ifCnt;
        readReg(REG_IFCNT, &ifCnt);
        return ifCnt & 0xFF;
    };

    uint8_t ifcnt = getIfcnt();

    auto validateIfcnt = [&](const char* pOp)
    {
        uint8_t newCnt = getIfcnt();
        if(newCnt != ifcnt+1)
            LOG(WARNING) << "Register write failure during: " << pOp;

        ifcnt = newCnt;
    };

    writeReg(REG_GSTAT, GSTAT_RESET); // clear reset bit
    validateIfcnt("GSTAT");

    // Disable PDN function, microstep resolution defined by MRES register
    uint32_t gconf = GCONF_PDN_DISABLE | GCONF_MSTEP_REG_SELECT;
    writeReg(REG_GCONF, gconf);
    validateIfcnt("GCONF");

    uint32_t iHoldRun = velDepConf_.ihold << 0 | velDepConf_.irun << 8 | velDepConf_.iholddelay << 16;
    writeReg(REG_IHOLD_IRUN, iHoldRun);
    validateIfcnt("IHOLD_IRUN");

    writeReg(REG_TPOWERDOWN, velDepConf_.tpowerdown);
    validateIfcnt("TPOWERDOWN");

    writeReg(REG_TPWMTHRS, velDepConf_.tpwmthrs);
    validateIfcnt("TPWMTHRS");

    uint32_t chopconf = 0;
    chopconf |= chopConf_.dedge << 29;
    chopconf |= chopConf_.intpol << 28;
    chopconf |= chopConf_.mres << 24;
    chopconf |= chopConf_.vsense << 17;
    chopconf |= chopConf_.tbl << 15;
    chopconf |= chopConf_.hend << 7;
    chopconf |= chopConf_.hstart << 4;
    chopconf |= chopConf_.toff << 0;
    writeReg(REG_CHOPCONF, chopconf);
    validateIfcnt("CHOPCONF");

    uint32_t pwmconf = 0;
    pwmconf |= pwmConf_.pwmlim << 28;
    pwmconf |= pwmConf_.pwmreg << 24;
    pwmconf |= pwmConf_.freewheel << 20;
    pwmconf |= pwmConf_.pwmfreq << 16;
    pwmconf |= pwmConf_.pwmgrad << 8;
    pwmconf |= pwmConf_.pwmofs << 0;
    writeReg(REG_PWMCONF, pwmconf);
    validateIfcnt("PWMCONF");

    if(pwmConf_.pwmautograd || pwmConf_.pwmautoscale)
    {
        pwmconf |= pwmConf_.pwmautograd << 19;
        pwmconf |= pwmConf_.pwmautoscale << 18;
        writeReg(REG_PWMCONF, pwmconf);
        validateIfcnt("PWMCONF2");
    }
}

void Driver::readDrvStatus(DrvStatus& stat)
{
    uint32_t value;
    int16_t result = readReg(REG_DRV_STATUS, &value);
    if (result)
        return;

    stat.stst = value >> 31;
    stat.stealth = (value >> 30) & 0x01;
    stat.csactual = (value >> 16) & 0x1F;
    stat.t157 = (value >> 11) & 0x01;
    stat.t150 = (value >> 10) & 0x01;
    stat.t143 = (value >> 9) & 0x01;
    stat.t120 = (value >> 8) & 0x01;
    stat.olb = (value >> 7) & 0x01;
    stat.ola = (value >> 6) & 0x01;
    stat.s2vsb = (value >> 5) & 0x01;
    stat.s2vsa = (value >> 4) & 0x01;
    stat.s2gb = (value >> 3) & 0x01;
    stat.s2ga = (value >> 2) & 0x01;
    stat.ot = (value >> 1) & 0x01;
    stat.otpw = (value >> 0) & 0x01;
}

void Driver::readPwmScale(PwmScale& scale)
{
    uint32_t value;
    int16_t result = readReg(REG_PWM_SCALE, &value);
    if (result)
        return;

    scale.scalesum = value & 0xFF;
    scale.scaleauto = (int32_t)(value << 7) >> 23;
}

} //namespace tmc2209