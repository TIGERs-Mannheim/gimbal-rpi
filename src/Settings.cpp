#include "Settings.hpp"
#include "easylogging++.h"

#include <fstream>

using json = nlohmann::json;

Settings::Settings(const std::string& filename)
:filename_(filename)
{
    load();
}

Settings::~Settings()
{
    save();
}

void Settings::load()
{
    namespace fs = std::filesystem;

    try
    {
        // Load file content
        std::ifstream in(filename_);

        if(!in.good())
            return;

        json jFile;

        in >> jFile;

        in.close();

        serial_ = jFile.value("serial", Serial());
        network_ = jFile.value("network", Network());
        cameraPose_ = jFile.value("cameraPose", CameraPose());
        limits_ = jFile.value("limits", Limits());
    }
    catch(json::exception& ex)
    {
        LOG(ERROR) << "Failed to load: " << filename_ << ", error: " << ex.what();
    }
}

void Settings::save()
{
    std::ofstream out(filename_);

    json jFile;

    jFile["serial"] = serial_;
    jFile["network"] = network_;
    jFile["cameraPose"] = cameraPose_;
    jFile["limits"] = limits_;

    out << std::setw(4) << jFile << std::endl;
}
