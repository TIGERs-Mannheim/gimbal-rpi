#include "StepperAxis.hpp"

StepperAxis::StepperAxis(std::shared_ptr<ISerialPort> pSerialPort, uint8_t address, const std::string& pinStep, const std::string& pinDir)
: driver_(pSerialPort, address),
  pinStep_(pinStep.c_str(), RPiGPIO::OUTPUT),
  pinDir_(pinDir.c_str(), RPiGPIO::OUTPUT)
{
}

void StepperAxis::step(bool dir)
{
    lastStepValue_ = !lastStepValue_;

    pinDir_.write(dir);
    pinStep_.write(lastStepValue_);
}
