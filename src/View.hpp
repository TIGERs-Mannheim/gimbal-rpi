#pragma once

#include "util/RPiGPIO.hpp"
#include <lvgl.h>
#include <string>
#include <functional>
#include <memory>

class View
{
public:
    enum Event
    {
        EVENT_HOME_CLICKED,
        EVENT_OFF_CLICKED,
        EVENT_MANUAL_CLICKED,
        EVENT_LIVE_CLICKED,
    };

    struct State
    {
        std::string hostname;
        std::string ip;
        std::string tracker;
        std::string trackerIp;
        float ballPos_m[3] {};
        std::string mode;
        float pan_deg = 0.0f;
        float tilt_deg = 0.0f;
    };

    using event_callback_t = std::function<void(Event)>;

    View(event_callback_t eventCallback);

    void spinOnce();
    void setState(const State& state);

private:
    static void lvIndevReadWrapper(lv_indev_t* pIndev, lv_indev_data_t* pData);
    static void lvEventCallback(lv_event_t* pEvent);

    lv_obj_t* createStatusScreen();

    event_callback_t eventCallback_;

    struct MappedKey
    {
        std::unique_ptr<RPiGPIO> pGpio;
        lv_key_t key;
    };

    std::vector<MappedKey> keys_;

    lv_style_t style_ {};
    lv_style_t styleSmall_ {};
    lv_style_t styleFocused_ {};

    lv_obj_t* pLblHostname_;
    lv_obj_t* pLblOwnIp_;
    lv_obj_t* pLblTracker_;
    lv_obj_t* pLblTrackerIp_;
    lv_obj_t* pLblBallPos_;
    lv_obj_t* pLblMode_;
    lv_obj_t* pLblPan_;
    lv_obj_t* pLblTilt_;

    lv_obj_t* pBtnHome_;
    lv_obj_t* pBtnOff_;
    lv_obj_t* pBtnManual_;
    lv_obj_t* pBtnLive_;
};
