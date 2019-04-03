/*
 * Copyright (C) 2019 Pinecone Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include "pthread.h"

class ThreadBase {
public:
    ThreadBase() { }
    virtual ~ThreadBase() { }
    bool start_thread();
    void wait_exit();
    virtual void _thread_entry() = 0;

private:
    static void * _thread_entry_func(void *arg);
    pthread_t _thread;
};
