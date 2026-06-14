#include "View.hpp"
#include "easylogging++.h"
#include "src/misc/lv_event_private.h"

using namespace std::chrono_literals;

View::View()
{
    // Create display device
    auto pDisp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(pDisp, "/dev/fb1");
    lv_display_set_rotation(pDisp, LV_DISPLAY_ROTATION_90);

    // Create input keys used by LVGL as rotary encoder replacement
    auto makeKey = [](const char* pName, lv_key_t key)
    {
        MappedKey k;
        k.key = key;
        k.pGpio = std::make_unique<RPiGPIO>(pName, RPiGPIO::INPUT, RPiGPIO::PULL_UP, true);

        return k;
    };

    keys_.emplace_back(makeKey("GPIO21", LV_KEY_RIGHT)); // KEY1
    keys_.emplace_back(makeKey("GPIO20", LV_KEY_ENTER)); // KEY2
    keys_.emplace_back(makeKey("GPIO16", LV_KEY_LEFT)); // KEY3

    // Configure input device
    pInputDevice_ = lv_indev_create();
    lv_indev_set_user_data(pInputDevice_, this);
    lv_indev_set_type(pInputDevice_, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(pInputDevice_, lvIndevReadWrapper);
    lv_indev_set_long_press_time(pInputDevice_, 500);
    lv_indev_set_long_press_repeat_time(pInputDevice_, 200);

    // Adjust style a bit
    lv_style_init(&style_);
    lv_style_set_pad_all(&style_, 5);
    lv_style_set_border_width(&style_, 1);
    lv_style_set_radius(&style_, 0);

    lv_style_init(&styleFocused_);
    lv_style_set_outline_color(&styleFocused_, lv_color_hex(0x222222));

    lv_style_init(&styleSmall_);
    lv_style_set_text_font(&styleSmall_, &lv_font_montserrat_14);
    lv_style_set_pad_all(&styleSmall_, 2);

    lv_style_init(&styleSpinboxCursor_);
    lv_style_set_bg_opa(&styleSpinboxCursor_, 0);
    lv_style_set_text_opa(&styleSpinboxCursor_, 0);
    lv_style_set_border_color(&styleSpinboxCursor_, lv_color_hex(0x6366f1));
    lv_style_set_border_width(&styleSpinboxCursor_, 2);
    lv_style_set_radius(&styleSpinboxCursor_, 2);
    lv_style_set_pad_ver(&styleSpinboxCursor_, 1);

    lv_style_init(&styleSpinboxMain_);
    lv_style_set_pad_all(&styleSpinboxMain_, 2);
    lv_style_set_text_letter_space(&styleSpinboxMain_, 3);
    lv_style_set_text_font(&styleSpinboxMain_, &lv_font_montserrat_18);

    setupScreen();
    createStatusTile();
    createSetupTile();
    createPoseTile();
    loadTile(0);

    lvThread_ = std::jthread(std::bind_front(&View::lvglThread, this));
}

void View::lvglThread(std::stop_token st)
{
    using clock_t = std::chrono::steady_clock;

    auto tLastCall = clock_t::now();

    while(!st.stop_requested())
    {
        State state;
        std::optional<Settings::CameraPose> newCameraPose;

        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            state = state_;
        }

        {
            std::lock_guard<std::mutex> lock(newCameraPoseMutex_);
            if(newCameraPose_.has_value())
            {
                newCameraPose = newCameraPose_;
                newCameraPose_.reset();
            }
        }

        lv_label_set_text(status_.pLblHostname, state.hostname.c_str());
        lv_label_set_text(status_.pLblOwnIp, state.ip.c_str());
        lv_label_set_text(status_.pLblTracker, state.tracker.c_str());
        lv_label_set_text(status_.pLblTrackerIp, state.trackerIp.c_str());
        lv_label_set_text_fmt(status_.pLblBallPos, "%6.2f %6.2f %6.2f", state.ballPos_m[0], state.ballPos_m[1], state.ballPos_m[2]);
        lv_label_set_text_fmt(status_.pLblGimbal, "%.2fV, %.0f%% CPU", state.gimbalSupply_V, state.gimbalCpuLoad*100.0f);
        lv_label_set_text(pLblMode_, state.mode.c_str());
        lv_label_set_text_fmt(pLblPan_, "Pan: % .1f°", state.pan_deg);
        lv_label_set_text_fmt(pLblTilt_, "Tilt: % .1f°", state.tilt_deg);
        lv_label_set_text_fmt(setup_.pLblPanRange, "% .1f .. % .1f", state.limitPan_deg[0], state.limitPan_deg[1]);
        lv_label_set_text_fmt(setup_.pLblTiltRange, "% .1f .. % .1f", state.limitTilt_deg[0], state.limitTilt_deg[1]);

        if(newCameraPose.has_value())
        {
            lv_spinbox_set_value(pose_.pBoxX, newCameraPose->positionInFieldFrame_m[0] * 1000.0f);
            lv_spinbox_set_value(pose_.pBoxY, newCameraPose->positionInFieldFrame_m[1] * 1000.0f);
            lv_spinbox_set_value(pose_.pBoxZ, newCameraPose->positionInFieldFrame_m[2] * 1000.0f);
            lv_spinbox_set_value(pose_.pBoxYaw, newCameraPose->yawInFieldFrame_deg);
        }

        lv_timer_handler();

        std::this_thread::sleep_until(tLastCall+30ms);
        tLastCall = clock_t::now();
    }
}

lv_obj_t* View::createHeader(lv_obj_t* pParent)
{
    static int32_t headerColumnWidths[] = { 30, LV_GRID_FR(1), 30, LV_GRID_TEMPLATE_LAST };
    static int32_t headerRowHeights[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pHeaderGrid = lv_obj_create(pParent);
    lv_obj_add_style(pHeaderGrid, &style_, 0);
    lv_obj_set_grid_dsc_array(pHeaderGrid, headerColumnWidths, headerRowHeights);

    pBtnNavLeft_ = lv_button_create(pHeaderGrid);
    lv_obj_set_grid_cell(pBtnNavLeft_, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_t* pLabel = lv_label_create(pBtnNavLeft_);
    lv_label_set_text(pLabel, LV_SYMBOL_LEFT);
    lv_obj_center(pLabel);

    pLblTileName_ = lv_label_create(pHeaderGrid);
    lv_obj_set_grid_cell(pLblTileName_, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(pLblTileName_, "Title");
    lv_obj_set_style_text_font(pLblTileName_, &lv_font_montserrat_18, 0);

    pBtnNavRight_ = lv_button_create(pHeaderGrid);
    lv_obj_set_grid_cell(pBtnNavRight_, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    pLabel = lv_label_create(pBtnNavRight_);
    lv_label_set_text(pLabel, LV_SYMBOL_RIGHT);
    lv_obj_center(pLabel);

    lv_obj_add_event_cb(pBtnNavLeft_, &View::lvEventCallback, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(pBtnNavRight_, &View::lvEventCallback, LV_EVENT_PRESSED, this);

    return pHeaderGrid;
}

lv_obj_t* View::createFooter(lv_obj_t* pParent)
{
    static int32_t footerColumnWidths[] = { 60, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t footerRowHeights[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pFooterGrid = lv_obj_create(pParent);
    lv_obj_add_style(pFooterGrid, &style_, 0);
    lv_obj_add_style(pFooterGrid, &styleSmall_, 0);
    lv_obj_set_grid_dsc_array(pFooterGrid, footerColumnWidths, footerRowHeights);

    pLblMode_ = lv_label_create(pFooterGrid);
    lv_obj_set_grid_cell(pLblMode_, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(pLblMode_, "INIT");

    pLblPan_ = lv_label_create(pFooterGrid);
    lv_obj_set_grid_cell(pLblPan_, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(pLblPan_, "Pan: 42.0°");

    pLblTilt_ = lv_label_create(pFooterGrid);
    lv_obj_set_grid_cell(pLblTilt_, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(pLblTilt_, "Tilt: -42.0°");

    return pFooterGrid;
}

void View::setupScreen()
{
    static int32_t columnWidths[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t rowHeights[] = { 30, LV_GRID_FR(1), 20, LV_GRID_TEMPLATE_LAST };

    auto pScreen = lv_screen_active();

    lv_obj_add_style(pScreen, &style_, 0);
    lv_obj_add_style(pScreen, &styleSmall_, 0);
    lv_obj_set_style_pad_all(pScreen, 0, 0);
    lv_obj_set_style_pad_gap(pScreen, 0, 0);
    lv_obj_set_size(pScreen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_align(pScreen, LV_ALIGN_TOP_MID);
    lv_obj_set_grid_dsc_array(pScreen, columnWidths, rowHeights);

    // Header section
    auto pHeader = createHeader(pScreen);
    lv_obj_set_grid_cell(pHeader, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    // Content section
    pTileView_ = lv_tileview_create(pScreen);
    lv_obj_set_grid_cell(pTileView_, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    // Footer section
    auto pFooter = createFooter(pScreen);
    lv_obj_set_grid_cell(pFooter, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
}

lv_obj_t* View::createStatusTile()
{
    auto pTile = lv_tileview_add_tile(pTileView_, 0, 0, LV_DIR_HOR);

    // Content Section
    static int32_t contentColumnWidths[] = { 70, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t contentRowHeights[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pContentGrid = lv_obj_create(pTile);
    lv_obj_add_style(pContentGrid, &style_, 0);
    lv_obj_add_style(pContentGrid, &styleSmall_, 0);
    lv_obj_set_size(pContentGrid, LV_PCT(100), 130);
    lv_obj_set_scrollbar_mode(pContentGrid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_grid_dsc_array(pContentGrid, contentColumnWidths, contentRowHeights);

    auto addRow = [&](const char* pName, const char* pDefaultText, int32_t row)
    {
        auto pNameLabel = lv_label_create(pContentGrid);
        lv_obj_set_grid_cell(pNameLabel, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_label_set_text(pNameLabel, pName);

        auto pValueLabel = lv_label_create(pContentGrid);
        lv_obj_set_grid_cell(pValueLabel, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_label_set_text(pValueLabel, pDefaultText);

        return pValueLabel;
    };

    status_.pLblHostname = addRow("Name:", "tigerspizero2w", 0);
    status_.pLblOwnIp = addRow("IP:", "-", 1);
    status_.pLblTracker = addRow("Tracker:", "None", 2);
    status_.pLblTrackerIp = addRow("", "-", 3);
    status_.pLblBallPos = addRow("Ball:", "-", 4);
    status_.pLblGimbal = addRow("Gimbal:", "-", 5);

    // Button Section
    static int32_t buttonColumnWidths[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t buttonRowHeights[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pButtonGrid = lv_obj_create(pTile);
    lv_obj_add_style(pButtonGrid, &style_, 0);
    lv_obj_set_size(pButtonGrid, LV_PCT(100), 40);
    lv_obj_align_to(pButtonGrid, pContentGrid, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_grid_dsc_array(pButtonGrid, buttonColumnWidths, buttonRowHeights);

    auto pInputGroup = lv_group_create();

    auto addBtn = [&](const char* pText, int32_t row, int32_t col, Event event)
    {
        auto pBtn = lv_button_create(pButtonGrid);
        lv_obj_add_style(pBtn, &styleFocused_, LV_STATE_FOCUS_KEY);
        lv_obj_set_grid_cell(pBtn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_t* pLabel = lv_label_create(pBtn);
        lv_label_set_text(pLabel, pText);
        lv_obj_center(pLabel);

        lv_obj_add_event_cb(pBtn, &View::lvEventCallback, LV_EVENT_PRESSED, this);
        lv_group_add_obj(pInputGroup, pBtn);

        auto pEventData = std::make_unique<EventData>();
        pEventData->event = event;

        lv_obj_set_user_data(pBtn, pEventData.get());

        eventData_.emplace_back(std::move(pEventData));

        return pBtn;
    };

    auto pBtnOff = addBtn("Off", 0, 0, EVENT_OFF_CLICKED);
    addBtn("Manual", 0, 1, EVENT_MANUAL_CLICKED);
    addBtn("Live", 0, 2, EVENT_LIVE_CLICKED);

    lv_obj_add_state(pBtnOff, LV_STATE_FOCUS_KEY);
    lv_group_focus_obj(pBtnOff);

    status_.data.pName = "Status";
    status_.data.pInputGroup = pInputGroup;
    lv_obj_set_user_data(pTile, &status_.data);

    return pTile;
}

lv_obj_t* View::createSetupTile()
{
    auto pTile = lv_tileview_add_tile(pTileView_, 1, 0, LV_DIR_HOR);

    // Content Section
    static int32_t contentColumnWidths[] = { 70, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t contentRowHeights[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pContentGrid = lv_obj_create(pTile);
    lv_obj_add_style(pContentGrid, &style_, 0);
    lv_obj_add_style(pContentGrid, &styleSmall_, 0);
    lv_obj_set_size(pContentGrid, LV_PCT(100), 50);
    lv_obj_set_scrollbar_mode(pContentGrid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_grid_dsc_array(pContentGrid, contentColumnWidths, contentRowHeights);

    auto addRow = [&](const char* pName, const char* pDefaultText, int32_t row)
    {
        auto pNameLabel = lv_label_create(pContentGrid);
        lv_obj_set_grid_cell(pNameLabel, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_label_set_text(pNameLabel, pName);

        auto pValueLabel = lv_label_create(pContentGrid);
        lv_obj_set_grid_cell(pValueLabel, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, row, 1);
        lv_label_set_text(pValueLabel, pDefaultText);

        return pValueLabel;
    };

    setup_.pLblPanRange = addRow("Pan:", "-42 .. 42", 0);
    setup_.pLblTiltRange = addRow("Tilt:", "-10 .. 10", 1);

    // Button Section
    static int32_t buttonColumnWidths[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t buttonRowHeights[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pButtonGrid = lv_obj_create(pTile);
    lv_obj_add_style(pButtonGrid, &style_, 0);
    lv_obj_set_size(pButtonGrid, LV_PCT(100), 120);
    lv_obj_align_to(pButtonGrid, pContentGrid, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_grid_dsc_array(pButtonGrid, buttonColumnWidths, buttonRowHeights);

    auto pInputGroup = lv_group_create();

    auto addBtn = [&](const char* pText, int32_t row, int32_t col, Event event, int param)
    {
        auto pBtn = lv_button_create(pButtonGrid);
        lv_obj_add_style(pBtn, &styleFocused_, LV_STATE_FOCUS_KEY);
        lv_obj_set_grid_cell(pBtn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_t* pLabel = lv_label_create(pBtn);
        lv_label_set_text(pLabel, pText);
        lv_obj_center(pLabel);

        lv_obj_add_event_cb(pBtn, &View::lvEventCallback, LV_EVENT_PRESSED, this);
        lv_group_add_obj(pInputGroup, pBtn);

        auto pEventData = std::make_unique<EventData>();
        pEventData->event = event;
        pEventData->intParam = param;

        lv_obj_set_user_data(pBtn, pEventData.get());

        eventData_.emplace_back(std::move(pEventData));

        return pBtn;
    };

    addBtn("- Pan", 0, 0, EVENT_LIMIT_PAN_CLICKED, 0);
    addBtn("+ Pan", 1, 0, EVENT_LIMIT_PAN_CLICKED, 1);
    addBtn("Cal Pan", 2, 0, EVENT_CALIB_CLICKED, 0);
    addBtn("0 Pan", 3, 0, EVENT_ZERO_CLICKED, 0);
    addBtn("- Tilt", 0, 1, EVENT_LIMIT_TILT_CLICKED, 0);
    addBtn("+ Tilt", 1, 1, EVENT_LIMIT_TILT_CLICKED, 1);
    addBtn("Cal Tilt", 2, 1, EVENT_CALIB_CLICKED, 1);
    addBtn("0 Tilt", 3, 1, EVENT_ZERO_CLICKED, 1);

    setup_.data.pName = "Setup";
    setup_.data.pInputGroup = pInputGroup;
    lv_obj_set_user_data(pTile, &setup_.data);

    return pTile;
}

lv_obj_t* View::createPoseTile()
{
    static int32_t contentColumnWidths[] = { 50, LV_GRID_FR(1), 50, LV_GRID_TEMPLATE_LAST };
    static int32_t contentRowHeights[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pTile = lv_tileview_add_tile(pTileView_, 2, 0, LV_DIR_HOR);
    auto pInputGroup = lv_group_create();

    auto pContentGrid = lv_obj_create(pTile);
    lv_obj_add_style(pContentGrid, &style_, 0);
    lv_obj_set_size(pContentGrid, LV_PCT(100), 160);
    lv_obj_set_scrollbar_mode(pContentGrid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_grid_dsc_array(pContentGrid, contentColumnWidths, contentRowHeights);

    auto addSpinbox = [&](const char* pText, const char* pUnitText, uint32_t digits, int32_t row, Event event)
    {
        auto pLabel = lv_label_create(pContentGrid);
        lv_label_set_text(pLabel, pText);
        lv_obj_set_grid_cell(pLabel, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, row, 1);

        auto pUnit = lv_label_create(pContentGrid);
        lv_label_set_text(pUnit, pUnitText);
        lv_obj_set_grid_cell(pUnit, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, row, 1);

        auto pBox = lv_spinbox_create(pContentGrid);
        lv_spinbox_set_digit_count(pBox, digits);
        lv_spinbox_set_value(pBox, 42000);
        lv_spinbox_set_dec_point_pos(pBox, 0);
        lv_obj_set_grid_cell(pBox, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, row, 1);

        lv_obj_add_style(pBox, &styleSpinboxMain_, LV_PART_MAIN);

        lv_obj_remove_style(pBox, nullptr, LV_PART_CURSOR);
        lv_obj_add_style(pBox, &styleSpinboxCursor_, LV_PART_CURSOR | LV_STATE_EDITED);

        lv_obj_add_event_cb(pBox, &View::lvEventCallback, LV_EVENT_VALUE_CHANGED, this);
        lv_group_add_obj(pInputGroup, pBox);

        auto pEventData = std::make_unique<EventData>();
        pEventData->event = event;
        lv_obj_set_user_data(pBox, pEventData.get());
        eventData_.emplace_back(std::move(pEventData));

        return pBox;
    };

    pose_.pBoxX = addSpinbox("X:", "mm", 5, 0, EVENT_POSE_X_CHANGED);
    pose_.pBoxY = addSpinbox("Y:", "mm", 5, 1, EVENT_POSE_Y_CHANGED);
    pose_.pBoxZ = addSpinbox("Z:", "mm", 5, 2, EVENT_POSE_Z_CHANGED);
    pose_.pBoxYaw = addSpinbox("Yaw:", "deg", 3, 3, EVENT_POSE_ORIENTATION_CHANGED);

    pose_.data.pName = "Cam Pose";
    pose_.data.pInputGroup = pInputGroup;
    lv_obj_set_user_data(pTile, &pose_.data);

    return pTile;
}

void View::loadTile(uint32_t tileIndex)
{
    lv_group_remove_obj(pBtnNavLeft_);
    lv_group_remove_obj(pBtnNavRight_);

    lv_tileview_set_tile_by_index(pTileView_, tileIndex, 0, LV_ANIM_ON);
    auto pTile = lv_tileview_get_tile_active(pTileView_);

    auto pTileData = static_cast<TileData*>(lv_obj_get_user_data(pTile));
    lv_group_add_obj(pTileData->pInputGroup, pBtnNavLeft_);
    lv_group_add_obj(pTileData->pInputGroup, pBtnNavRight_);
    lv_indev_set_group(pInputDevice_, pTileData->pInputGroup);

    lv_label_set_text(pLblTileName_, pTileData->pName);

    tileIndex_ = tileIndex;
}

void View::setState(const State& state)
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    state_ = state;
}

void View::setPose(const Settings::CameraPose& pose)
{
    std::lock_guard<std::mutex> lock(newCameraPoseMutex_);
    newCameraPose_ = pose;
}

bool View::getNextEvent(EventData& event)
{
    std::lock_guard<std::mutex> lock(eventQueueMutex_);

    if(eventQueue_.empty())
        return false;

    event = eventQueue_.front();
    eventQueue_.pop();

    return true;
}

void View::lvIndevReadWrapper(lv_indev_t* pIndev, lv_indev_data_t* pData)
{
    auto pPresenter = static_cast<View*>(lv_indev_get_user_data(pIndev));

    RPiGPIO::Event event;

    for(auto& key : pPresenter->keys_)
    {
        if(key.pGpio->getNextEvent(event))
        {
            if(key.key == LV_KEY_ENTER)
            {
                if(event == RPiGPIO::EVENT_RSING)
                    pPresenter->isEnterPressed_ = true;
                else
                    pPresenter->isEnterPressed_ = false;
            }
            else if(key.key == LV_KEY_LEFT && event == RPiGPIO::EVENT_RSING)
            {
                pData->enc_diff = -1;
            }
            else if(key.key == LV_KEY_RIGHT && event == RPiGPIO::EVENT_RSING)
            {
                pData->enc_diff = 1;
            }
            else
            {
                pData->enc_diff = 0;
            }

            pData->continue_reading = true;
        }
    }

    if(pPresenter->isEnterPressed_)
        pData->state = LV_INDEV_STATE_PRESSED;
    else
        pData->state = LV_INDEV_STATE_RELEASED;
}

void View::lvEventCallback(lv_event_t* pEvent)
{
    auto pPresenter = static_cast<View*>(lv_event_get_user_data(pEvent));
    auto pSrc = lv_event_get_target_obj(pEvent);

    if(pSrc == pPresenter->pBtnNavLeft_ && pPresenter->tileIndex_ > 0)
    {
        pPresenter->loadTile(pPresenter->tileIndex_ - 1);
        lv_obj_add_state(pPresenter->pBtnNavLeft_, LV_STATE_FOCUS_KEY);
        lv_group_focus_obj(pPresenter->pBtnNavLeft_);
    }
    else if(pSrc == pPresenter->pBtnNavRight_ && pPresenter->tileIndex_ < lv_obj_get_child_count(pPresenter->pTileView_) - 1)
    {
        pPresenter->loadTile(pPresenter->tileIndex_ + 1);
        lv_obj_add_state(pPresenter->pBtnNavRight_, LV_STATE_FOCUS_KEY);
        lv_group_focus_obj(pPresenter->pBtnNavRight_);
    }
    else if(lv_obj_get_user_data(pSrc))
    {
        auto pEventData = static_cast<EventData*>(lv_obj_get_user_data(pSrc));
        if(lv_event_get_code(pEvent) == LV_EVENT_VALUE_CHANGED)
        {
            pEventData->intParam = lv_spinbox_get_value(pSrc);
        }

        std::lock_guard<std::mutex> lock(pPresenter->eventQueueMutex_);
        pPresenter->eventQueue_.push(*pEventData);
    }
}
