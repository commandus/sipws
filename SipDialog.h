#ifndef SIPDIALOG_H
#define SIPDIALOG_H	1

#include <string>
#include <map>
#include "json/json.h"
#include "SipAddress.h"
#include "SipMessage.h"
#include "SipRegistry.h"
#include "AuthHeaderMD5.h"
#include "Logger.h"

/**
* Parse {@link SipMessage} and return one or more {@link SipMessage}'s - reply for
* initiator, request for callee.
* Pass phone user id and password to {@link SipDialogFactory#getInstance(Context, std::string, std::string)}
* which is used in the produce messages 401- digest authenticate of user and authorize user.
* <br/>
* When SIP server started, provide SIP server address and port by {@link #setServiceInfo(InetAddress, int)}
* <br/>
* Then process messages with {@link SipDialogFactory#mkResponse(InetAddress, int, SipMessage)}}
*
*/
class SipDialog
{
private:
	Logger *logger;
	size_t mTag;
	SipRegistry *mRegistry;
	AuthHeaderMD5 *mAuthAcct;
	/**
	* Validate transport
	* @param proto transport protocol
	* @param headers headers to correct
	*/
	void validateTransport(TProto proto, std::map<std::string, std::string> &headers);
	void addWWWAuthenticate(struct sockaddr_in *svcsocket, const std::string &method, std::map<std::string, std::string> &headers, SipMessage &r);
	bool authenticate(const std::string &method, std::map<std::string, std::string> &headers);
public:
	SipDialog(SipRegistry *registry, Logger *logger);
	~SipDialog();
	std::string getTag();
	/**
	* process responses
	* @param proto
	* @param sender
	* @param senderPort
	* @param m	{@link SipMessage}
	* @return
	*/
	std::vector<SipMessage> &mkResponse(struct sockaddr_in *svcsocket, struct sockaddr_in *sender, SipMessage &m, std::vector<SipMessage> &result, time_t now);
	std::vector<SipMessage> &mkResponse(struct sockaddr_in *svcsocket, SipMessage &m, std::vector<SipMessage> &result, time_t now);
	std::vector<SipMessage> &processResponse(struct sockaddr_in *svcsocket, TProto proto, struct sockaddr_in *sender, int senderPort, SipMessage &m, 
		std::vector<SipMessage> &result, time_t registered);
	void updateLocation(struct sockaddr_in *sender, SipMessage &m, SipMessage &r, time_t registered);
};

#endif
