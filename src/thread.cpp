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

Thread::Thread() {
	m_mutex = g_mutex_new();
	m_cond = g_cond_new();
	m_stop = false;
	m_thread = NULL;
}

Thread::~Thread() {
	g_mutex_free(m_mutex);
	g_cond_free(m_cond);
}

void Thread::lockMutex() {
	g_mutex_lock(m_mutex);
}

void Thread::unlockMutex() {
	g_mutex_unlock(m_mutex);
}

void Thread::wait() {
	g_cond_wait(m_cond, m_mutex);
}

void Thread::wakeUp() {
	g_cond_signal(m_cond);
}

void Thread::stop() {
	g_mutex_lock(m_mutex);
	m_stop = true;
	g_mutex_unlock(m_mutex);
}

void Thread::run(GThreadFunc func, gpointer data) {
	m_thread = g_thread_create(func, data, true, NULL);
}

void Thread::join() {
	if (m_thread)
		g_thread_join(m_thread);
}

void Thread::stopped() {
	m_thread = NULL;
}

bool Thread::shouldStop() {
	g_mutex_lock(m_mutex);
	bool stop = m_stop;
	g_mutex_unlock(m_mutex);
	return stop;
}

bool Thread::isRunning() {
	return m_thread != NULL;
}
