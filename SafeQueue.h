#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono> 
#include "types.h" 

template <typename T>
class SafeQueue
{
private:
    std::queue<T> queue_; 
    mutable std::mutex mutex_;
    std::condition_variable condition_;

public:
    void push(const T& data);

    T pop();

    size_t size() const;

    bool tryPop(T& data);

    bool empty() const;
};


template <typename T>
void SafeQueue<T>::push(const T& data)
{
    std::lock_guard<std::mutex> lock(mutex_); 
    queue_.push(data);
    condition_.notify_one();
}

template <typename T>
T SafeQueue<T>::pop()
{
    std::unique_lock<std::mutex> lock(mutex_); 
    condition_.wait(lock, [this] { return !queue_.empty(); });
    T data = queue_.front();
    queue_.pop();
    return data;
}

template <typename T>
size_t SafeQueue<T>::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();e
}

template <typename T>
bool SafeQueue<T>::tryPop(T& data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty())
    {
        return false;
    }
    else
    {
        data = queue_.front();
        queue_.pop();
        return true;
    }
}

template <typename T>
bool SafeQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

#endif // SAFEQUEUE_H