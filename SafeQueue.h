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
    std::queue<T> dataQueue_; 
    mutable std::mutex queueMutex_;
    std::condition_variable queueConditionVariable_;

public:
    void enqueue(const T& data);

    T dequeue();

    size_t size() const;

    bool tryDequeue(T& data);

    bool empty() const;
};


template <typename T>
void SafeQueue<T>::enqueue(const T& data)
{
    std::lock_guard<std::mutex> lock(queueMutex_); 
    dataQueue_.push(data);
    queueConditionVariable_.notify_one();
}

template <typename T>
T SafeQueue<T>::dequeue()
{
    std::unique_lock<std::mutex> lock(queueMutex_); 
    queueConditionVariable_.wait(lock, [this] { return !dataQueue_.empty(); });
    T data = dataQueue_.front();
    dataQueue_.pop();
    return data;
}

template <typename T>
size_t SafeQueue<T>::size() const
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    return dataQueue_.size();
}

template <typename T>
bool SafeQueue<T>::tryDequeue(T& data)
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (dataQueue_.empty())
    {
        return false;
    }
    else
    {
        data = dataQueue_.front();
        dataQueue_.pop();
        return true;
    }
}

template <typename T>
bool SafeQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    return dataQueue_.empty();
}

#endif // SAFEQUEUE_H