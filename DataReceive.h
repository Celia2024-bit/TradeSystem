#ifndef DATARECEIVE_H
#define DATARECEIVE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <random>
#include "types.h"
#include "SafeQueue.h"
#include "TradeAlgorithm.h"

class DataReceive
{
private:
    SafeQueue<TradeData>& dataQueue_;       
    SafeQueue<ActionSignal>& actionQueue_;  
    std::condition_variable& dataCV_;       
    std::condition_variable& actionCV_;    
    std::mutex& priceMutex_;                
    std::mutex& actionMutex_;               
    std::atomic<bool> isReceivingData_;     
    doubleVect recentPrices_;         
    TradeAlgorithm tradeAl_; 
public:
    DataReceive() = delete;
    DataReceive(SafeQueue<TradeData>& dataQueue, SafeQueue<ActionSignal>& actionQueue,
                std::condition_variable& dataCV, std::mutex& priceMutex,
                std::condition_variable& actionCV, std::mutex& actionMutex);

    void StartReceiveData();

    void StopReceiveData();

    void ProcessDataAndGenerateSignals();
};

#endif // DATARECEIVE_H