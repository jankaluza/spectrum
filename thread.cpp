#include "thread.h"


#ifdef HAVE_PTHREADS
void *thread_thread(void *v) {
        static_cast<Thread*>(v)->exec();
        return 0;
}
#elif WIN32
DWORD WINAPI thread_thread(LPVOID v) {
    static_cast<Thread*>(v)->exec();
    return 0;
}
#else
#error NO THREADING SUPPORT
#endif

Thread::Thread() {
#ifdef HAVE_PTHREADS

#elif WIN32
    m_handle = CreateThread(0, 0, thread_thread, this, CREATE_SUSPENDED, &m_id);
#endif
}

Thread::~Thread() {
#ifdef HAVE_PTHREADS

#elif WIN32
    CloseHandle(m_handle);
#endif
}

void Thread::run() {
#ifdef HAVE_PTHREADS
        int status = pthread_create(&m_id, NULL, thread_thread, this);
#elif WIN32
    ResumeThread(m_handle);
#endif
}

void Thread::join() {
#ifdef HAVE_PTHREADS
        void *value;
        pthread_join(m_id, &value);
#elif WIN32
    WaitForSingleObject(m_handle, INFINITE);
#endif
}

MyMutex::MyMutex() {
#ifdef HAVE_PTHREADS
        pthread_mutex_init(&m_mutex, NULL);
#elif WIN32
    InitializeCriticalSection(&m_cs);
#endif
}

MyMutex::~MyMutex() {
#ifdef HAVE_PTHREADS
        pthread_mutex_destroy(&m_mutex);
#elif WIN32
    DeleteCriticalSection(&m_cs);
#endif
}

void MyMutex::lock() {
#ifdef HAVE_PTHREADS
        pthread_mutex_lock(&m_mutex);
#elif WIN32
    EnterCriticalSection(&m_cs);
#endif
}

bool MyMutex::tryLock() {
#ifdef HAVE_PTHREADS
        if (pthread_mutex_trylock(&m_mutex) != EBUSY) {
                return true;
        } else {
                return false;
        }
#elif WIN32
    if (TryEnterCriticalSection(&m_cs) != 0) {
        return true;
    } else {
        return false;
    }
#endif
        return false;
}

void MyMutex::unlock() {
#ifdef HAVE_PTHREADS
        pthread_mutex_unlock(&m_mutex);
#elif WIN32
    LeaveCriticalSection(&m_cs);
#endif
}
