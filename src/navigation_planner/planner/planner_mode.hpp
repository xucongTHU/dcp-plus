#pragma once

namespace dcp {
namespace planner {

enum class PlannerMode {
    STANDARD,           // 基础版：基于规则的规划
    PRO,                // 专业版：A* + 规则优化
    PLUS                // 专业增强版：ValueMap + RL + Safety Layer
};

} // namespace planner
} // namespace dcp