//
// Created by xucong on 25-9-24.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef OBSERVER_H
#define OBSERVER_H

// #include "ad_rscl/ad_rscl.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <functional>


// TODO: 替换为实际的ReceivedMsg定义
struct ReceivedMsgBase {
    virtual ~ReceivedMsgBase() = default;
};

template<typename T>
struct ReceivedMsg : public ReceivedMsgBase {
    T data;
};

namespace senseAD {
namespace rscl {
namespace comm {
    struct RawMessage {
        std::string content;
    };
}
}
}

using TRawMessagePtr = std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>;

namespace dcl {
namespace channel {

class Observer {
public:
    virtual ~Observer() = default;

    virtual void OnMessageReceived(const std::string& topic, const TRawMessagePtr& subject) = 0;
};

class Subject {
public:
    virtual~Subject() = default;

    void addObserver(std::shared_ptr<Observer> observer) {
        observers_.emplace_back(observer);
    }


    void removeObserver(std::shared_ptr<Observer> observer) {
        auto iter = std::find(observers_.begin(), observers_.end(), observer);
        if (iter != observers_.end()) {
            observers_.erase(iter);
        }
    }

    void notifyAll(const std::string& topic, const TRawMessagePtr& subject) {
        for (const auto& observer : observers_) {
            observer->OnMessageReceived(topic, subject);
        }
        // std::cout << "notify all observers, topic: " << topic << std::endl;
    }

    const std::vector<std::shared_ptr<Observer>>& getObservers() const { return observers_; }

protected:
    std::vector<std::shared_ptr<Observer>> observers_;
};

} 
} 

#endif
