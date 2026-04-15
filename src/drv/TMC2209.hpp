#pragma once

#include "SerialPort.hpp"
#include <chrono>

namespace tmc2209
{

enum Register
{
    REG_GCONF = 0x00,
    REG_GSTAT = 0x01,
    REG_IFCNT = 0x02,
    REG_NODECONF = 0x03,
    REG_IOIN = 0x06,
    REG_FACTORY_CONF = 0x07,
    REG_IHOLD_IRUN = 0x10,
    REG_TPOWERDOWN = 0x11,
    REG_TSTEP = 0x12,
    REG_TPWMTHRS = 0x13,
    REG_CHOPCONF = 0x6C,
    REG_DRV_STATUS = 0x6F,
    REG_PWMCONF = 0x70,
    REG_PWM_SCALE = 0x71,
    REG_PWM_AUTO = 0x72,
};

enum GSTAT
{
    GSTAT_RESET = (1 << 0),
    GSTAT_DRVERR = (1 << 1),
    GSTAT_UV_CP = (1 << 2),
};

enum GCONF
{
    GCONF_I_SCALE_ANALOG = (1 << 0),
    GCONF_EN_SPREAD_CYCLE = (1 << 2),
    GCONF_SHAFT = (1 << 3),
    GCONF_PDN_DISABLE = (1 << 6),
    GCONF_MSTEP_REG_SELECT = (1 << 7),
};

struct VelDepConf
{
    uint32_t ihold;         // 0..31 => Standstill current
    uint32_t irun;          // 0..31 => Motor run current
    uint32_t iholddelay;    // 0..15 => Smooth power down after motor hold
    uint32_t tpowerdown;    // 0..255 => 0..5.6s from motor standstill to power down
    uint32_t tpwmthrs;      // Upper velocity limit for StealthChop, 0 => Switch to SpreadCycle disabled
};

struct ChopConf
{
    uint32_t dedge;         // 0..1 => Enable step impulse at each step edge to reduce step frequency requirement
    uint32_t intpol;        // 0..1 => Interpolation to 256 microsteps
    uint32_t mres;          // 0..8 => 1..256 microsteps per STEP pulse
    uint32_t vsense;        // 0..1 => low-sense (0) or high-sense (1) resistor. V_sense_max = 325mV or 180mV respectively.
    uint32_t tbl;           // 0..3 => comparator blanking time
    uint32_t hend;          // 0..15 => Hysteresis low value
    uint32_t hstart;         // 0..7 => Hysteresis start value
    uint32_t toff;          // 0..15 => Slow decay phase duration
};

struct PwmConf
{
    uint32_t pwmlim;        // 0..15 => Limit for PWM_SCALE_AUTO when switching back from SpreadCycle to StealthChop. Default = 12.
    uint32_t pwmreg;        // 0..15 => Regulation loop gradient
    uint32_t freewheel;     // 0..3 => normal, freewheel, LS short, HS short
    uint32_t pwmautograd;   // 0..1 => PWM automatic gradient adaptation
    uint32_t pwmautoscale;  // 0..1 => PWM automatic amplitude scaling
    uint32_t pwmfreq;       // 0..3 => PWM frequency selection
    uint32_t pwmgrad;       // 0..255 => User defined amplitude gradient
    uint32_t pwmofs;        // 0..255 => User defined amplitude (offset)
};

struct DrvStatus
{
    uint32_t stst;          // 0..1 => Standstill indicator
    uint32_t stealth;       // 0..1 => StealthChop indicator
    uint32_t csactual;      // 0..31 => Actual motor current
    uint32_t t157;          // Temperature thresholds exceeded
    uint32_t t150;
    uint32_t t143;
    uint32_t t120;
    uint32_t olb;           // 0..1 => Open load indicators
    uint32_t ola;
    uint32_t s2vsb;         // 0..1 => low-side short indicators
    uint32_t s2vsa;
    uint32_t s2gb;          // 0..1 => short to ground indicators
    uint32_t s2ga;
    uint32_t ot;            // 0..1 => Overtemperature warning
    uint32_t otpw;          // 0..1 => Overtemperature pre-warning
};

struct PwmAuto
{
    uint32_t ofs;
    uint32_t grad;
};

struct PwmScale
{
    uint32_t scalesum;
    int32_t scaleauto;
};

class Driver
{
public:
    Driver(std::shared_ptr<ISerialPort> pSerialPort, uint8_t address = 0);

    void spinOnce();
    bool isConnected() const { return isConnected_; }

    DrvStatus getDrvStatus() const { return drvStatus_; }

    void setHighPower(bool highPower);

private:
    using clock_t = std::chrono::steady_clock;

    uint8_t calcCrc(const uint8_t* pData, uint8_t datagramSize);
    void writeReg(uint8_t reg, uint32_t value);
    int16_t readReg(uint8_t reg, uint32_t* pValue);
    int readBytes(uint8_t* pData, int length, std::chrono::milliseconds timeout = std::chrono::milliseconds(10));
    void setup();
    void readDrvStatus(DrvStatus& stat);
    void readPwmScale(PwmScale& scale);

    std::shared_ptr<ISerialPort> pSerialPort_;
    const uint8_t address_ = 0;

    bool isConnected_ = false;
    clock_t::time_point tLastConnectionCheck_;

    uint32_t numRxTimeouts_ = 0;
    uint32_t numRxCrcErrors_ = 0;

    VelDepConf velDepConf_ {};
    ChopConf chopConf_ {};
    PwmConf pwmConf_ {};

    PwmAuto pwmAuto_ {};
    DrvStatus drvStatus_ {};
    PwmScale pwmScale_ {};
};

} //namespace tmc2209
