#pragma once

#include "drv/TMC2209.hpp"
#include "RPiGPIO.hpp"

class StepperAxis
{
public:
    StepperAxis(std::shared_ptr<ISerialPort> pSerialPort, uint8_t address, const std::string& pinStep, const std::string& pinDir);

    void step(bool dir);

    tmc2209::Driver& getDriver() { return driver_; }
    const tmc2209::Driver& getDriver() const { return driver_; }

private:
    tmc2209::Driver driver_;
    RPiGPIO pinStep_;
    RPiGPIO pinDir_;

    bool lastStepValue_ = false;
};
