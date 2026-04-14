#include "SysFsEntry.hpp"
#include <fstream>
#include <filesystem>

bool SysFsEntry::isExisting() const
{
    return std::filesystem::exists(path_);
}

std::string SysFsEntry::readString() const
{
    std::fstream sys;
    sys.rdbuf()->pubsetbuf(nullptr, 0);
    sys.open(path_, std::ios::in);
    if(!sys)
        return "";

    std::string value;
    sys >> value;

    return value;
}

int32_t SysFsEntry::readInt32() const
{
    auto value = readString();

    try
    {
        return std::stoi(value);
    }
    catch(const std::logic_error&)
    {
        return 0;
    }
}

float SysFsEntry::readFloat() const
{
    auto value = readString();

    try
    {
        return std::stof(value);
    }
    catch(const std::logic_error&)
    {
        return 0;
    }
}

void SysFsEntry::writeString(const std::string& str)
{
    std::ofstream sys;
    sys.rdbuf()->pubsetbuf(nullptr, 0);
    sys.open(path_, std::ios::out);
    if(!sys)
        return;

    sys << str;
}

void SysFsEntry::writeInt32(int32_t val)
{
    writeString(std::to_string(val));
}

void SysFsEntry::writeFloat(float val)
{
    writeString(std::to_string(val));
}