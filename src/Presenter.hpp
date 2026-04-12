#pragma once

#include "State.hpp"
#include "Settings.hpp"
#include "util/RPiGPIO.hpp"
#include <lvgl.h>

class Presenter
{
public:
    Presenter(State& state, Settings& settings);

    void spinOnce();

private:
    static void lvIndevReadWrapper(lv_indev_t* pIndev, lv_indev_data_t* pData);

    State& state_;
    Settings& settings_;

    struct MappedKey
    {
        std::unique_ptr<RPiGPIO> pGpio;
        lv_key_t key;
    };

    std::vector<MappedKey> keys_;

    lv_style_t style_ {};
    lv_obj_t* lblBallPos_;
    lv_obj_t* lblTarget_;
};
