/*
 * Copyright (C) 2019 FishSemi Inc. All rights reserved.
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

#include "thread_base.h"

bool ThreadBase::start_thread()
{
    return (pthread_create(&_thread, NULL, _thread_entry_func, this) == 0);
}

void ThreadBase::wait_exit()
{
    (void) pthread_join(_thread, NULL);
}

void* ThreadBase:: _thread_entry_func(void *arg)
{
    ((ThreadBase *)arg)->_thread_entry();
    return NULL;
}
