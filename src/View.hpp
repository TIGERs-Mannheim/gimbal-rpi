#pragma once

#include "util/RPiGPIO.hpp"
#include "Settings.hpp"
#include <lvgl.h>
#include <string>
#include <memory>
#include <queue>
#include <thread>

class View
{
public:
    enum Event
    {
        EVENT_OFF_CLICKED,
        EVENT_MANUAL_CLICKED,
        EVENT_LIVE_CLICKED,
        EVENT_QUIT_CLICKED,
        EVENT_CALIB_CLICKED,
        EVENT_LIMIT_PAN_CLICKED,
        EVENT_LIMIT_TILT_CLICKED,
        EVENT_ZERO_CLICKED,
        EVENT_POSE_X_CHANGED,
        EVENT_POSE_Y_CHANGED,
        EVENT_POSE_Z_CHANGED,
        EVENT_POSE_ORIENTATION_CHANGED,
    };

    struct EventData
    {
        Event event;
        int intParam = 0;
        float floatParam = 0;
        float floatParam2 = 0;
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
        std::array<float, 2> limitPan_deg = { -90.0f, 90.0f };
        std::array<float, 2> limitTilt_deg = { -40.0f, 40.0f };

        GimbalMsgState gimbalState;
    };

    View();

    void setState(const State& state);
    void setPose(const Settings::CameraPose& pose);

    bool getNextEvent(EventData& event);

private:
    static void lvIndevReadWrapper(lv_indev_t* pIndev, lv_indev_data_t* pData);
    static void lvEventCallback(lv_event_t* pEvent);

    void lvglThread(std::stop_token st);

    lv_obj_t* createHeader(lv_obj_t* pParent);
    lv_obj_t* createFooter(lv_obj_t* pParent);
    void setupScreen();

    lv_obj_t* createStatusTile();
    lv_obj_t* createSetupTile();
    lv_obj_t* createPoseTile();
    lv_obj_t* createDebugTile();

    void loadTile(uint32_t tileIndex);

    std::vector<std::unique_ptr<EventData>> eventData_;
    std::queue<EventData> eventQueue_;
    std::mutex eventQueueMutex_;

    std::jthread lvThread_;

    State state_;
    std::mutex stateMutex_;

    std::optional<Settings::CameraPose> newCameraPose_;
    std::mutex newCameraPoseMutex_;

    struct MappedKey
    {
        std::unique_ptr<RPiGPIO> pGpio;
        lv_key_t key;
    };

    std::vector<MappedKey> keys_;

    lv_indev_t* pInputDevice_;
    bool isEnterPressed_ = false;

    lv_style_t style_ {};
    lv_style_t styleSmall_ {};
    lv_style_t styleFocused_ {};
    lv_style_t styleSpinboxCursor_ {};
    lv_style_t styleSpinboxMain_ {};

    lv_obj_t* pBtnNavLeft_;
    lv_obj_t* pLblTileName_;
    lv_obj_t* pBtnNavRight_;

    lv_obj_t* pTileView_;
    uint32_t tileIndex_ = 0;

    lv_obj_t* pLblMode_;
    lv_obj_t* pLblPan_;
    lv_obj_t* pLblTilt_;

    struct TileData
    {
        const char* pName = nullptr;
        lv_group_t* pInputGroup = nullptr;
    };

    struct
    {
        TileData data;

        lv_obj_t* pLblHostname;
        lv_obj_t* pLblOwnIp;
        lv_obj_t* pLblTracker;
        lv_obj_t* pLblTrackerIp;
        lv_obj_t* pLblBallPos;
    } status_;

    struct
    {
        TileData data;

        lv_obj_t* pLblPanRange;
        lv_obj_t* pLblTiltRange;
    } setup_;

    struct
    {
        TileData data;

        lv_obj_t* pBoxX;
        lv_obj_t* pBoxY;
        lv_obj_t* pBoxZ;
        lv_obj_t* pBoxYaw;
    } pose_;

    struct
    {
        TileData data;

        lv_obj_t* pLblSupplyAndCpu;
        lv_obj_t* pLblVelocity;
        lv_obj_t* pLblCurrent;
        lv_obj_t* pLblEncoderErrors;
        lv_obj_t* pLblMotorErrors;
        lv_obj_t* pLblMcuVersion;
    } debug_;
};
