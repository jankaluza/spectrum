#ifndef AUTOCONNECTLOOP_H
#define AUTOCONNECTLOOP_H

#include <string>
#include <map>

class GlooxMessageHandler;

class AutoConnectLoop
{
	public:
		AutoConnectLoop(GlooxMessageHandler *m);
		~AutoConnectLoop();
		bool restoreNextConnection();
	
	private:
		GlooxMessageHandler *main;
		std::map<std::string,int> m_probes;
	
};

#endif
