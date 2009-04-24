#ifndef THREAD_H
#define THREAD_H

#include <iostream>

#define HAVE_PTHREADS

#ifdef HAVE_PTHREADS
#include <pthread.h>
#include <errno.h>
#elif WIN32
#include <windows.h>
#else
        #error NO THREADING SUPPORT!!! THIS SOFTWARE WILL NOT RUN!
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
