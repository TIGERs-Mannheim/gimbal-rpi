#pragma once

#include <gpiod.h>

class RPiGPIO
{
public:
    enum Direction
    {
        OUTPUT,
        INPUT,
    };

    RPiGPIO(const char* pName, Direction dir);
    ~RPiGPIO();

    operator bool() const { return initSuccess_; }

    void write(bool on);
    bool read();

private:
    bool initSuccess_ = false;
    gpiod_chip* pChip_ = nullptr;
    gpiod_line_request* pLine_ = nullptr;
    unsigned int lineNumber_ = 0;
};
