#pragma once

/**
 * Interface for basic I/O ports able to read and write bytes.
 *
 * Implementations can be an actual hardware port (e.g. serial line)
 * or a lower level protocol.
 */
class IIoPort
{
public:
    virtual ~IIoPort() {}

    virtual int write(const void* buf, int len) =0;
    virtual int read(void* buf, int len) =0;
};
