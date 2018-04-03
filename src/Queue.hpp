/*
 obs-ios-camera-source
 Copyright (C) 2018 Will Townsend <will@townsend.io>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program. If not, see <https://www.gnu.org/licenses/>
 */

#ifndef WorkQueue_hpp
#define WorkQueue_hpp

#include <list>
#include <mutex>
#include <condition_variable>

class PacketItem
{
    std::vector<char> mPacket;
    
public:
    PacketItem(std::vector<char> packet): mPacket(packet) { }
    
    std::vector<char> getPacket() {
        return mPacket;
    }
};

template <typename T> class WorkQueue
{
    std::list<T>   m_queue;
    std::mutex mMutex;
    std::condition_variable mConditionVariable;
    
public:
    WorkQueue() {
    }
    ~WorkQueue() {
    }
    void add(T item) {
        mMutex.lock();
        m_queue.push_back(item);
        mConditionVariable.notify_all();
        mMutex.unlock();
        printf("Added item. item count: %d\n", this->size());
    }
    T remove() {
        std::unique_lock<std::mutex> lck (mMutex);
        while (m_queue.size() == 0) {
            mConditionVariable.wait(lck);
        }
        T item = m_queue.front();
        m_queue.pop_front();
        lck.unlock();
        printf("Removed item. item count: %d\n", this->size());
        return item;
    }
    int size() {
        mMutex.lock();
        int size = m_queue.size();
        mMutex.unlock();
        return size;
    }
};

#endif
