#include "Presenter.hpp"
#include "easylogging++.h"

Presenter::Presenter(State& state, Settings& settings)
: state_(state),
  settings_(settings)
{
    auto makeKey = [](const char* pName, lv_key_t key)
    {
        MappedKey k;
        k.key = key;
        k.pGpio = std::make_unique<RPiGPIO>(pName, RPiGPIO::INPUT, RPiGPIO::PULL_UP, true);

        return k;
    };

    keys_.emplace_back(makeKey("GPIO13", LV_KEY_ENTER)); // Joystick PRESS
    keys_.emplace_back(makeKey("GPIO26", LV_KEY_LEFT)); // Joystick LEFT
    keys_.emplace_back(makeKey("GPIO5", LV_KEY_RIGHT)); // Joystick RIGHT
    keys_.emplace_back(makeKey("GPIO19", LV_KEY_UP)); // Joystick UP
    keys_.emplace_back(makeKey("GPIO6", LV_KEY_DOWN)); // Joystick DOWN
    keys_.emplace_back(makeKey("GPIO21", LV_KEY_NEXT)); // KEY1
    keys_.emplace_back(makeKey("GPIO16", LV_KEY_PREV)); // KEY3
    keys_.emplace_back(makeKey("GPIO20", LV_KEY_ESC)); // KEY2

    auto pDisp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(pDisp, "/dev/fb1");
    lv_display_set_rotation(pDisp, LV_DISPLAY_ROTATION_90);

    auto pGroup = lv_group_create();
    // lv_group_set_default(pGroup);

    auto pInDev = lv_indev_create();
    lv_indev_set_user_data(pInDev, this);
    lv_indev_set_type(pInDev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(pInDev, lvIndevReadWrapper);
    lv_indev_set_group(pInDev, pGroup);

    lv_style_init(&style_);
    lv_style_set_bg_color(&style_, lv_color_hex(0x000000));
    lv_style_set_text_color(&style_, lv_color_hex(0x00FF00));
    lv_style_set_border_color(&style_, lv_color_hex(0x00FF00));
    lv_style_set_text_font(&style_, &lv_font_montserrat_18);

    auto pScreen = lv_screen_active();

    lv_obj_add_style(pScreen, &style_, 0);

    lv_obj_t* pFlexbox = lv_obj_create(pScreen);
    lv_group_add_obj(pGroup, pFlexbox);
    lv_obj_set_size(pFlexbox, 240, 120);
    lv_obj_align(pFlexbox, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_flex_flow(pFlexbox, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_style(pFlexbox, &style_, 0);

    lblBallPos_ = lv_label_create(pFlexbox);
    // lv_obj_set_pos(lblBallPos_, 0, 0);

    lblTarget_ = lv_label_create(pFlexbox);
    // lv_obj_set_pos(lblTarget_, 0, 50);

    auto pBtn = lv_button_create(pFlexbox);
    lv_group_add_obj(pGroup, pBtn);
    lv_obj_set_size(pBtn, 80, 30);

    auto pBtn2 = lv_button_create(pFlexbox);
    lv_group_add_obj(pGroup, pBtn2);
    lv_obj_set_size(pBtn2, 80, 30);
    lv_obj_add_flag(pBtn2, LV_OBJ_FLAG_CHECKABLE);
}

void Presenter::spinOnce()
{
    lv_label_set_text_fmt(lblBallPos_, "%6.2f %6.2f %6.2f", state_.ball.pos_m[0], state_.ball.pos_m[1], state_.ball.pos_m[2]);
    lv_label_set_text_fmt(lblTarget_, "Pan: %6.1f, Tilt: %6.1f", state_.camera.pan_deg, state_.camera.tilt_deg);

    lv_timer_handler();
}

void Presenter::lvIndevReadWrapper(lv_indev_t* pIndev, lv_indev_data_t* pData)
{
    auto pPresenter = static_cast<Presenter*>(lv_indev_get_user_data(pIndev));

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
