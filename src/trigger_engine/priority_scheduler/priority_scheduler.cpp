//
// Created by xucong on 25-6-27.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "priority_scheduler.h"
#include <future>
#include <iostream>


namespace dcl {
namespace trigger {

PriorityScheduler::PriorityScheduler(std::shared_ptr<ThreadPool> threadPool)
        : thread_pool_(std::move(threadPool)) {}

PriorityScheduler::~PriorityScheduler() {
    stop_scheduling_.store(true);
    condition_.notify_all();
    if (scheduling_thread_.joinable()) {
        scheduling_thread_.join();
    }
}

void PriorityScheduler::AddTask(TriggerTask task) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    trigger_queue_.emplace(std::move(task));
    condition_.notify_one();
}

void PriorityScheduler::StartScheduling() {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    if (!scheduling_thread_.joinable()) {
        scheduling_thread_ = std::thread(&PriorityScheduler::ScheduleTasks, this);
    }

    condition_.notify_one();
}

void PriorityScheduler::ScheduleTasks() {
#if 1
        ProcessOneTriggerQueue();
#else
        ProcessMultiTriggerQueue();
#endif
}

//plan A: support only one trigger, for priority
void PriorityScheduler::ProcessOneTriggerQueue() {
    while (!stop_scheduling_.load()) {
        std::unique_ptr<TriggerTask> highest_priority_task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait_for(lock, std::chrono::milliseconds(100), [this] { 
                return !trigger_queue_.empty() || stop_scheduling_.load(); 
            });

            if (stop_scheduling_.load() || trigger_queue_.empty()) {
                continue;
            }
            
            highest_priority_task = std::make_unique<TriggerTask>(std::move(const_cast<TriggerTask&>(trigger_queue_.top())));
            std::cout << "[PriorityScheduler] Processing for " << highest_priority_task->trigger_name << " with priority(" 
                       << static_cast<int>(highest_priority_task->priority) << ").\n";
            trigger_queue_.pop();
        }

        if (highest_priority_task && !highest_priority_task->cancelled) {
            thread_pool_->enqueue([this, task = std::move(*highest_priority_task)]() {
                // std::cout << "[PriorityScheduler] Processing for " << task.trigger_name << " with priority(" 
                //           << static_cast<int>(task.priority) << ").\n";
                // task.trigger->Proc();
                while (!task.cancelled) {
                    task.trigger->Proc();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
                }
            });
        }
    }
}

// plan B: support multiple triggers
void PriorityScheduler::ProcessMultiTriggerQueue() {
    // 使用非阻塞方式获取锁，避免线程阻塞
    std::unique_lock<std::mutex> lock(queue_mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        return;
    }

    // 使用条件变量等待1s, 直到队列不为空或者超时
    schedule_condition_.wait_for(lock, std::chrono::milliseconds(1000), [this]() {
        return !waiting_queue_.empty() || !running_queue_.empty();
    });

    if (stop_scheduling_.load() || waiting_queue_.empty()) {
        return;
    }

    // 获取线程池大小和运行中的任务数
    const size_t thread_pool_size = thread_pool_->getThreadCount();
    const size_t running_tasks = running_queue_.size();
    while (running_queue_.size() < thread_pool_size && !waiting_queue_.empty()) {
        // 获取下一个任务
        auto next_task = std::move(const_cast<TriggerTask&>(waiting_queue_.top()));
        waiting_queue_.pop();
        waiting_priorities_.erase(next_task.priority);

        // 设置任务状态为运行中
        next_task.state = TaskState::RUNNING;
        auto task_ptr = std::make_shared<TriggerTask>(next_task);
        lock.unlock();
        // LOG_INFO("Task [%s] started running (priority: %d)", task_ptr->trigger_name.c_str(), task_ptr->priority);

        // 提交任务到线程池
        thread_pool_->enqueue([this, task_ptr]() {
            ExecuteTask(task_ptr);
        });
        lock.lock(); // 重新获取锁
    }

    // 检查是否需要抢占
    if (!waiting_queue_.empty() && !running_queue_.empty()) {
        CheckPreemption();
    }
}

/**
 * @brief 执行指定的触发任务
 *
 * 该函数负责处理任务的执行逻辑，包括检查任务是否被取消、
 * 判断重试间隔、检查触发条件以及根据执行结果决定是否需要重试。
 *
 * @param task_ptr 指向待执行任务的智能指针
 */
void PriorityScheduler::ExecuteTask(std::shared_ptr<TriggerTask> task_ptr) {
    // 如果任务已被取消，则直接标记为已完成并通知调度器
    if (task_ptr->cancelled) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_ptr->state = TaskState::FINISHED;
        schedule_condition_.notify_one();
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - task_ptr->last_attempt_time);

    // 检查重试间隔
    if (elapsed < task_ptr->retry_interval) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_ptr->state = TaskState::WAITING;
        waiting_queue_.emplace(*task_ptr);
        waiting_priorities_[task_ptr->priority] = true;
        schedule_condition_.notify_one();
        return;
    }

    // 检查条件并执行任务
    if (task_ptr->trigger->CheckCondition()) {
        task_ptr->trigger->Proc();
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_ptr->state = TaskState::FINISHED;
    } else if (task_ptr->retry_count < task_ptr->max_retries) {
        // 需要重试
        task_ptr->retry_count++;
        task_ptr->last_attempt_time = now;

        AdjustTaskPriority(task_ptr);

        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_ptr->state = TaskState::WAITING;
        waiting_queue_.emplace(*task_ptr);
        waiting_priorities_[task_ptr->priority] = true;
    } else {
        // 达到最大重试次数
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_ptr->state = TaskState::FINISHED;
    }

    schedule_condition_.notify_one();
}

void PriorityScheduler::CheckPreemption() {
    auto& highest_waiting = waiting_queue_.top();
    auto& lowest_running = running_queue_.top();

    // 如果等待队列中的最高优先级任务比运行队列中的最低优先级任务优先级更高
    if (highest_waiting.priority < lowest_running.priority) {
        // 取消运行队列中优先级最低的任务
        auto preempted_task = const_cast<TriggerTask&>(running_queue_.top());
        running_queue_.pop();

        preempted_task.cancelled = true;
        preempted_task.state = TaskState::WAITING;

        waiting_queue_.emplace(preempted_task);
        waiting_priorities_[preempted_task.priority] = true;

        // 通知调度线程有新任务需要处理
        schedule_condition_.notify_one();
    }
}

void PriorityScheduler::AdjustTaskPriority(std::shared_ptr<TriggerTask> task_ptr) {
    const int ADJUST_PRIORITY_THRESHOLD = 3;  // 连续失败3次后开始调整优先级
    if (task_ptr->retry_count > ADJUST_PRIORITY_THRESHOLD) {
        const int8_t MIN_PRIORITY = 19;  // 最大优先级值
        int8_t new_priority = task_ptr->priority + (task_ptr->retry_count - ADJUST_PRIORITY_THRESHOLD);
        task_ptr->priority = std::min(new_priority, MIN_PRIORITY);

        LOG_INFO("Task [%s] priority adjusted from %d to %d after %d retries",
                 task_ptr->trigger_name.c_str(),
                 task_ptr->priority - (task_ptr->retry_count - ADJUST_PRIORITY_THRESHOLD),
                 task_ptr->priority,
                 task_ptr->retry_count);
    }
}

} 
} 