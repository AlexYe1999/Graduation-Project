#pragma once
#include "Utility.hpp"

class Texture{
public:
    Texture(const std::string& fileName = "")
        : m_width(0)
        , m_height(0)
        , m_channel(0)
        , m_bits(0)
    {}

    virtual ~Texture(){}

protected:
    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_channel;
    uint16_t m_bits;
};