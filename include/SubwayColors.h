#ifndef SUBWAY_COLORS_H
#define SUBWAY_COLORS_H

#include <cstdint>
#include <map>
#include <string>

class SubwayColorMap {
public:
    enum SubwayColor : uint32_t {
        Blue = 0x0039a6,
        Orange = 0xff6319,
        Lime = 0x6cbe45,
        Gray = 0xa7a9ac,
        Brown = 0x7B3F00,
        Yellow = 0xfccc0a,
        Red = 0xee352e,
        Green = 0x00933c,
        Purple = 0xb933ad,
        Turquoise = 0x00add0,
        Default = 0xffffff
    };
    SubwayColorMap();
    SubwayColor getColor(const std::string& routeId) const;

private:
    std::map<std::string, SubwayColor> routeColors;
};

#endif // SUBWAY_COLORS_H