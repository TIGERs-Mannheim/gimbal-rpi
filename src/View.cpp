#include "View.hpp"
#include "easylogging++.h"

View::View(event_callback_t eventCallback)
: eventCallback_(eventCallback)
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
    auto pInDev = lv_indev_create();
    lv_indev_set_user_data(pInDev, this);
    lv_indev_set_type(pInDev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(pInDev, lvIndevReadWrapper);

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

    auto pStatusScreen = createStatusScreen();
    lv_screen_load(pStatusScreen);
    lv_indev_set_group(pInDev, static_cast<lv_group_t*>(lv_obj_get_user_data(pStatusScreen)));
}

lv_obj_t* View::createStatusScreen()
{
    auto pScreen = lv_obj_create(nullptr);

    // Header section
    static int32_t headerColumnWidths[] = { 30, LV_GRID_FR(1), 30, LV_GRID_TEMPLATE_LAST };
    static int32_t headerRowHeights[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pHeaderGrid = lv_obj_create(pScreen);
    lv_obj_add_style(pHeaderGrid, &style_, 0);
    lv_obj_set_size(pHeaderGrid, 240, 40);
    lv_obj_set_align(pHeaderGrid, LV_ALIGN_TOP_MID);
    lv_obj_set_grid_dsc_array(pHeaderGrid, headerColumnWidths, headerRowHeights);

    auto pBtnPrev = lv_button_create(pHeaderGrid);
    lv_obj_set_grid_cell(pBtnPrev, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_t* pLabel = lv_label_create(pBtnPrev);
    lv_label_set_text(pLabel, LV_SYMBOL_LEFT);
    lv_obj_center(pLabel);

    pLabel = lv_label_create(pHeaderGrid);
    lv_obj_set_grid_cell(pLabel, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(pLabel, "Status");

    auto pBtnNext = lv_button_create(pHeaderGrid);
    lv_obj_set_grid_cell(pBtnNext, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    pLabel = lv_label_create(pBtnNext);
    lv_label_set_text(pLabel, LV_SYMBOL_RIGHT);
    lv_obj_center(pLabel);

    // Footer section
    static int32_t footerColumnWidths[] = { 60, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t footerRowHeights[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pFooterGrid = lv_obj_create(pScreen);
    lv_obj_add_style(pFooterGrid, &style_, 0);
    lv_obj_add_style(pFooterGrid, &styleSmall_, 0);
    lv_obj_set_size(pFooterGrid, 240, 30);
    lv_obj_set_align(pFooterGrid, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_grid_dsc_array(pFooterGrid, footerColumnWidths, footerRowHeights);

    pLblMode_ = lv_label_create(pFooterGrid);
    lv_obj_set_grid_cell(pLblMode_, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(pLblMode_, "Mode: Off");

    pLblPan_ = lv_label_create(pFooterGrid);
    lv_obj_set_grid_cell(pLblPan_, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(pLblPan_, "Pan: 42.0°");

    pLblTilt_ = lv_label_create(pFooterGrid);
    lv_obj_set_grid_cell(pLblTilt_, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_label_set_text(pLblTilt_, "Tilt: -42.0°");

    // Content Section
    static int32_t contentColumnWidths[] = { 70, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t contentRowHeights[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pContentGrid = lv_obj_create(pScreen);
    lv_obj_add_style(pContentGrid, &style_, 0);
    lv_obj_add_style(pContentGrid, &styleSmall_, 0);
    lv_obj_set_size(pContentGrid, LV_PCT(100), 100);
    lv_obj_set_scrollbar_mode(pContentGrid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align_to(pContentGrid, pHeaderGrid, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
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

    pLblHostname_ = addRow("Name:", "tigerspizero2w", 0);
    pLblOwnIp_ = addRow("IP:", "-", 1);
    pLblTracker_ = addRow("Tracker:", "None", 2);
    pLblTrackerIp_ = addRow("", "-", 3);
    pLblBallPos_ = addRow("Ball:", "-", 4);

    // Button Section
    static int32_t buttonColumnWidths[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t buttonRowHeights[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    auto pButtonGrid = lv_obj_create(pScreen);
    lv_obj_add_style(pButtonGrid, &style_, 0);
    lv_obj_set_size(pButtonGrid, LV_PCT(100), 70);
    lv_obj_align_to(pButtonGrid, pContentGrid, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_grid_dsc_array(pButtonGrid, buttonColumnWidths, buttonRowHeights);

    auto addBtn = [&](const char* pText, int32_t row, int32_t col)
    {
        auto pBtn = lv_button_create(pButtonGrid);
        lv_obj_add_style(pBtn, &styleFocused_, LV_STATE_FOCUS_KEY);
        lv_obj_set_grid_cell(pBtn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        pLabel = lv_label_create(pBtn);
        lv_label_set_text(pLabel, pText);
        lv_obj_center(pLabel);

        return pBtn;
    };

    pBtnHome_ = addBtn(LV_SYMBOL_HOME, 0, 0);
    pBtnOff_ = addBtn("Off", 1, 0);
    pBtnManual_ = addBtn("Manual", 0, 1);
    pBtnLive_ = addBtn("Live", 1, 1);

    lv_obj_add_event_cb(pBtnHome_, &View::lvEventCallback, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(pBtnOff_, &View::lvEventCallback, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(pBtnManual_, &View::lvEventCallback, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(pBtnLive_, &View::lvEventCallback, LV_EVENT_PRESSED, this);

    // Create group and add items which should be navigateble
    auto pInputGroup = lv_group_create();
    // lv_group_add_obj(pInputGroup, pBtnPrev); // Disable navigation until this feature is used
    // lv_group_add_obj(pInputGroup, pBtnNext);
    lv_group_add_obj(pInputGroup, pBtnHome_);
    lv_group_add_obj(pInputGroup, pBtnOff_);
    lv_group_add_obj(pInputGroup, pBtnManual_);
    lv_group_add_obj(pInputGroup, pBtnLive_);

    lv_obj_add_state(pBtnOff_, LV_STATE_FOCUS_KEY);
    lv_group_focus_obj(pBtnOff_);

    lv_obj_set_user_data(pScreen, pInputGroup);

    return pScreen;
}

void View::spinOnce()
{
    lv_timer_handler();
}

void View::setState(const State& state)
{
    lv_label_set_text(pLblHostname_, state.hostname.c_str());
    lv_label_set_text(pLblOwnIp_, state.ip.c_str());
    lv_label_set_text(pLblTracker_, state.tracker.c_str());
    lv_label_set_text(pLblTrackerIp_, state.trackerIp.c_str());
    lv_label_set_text_fmt(pLblBallPos_, "%6.2f %6.2f %6.2f", state.ballPos_m[0], state.ballPos_m[1], state.ballPos_m[2]);
    lv_label_set_text(pLblMode_, state.mode.c_str());
    lv_label_set_text_fmt(pLblPan_, "Pan: % .1f°", state.pan_deg);
    lv_label_set_text_fmt(pLblTilt_, "Tilt: % .1f°", state.tilt_deg);
}

void View::lvIndevReadWrapper(lv_indev_t* pIndev, lv_indev_data_t* pData)
{
    auto pPresenter = static_cast<View*>(lv_indev_get_user_data(pIndev));

    RPiGPIO::Event event;

    for(auto& key : pPresenter->keys_)
    {
        if(key.pGpio->getNextEvent(event))
        {
            pData->key = key.key;

            LOG(INFO) << "Key " << key.key << " event " << event;

            if(event == RPiGPIO::EVENT_RSING)
                pData->state = LV_INDEV_STATE_PRESSED;
            else
                pData->state = LV_INDEV_STATE_RELEASED;

            pData->continue_reading = true;
        }
    }
}

void View::lvEventCallback(lv_event_t* pEvent)
{
    auto pPresenter = static_cast<View*>(lv_event_get_user_data(pEvent));
    auto pSrc = lv_event_get_target_obj(pEvent);

    if(pSrc == pPresenter->pBtnHome_)
    {
        LOG(INFO) << "Home pressed";
        pPresenter->eventCallback_(EVENT_HOME_CLICKED);
    }
    else if(pSrc == pPresenter->pBtnOff_)
    {
        LOG(INFO) << "Off pressed";
        pPresenter->eventCallback_(EVENT_OFF_CLICKED);
    }
    else if(pSrc == pPresenter->pBtnManual_)
    {
        LOG(INFO) << "Manual pressed";
        pPresenter->eventCallback_(EVENT_MANUAL_CLICKED);
    }
    else if(pSrc == pPresenter->pBtnLive_)
    {
        LOG(INFO) << "Live pressed";
        pPresenter->eventCallback_(EVENT_LIVE_CLICKED);
    }
}
