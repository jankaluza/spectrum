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

#include "spectrumtimer.h"
#include "main.h"
#include "log.h"

static gboolean _callback(gpointer data) {
	SpectrumTimer *t = (SpectrumTimer*) data;
	return t->timeout();
}

SpectrumTimer::SpectrumTimer (int time, SpectrumTimerCallback callback, void *data) {
	m_data = data;
	m_callback = callback;
	m_timeout = time;
	m_id = 0;
}

SpectrumTimer::~SpectrumTimer() {
	stop();
}

void SpectrumTimer::start() {
	if (m_id == 0) {
		if (m_timeout >= 1000)
			m_id = purple_timeout_add_seconds(m_timeout / 1000, _callback, this);
		else
			m_id = purple_timeout_add(m_timeout, _callback, this);
	}
}

void SpectrumTimer::stop() {
	if (m_id != 0) {
		purple_timeout_remove(m_id);
		m_id = 0;
	}
}

gboolean SpectrumTimer::timeout() {
	gboolean ret = m_callback(m_data);
	if (!ret)
		m_id = 0;
	return ret;
}


