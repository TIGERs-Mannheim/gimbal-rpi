#include "RPiGPIO.hpp"
#include "easylogging++.h"
#include <optional>

RPiGPIO::RPiGPIO(const char* pName, Direction dir, Pull pull, bool isActiveLow)
{
    const char* pGpioChipPaths[] = { "/dev/gpiochip0", "/dev/gpiochip1", "/dev/gpiochip2" };
    int result = 0;

    for(const auto pChipPath : pGpioChipPaths)
    {
        pChip_ = gpiod_chip_open(pChipPath);
        if(!pChip_)
        {
            LOG(ERROR) << "Failed to open GPIO chip at location: " << pChipPath;
            continue;
        }

        auto pChipInfo = gpiod_chip_get_info(pChip_);
        if(!pChipInfo)
        {
            LOG(ERROR) << "Failed to get GPIO chip info at location: " << pChipPath;
            continue;
        }

        // Go through IO lines
        std::optional<unsigned int> desiredLineNumber;
        const size_t chipLineNumbers = gpiod_chip_info_get_num_lines(pChipInfo);

        gpiod_chip_info_free(pChipInfo);

        for(size_t i = 0; i < chipLineNumbers; i++)
        {
            auto pLineInfo = gpiod_chip_get_line_info(pChip_, i);
            if(!pLineInfo)
                continue;

            std::string lineName(gpiod_line_info_get_name(pLineInfo));
            gpiod_line_info_free(pLineInfo);

            if(lineName == std::string(pName))
            {
                desiredLineNumber = i;
                break;
            }
        }

        if(!desiredLineNumber)
            continue;

        // Configure pin and get request line
        auto pRequestConfig = gpiod_request_config_new();
        gpiod_request_config_set_consumer(pRequestConfig, "TIGERs");
        gpiod_request_config_set_event_buffer_size(pRequestConfig, 0);

        auto pLineSetting = gpiod_line_settings_new();
        if(dir == OUTPUT)
        {
            gpiod_line_settings_set_direction(pLineSetting, GPIOD_LINE_DIRECTION_OUTPUT);
            gpiod_line_settings_set_drive(pLineSetting, GPIOD_LINE_DRIVE_PUSH_PULL);
            gpiod_line_settings_set_output_value(pLineSetting, GPIOD_LINE_VALUE_INACTIVE);
        }
        else
        {
            gpiod_line_settings_set_direction(pLineSetting, GPIOD_LINE_DIRECTION_INPUT);

            switch(pull)
            {
                case PULL_DOWN:
                    gpiod_line_settings_set_bias(pLineSetting, GPIOD_LINE_BIAS_PULL_DOWN);
                    break;
                case PULL_UP:
                    gpiod_line_settings_set_bias(pLineSetting, GPIOD_LINE_BIAS_PULL_UP);
                    break;
                default:
                    gpiod_line_settings_set_bias(pLineSetting, GPIOD_LINE_BIAS_DISABLED);
                    break;
            }

            gpiod_line_settings_set_active_low(pLineSetting, isActiveLow);
            gpiod_line_settings_set_edge_detection(pLineSetting, GPIOD_LINE_EDGE_BOTH);
            gpiod_line_settings_set_debounce_period_us(pLineSetting, 100);

            pEventBuffer_ = gpiod_edge_event_buffer_new(0);
        }

        auto pLineConfig = gpiod_line_config_new();
        result = gpiod_line_config_add_line_settings(pLineConfig, &desiredLineNumber.value(), 1, pLineSetting);
        if(result)
        {
            LOG(ERROR) << "Failed to add line settings for pin: " << pName;
            continue;
        }

        pLine_ = gpiod_chip_request_lines(pChip_, pRequestConfig, pLineConfig);
        if(!pLine_)
        {
            LOG(ERROR) << "Failed to request line: " << pName;
            continue;
        }

        gpiod_request_config_free(pRequestConfig);

        lineNumber_ = *desiredLineNumber;

        break;
    }

    if(!pLine_)
    {
        LOG(ERROR) << "Failed to find GPIO line: " << pName << " on any GPIO chip";
        return;
    }

    initSuccess_ = true;
}

RPiGPIO::~RPiGPIO()
{
    if(pEventBuffer_)
        gpiod_edge_event_buffer_free(pEventBuffer_);

    if(pLine_)
        gpiod_line_request_release(pLine_);

    if(pChip_)
        gpiod_chip_close(pChip_);
}

void RPiGPIO::write(bool on)
{
    if(!pLine_)
        return;

    int result = gpiod_line_request_set_value(pLine_, lineNumber_, on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
    if(result)
    {
        LOG(ERROR) << "GPIO write failed.";
    }
}

bool RPiGPIO::read()
{
    if(!pLine_)
        return false;

    int result = gpiod_line_request_get_value(pLine_, lineNumber_);
    if(result < 0)
    {
        LOG(ERROR) << "GPIO read failed.";
        return false;
    }

    return result == 1;
}

bool RPiGPIO::getNextEvent(Event& event)
{
    if(!pEventBuffer_)
        return false;

    if(gpiod_line_request_wait_edge_events(pLine_, 0) == 1)
    {
        if(gpiod_line_request_read_edge_events(pLine_, pEventBuffer_, 1) > 0)
        {
            auto pEvent = gpiod_edge_event_buffer_get_event(pEventBuffer_, 0);
            if(gpiod_edge_event_get_event_type(pEvent) == GPIOD_EDGE_EVENT_RISING_EDGE)
                event = EVENT_RSING;
            else
                event = EVENT_FALLING;

            return true;
        }
    }

    return false;
}
