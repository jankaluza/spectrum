#ifndef _HI_STATS_H
#define _HI_STATS_H

#include <gloox/stanza.h>

class GlooxMessageHandler;

// typedef std::list<Tag*> TagList;

using namespace gloox;


class GlooxStatsHandler : public IqHandler
{
	public:
		GlooxStatsHandler(GlooxMessageHandler *parent);
		~GlooxStatsHandler();
		bool handleIq (Stanza *stanza);
		bool handleIqID (Stanza *stanza, int context);
		GlooxMessageHandler *p;
	
		void messageFromLegacy() { m_messagesOut++; }
		void messageFromJabber() { m_messagesIn++; }
	
	private:
		long m_messagesIn;		// messages from Jabber
		long m_messagesOut;		// messages from legacy network
		time_t m_startTime;
};

#endif
