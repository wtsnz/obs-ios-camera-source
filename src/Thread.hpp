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

#ifndef Thread_hpp
#define Thread_hpp

#include <stdio.h>
#include <thread>
#include <atomic>

class Thread
{
public:
    Thread();
    virtual ~Thread();
    
    void start();
    void join();
    
    std::thread self();
    
    virtual void* run() = 0;
    
    bool shouldStop() {
        return mShouldStop.load();
    }
    
private:
    
    std::thread *mThread;
    std::atomic<bool> mRunning;
    std::atomic<bool> mShouldStop;
};

#endif /* Thread_hpp */
