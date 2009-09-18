/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#ifndef THREAD_H
#define THREAD_H

#include <iostream>

#if WIN32
#include <windows.h>
#else
#define HAVE_PTHREADS
#include <pthread.h>
#include <errno.h>
#endif

class Thread;

#ifdef HAVE_PTHREADS
void *thread_thread(void *v);
#elif WIN32
DWORD WINAPI thread_thread(void *v);
#endif

class Thread {
#ifdef HAVE_PTHREADS
        pthread_t m_id;
#elif WIN32
    HANDLE m_handle;
    DWORD m_id;
#endif
public:
    Thread();
    virtual ~Thread();

    virtual void exec() = 0;
    void run();
    void join();
};

/*
00055 struct Function {
00056     Thread *t;
00057     void (Thread::*f)();
00058 };
00059 */

class MyMutex {
    #ifdef HAVE_PTHREADS
                pthread_mutex_t m_mutex;
    #elif WIN32
        CRITICAL_SECTION m_cs;
     #endif
public:
    MyMutex();

    ~MyMutex();

        void lock();

        bool tryLock();

        void unlock();
};
#endif
