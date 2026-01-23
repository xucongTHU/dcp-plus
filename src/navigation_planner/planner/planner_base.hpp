// dcp/planner/planner_base.hpp
#ifndef LANNER_BASE_HPP
#define PLANNER_BASE_HPP

#include <vector>
#include "../value_map/costmap.h"

namespace dcp::planner {

struct PlannerInput {
    Point start;
    Point goal;
    CostMap* costmap;
    
    PlannerInput(const Point& s = Point(), const Point& g = Point(), CostMap* cm = nullptr)
        : start(s), goal(g), costmap(cm) {}
};

class PlannerBase {
public:
    virtual ~PlannerBase() = default;

    virtual void reset() = 0;

    virtual Trajectory plan(const PlannerInput& input) = 0;
    
    // virtual void UpdateConfiguration(const std::map<std::string, double>& config) = 0;
};

} // namespace dcp::planner

#endif // PLANNER_BASE_HPP