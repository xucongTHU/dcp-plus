#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace dcp::trigger {

struct Point
{
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

struct TriggerContext {
    std::string businessType;
    std::string triggerId;
    int64_t triggerTimestamp;
    std::string triggerDesc;
    Point pos;
};

}