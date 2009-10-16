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
        /*int status = */pthread_create(&m_id, NULL, thread_thread, this);
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

WaitCondition::WaitCondition() {
#ifdef HAVE_PTHREADS
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&m_cond, NULL);
#else
	m_handle = NULL;
	m_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
}

WaitCondition::~WaitCondition() {
#ifdef HAVE_PTHREADS
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_cond);
#else
	if (m_handle) {
		CloseHandle(m_handle);
	}
#endif
}

void WaitCondition::wait() {
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&m_mutex);
	pthread_cond_wait(&m_cond, &m_mutex);
	pthread_mutex_unlock(&m_mutex);
#else
	bool wait_success = false;
	if (m_handle) {
		wait_success = WaitForSingleObject(m_handle, INFINITE) == WAIT_OBJECT_0;
	}
#endif
}

void WaitCondition::wakeUp() {
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&m_mutex);
	pthread_cond_broadcast(&m_cond);
	pthread_mutex_unlock(&m_mutex);
#else
	if (m_handle) {
		SetEvent(m_handle);
	}
#endif
}

