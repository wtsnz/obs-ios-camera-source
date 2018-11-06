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

#include "Thread.hpp"

Thread::Thread(): mThread(nullptr), mRunning(false), mShouldStop(false) { }

Thread::~Thread()
{
    if (mThread == nullptr) {
        return;
    }

    mShouldStop = true;
    if (mRunning && mThread->joinable()) {
        mThread->join();
    }

    delete mThread;
    mThread = nullptr;
}

void Thread::start()
{
    if (mThread != nullptr) {
        return;
    }

    mShouldStop = false;

    mThread = new std::thread([this]{
        this->run();
    });

    mRunning = true;
}

void Thread::join()
{
    if (mThread == nullptr) {
        return;
    }

    mShouldStop = true;

    if (mThread->joinable()) {
        mThread->join();
    }

    delete mThread;
    mThread = nullptr;

    mRunning = false;
}

