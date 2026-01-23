//
// Created by xucong on 25-6-27.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <queue>
#include "scheduler.h"

namespace dcp::trigger
{

struct TaskComparator {
    bool operator()(const TriggerTask& a, const TriggerTask& b) const {
        return a.priority > b.priority;
    }
};

using TaskPriorityQueue = std::priority_queue<TriggerTask, std::vector<TriggerTask>, TaskComparator>;

}

#endif // TASK_QUEUE_H
