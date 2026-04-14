#pragma once

#include <string>
#include <cstdint>

class SysFsEntry
{
public:
    SysFsEntry(const std::string& path):path_(path) {}

    bool isExisting() const;
    std::string readString() const;
    int32_t readInt32() const;
    float readFloat() const;
    void writeString(const std::string& str);
    void writeInt32(int32_t val);
    void writeFloat(float val);

private:
    std::string path_;
};
