#ifndef _SIPLISTENER_H_
#define _SIPLISTENER_H_	1

#include "SipMessage.h"

class SipListener
{
	// typedef bool (SipListener::TonSendMessage) (SipMessage &m);
public:
	SipListener();
	SipListener *next;
	virtual bool sendMessage(SipMessage &m) = 0;
};

#endif
