//
// Created by xucong on 25-1-13.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#pragma once

#include <boost/circular_buffer.hpp>
#include <boost/core/noncopyable.hpp>
#include <functional>

namespace dcp::common{

template <typename _T>
class TimeSeriesBuffer : private boost::noncopyable {
public:
  using GetTime = std::function<int64_t(_T)>;
  TimeSeriesBuffer(const size_t& capacity, GetTime&& func)
    : buffer_(capacity), getTime(std::forward<GetTime>(func)) {}
  ~TimeSeriesBuffer() = default;

  inline bool push(const _T& data) {
    if (!buffer_.empty() && getTime(buffer_.back()) > getTime(data)) {
      return false;
    }
    buffer_.push_back(data);
    return true;
  }

  inline bool pop() {
    if (buffer_.empty()) {
      return false;
    }
    buffer_.pop_front();
    return true;
  }

  // find left and right for interpolation, where left <= data <= right
  // bool between(const _T& data, _T* left, _T* right) const {
  bool between(const int64_t& time, _T* const left, _T* const right) const {  // PRQA S 6006
    auto iter =
      std::lower_bound(buffer_.begin(), buffer_.end(), time,
                       [this](const _T& data, const int64_t& time) { return getTime(data) < time; });
    assert(iter == buffer_.end() || time <= getTime(*iter));
    if (iter == buffer_.end()) {
      return false;
    }
    if (time == getTime(*iter)) {
      if (std::distance(iter, buffer_.end()) > 1) {
        *left = *iter;
        *right = *(iter + 1);
        return true;
      } else if (std::distance(buffer_.begin(), iter) > 0) {
        *left = *(iter - 1);
        *right = *iter;
        return true;
      }
      return false;
    }
    if (std::distance(buffer_.begin(), iter) > 0) {
      *left = *(iter - 1);
      *right = *iter;
      return true;
    }
    return false;
  }

  inline size_t size() const noexcept { return buffer_.size(); }

  inline _T& at(const size_t& index) { return buffer_.at(index); }

  inline const _T& at(size_t index) const { return buffer_.at(index); }

  inline bool empty() const noexcept { return buffer_.empty(); }

  inline void clear() noexcept { buffer_.clear(); }

  _T& front() { return buffer_.front(); }

  _T& back() { return buffer_.back(); }

  const _T& front() const { return buffer_.front(); }

  const _T& back() const { return buffer_.back(); }

  int64_t frontTime() const { return getTime(buffer_.front()); }

  int64_t backTime() const { return getTime(buffer_.back()); }

private:
  boost::circular_buffer<_T> buffer_;
  GetTime getTime;
};

}  
