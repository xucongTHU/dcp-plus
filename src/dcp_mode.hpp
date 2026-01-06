#pragma once

namespace dcp {

enum class DcpMode {
    STANDARD,           // 基础版：基于规则+AI语义触发（影子模式）
    PRO,                // 专业版：固定ValueMap + 影子模式
    PLUS                // 专业增强版：可解释/动态可调ValueMap + RLPlanner + Safety Layer
};

} // namespace dcp