//
// Created by xucong on 25-6-27.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef PRIORITY_SCHEDULER_H
#define PRIORITY_SCHEDULER_H

#include "scheduler.h"
#include "task_queue.h"
#include <memory>
#include <mutex>
#include "ThreadPool/ThreadPool.h"

namespace dcp::trigger
{

class PriorityScheduler : public Scheduler {
public:
    PriorityScheduler(std::shared_ptr<ThreadPool> threadPool);
    ~PriorityScheduler() override;
    void AddTask(TriggerTask task) override;
    void StartScheduling() override;

private:
    void ScheduleTasks();
    void ProcessOneTriggerQueue();
    void ProcessMultiTriggerQueue();
    void ExecuteTask(std::shared_ptr<TriggerTask> task_ptr);
    void CheckPreemption();
    void AdjustTaskPriority(std::shared_ptr<TriggerTask> task_ptr);
    std::shared_ptr<ThreadPool> thread_pool_;
    TaskPriorityQueue trigger_queue_;
    std::mutex queue_mutex_;
    std::mutex schedule_mutex_;
    std::condition_variable condition_;
    std::thread scheduling_thread_;
    std::atomic<bool> stop_scheduling_{ false};

    //
    TaskPriorityQueue waiting_queue_;
    TaskPriorityQueue running_queue_;
    std::unordered_map<int8_t, bool> waiting_priorities_;
    std::condition_variable schedule_condition_;
};

}

#endif // PRIORITY_SCHEDULER_H
