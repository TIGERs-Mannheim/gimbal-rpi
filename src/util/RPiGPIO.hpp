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

    enum Pull
    {
        PULL_NONE,
        PULL_UP,
        PULL_DOWN,
    };

    enum Event
    {
        EVENT_RSING,
        EVENT_FALLING,
    };

    RPiGPIO(const char* pName, Direction dir, Pull pull = PULL_NONE, bool isActiveLow = false);
    ~RPiGPIO();

    operator bool() const { return initSuccess_; }

    void write(bool on);
    bool read();
    bool getNextEvent(Event& event);

private:
    bool initSuccess_ = false;
    gpiod_chip* pChip_ = nullptr;
    gpiod_line_request* pLine_ = nullptr;
    gpiod_edge_event_buffer* pEventBuffer_ = nullptr;
    unsigned int lineNumber_ = 0;
};
