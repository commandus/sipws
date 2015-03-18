#include <string.h>
#include "SipDialog.h"
#include "sipwsUtil.h"

SipDialog::SipDialog(SipRegistry *registry, Logger *logger) 
	: mTag(0), logger(logger), mRegistry(registry)
{
	// authenticate users to access /acct
	mAuthAcct = new AuthHeaderMD5(mRegistry->Domain);
}

SipDialog::~SipDialog()
{
	delete mAuthAcct;
}

std::string SipDialog::getTag()
{
	mTag++;
	if (mTag > 999999)
		mTag = 1;
	return size_t2Dec(mTag);
}

std::vector<SipMessage> &SipDialog::mkResponse(struct sockaddr_in *svcsocket, SipMessage &m, std::vector<SipMessage> &result, time_t now)
{
	return mkResponse(svcsocket, &m.Address, m, result, now);
}

// update location
void SipDialog::updateLocation(struct sockaddr_in *sender, SipMessage &m, SipMessage &r, time_t registered)
{
	int remotePort = parseInt(m.getHeaderParameter("V", "rport"), sender->sin_port);
	// int remotePort = sender->sin_port;
	SipAddress aFrom(m.Proto, m.Headers["T"]);	// "T" not "M" Contact
	if (aFrom.Port != 0)
		remotePort = aFrom.Port;
	int expires = m.getExpires();
	mRegistry->put(m.Proto, aFrom.Id, mRegistry->Domain, aFrom.Tag, addr2String(sender), remotePort, expires, registered);

	// delete Authorization header
	r.rmHeader("Authorization");
	// change expires
	std::string sexpires(int2Dec(expires));
	r.Headers["Expires"] = sexpires;
	// r.addHeaderParameter("M", "expires", sexpires);	// 2015/03/04
	r.addHeaderParameter("V", "received", addr2String(sender));
}

std::vector<SipMessage> &SipDialog::mkResponse(struct sockaddr_in *svcsocket, struct sockaddr_in *sender, SipMessage &m, std::vector<SipMessage> &result, time_t now)
{
	int maxForwards = m.getMaxForwards();
	if (maxForwards == 0) 
		return result;
	maxForwards--;
	m.setMaxForwards(maxForwards);

	// response
	if (m.mCommand == C_RESPONSE)
		return processResponse(svcsocket, m.Proto, sender, sender->sin_port, m, result, now);

	// commands
	SipMessage r(m.Proto, svcsocket, sender, m);

	if (!r.existHeaderParameter("T", "tag"))	// To
		r.addHeaderParameter("T", "tag", getTag());
	r.replaceHeaderParameter("V", "rport", "rport=" + int2Dec(m.Address.sin_port));// m.Port senderPort
	// r.rmHeaderParameter("V", "rport");

	std::string branch = m.getHeaderParameter("V", "branch");
	if (branch.length() == 0)
		branch = "branch=z9hG4bK" + getTag();
	else
		branch = "branch=" + branch;
	SipAddress toAddress;
	bool toExists = mRegistry->getByAddress(m.Headers["T"], toAddress);
	bool toRegistered = toExists && toAddress.Availability == AVAIL_YES;
	SipAddress fromAddress;
	bool fromExists = mRegistry->getByAddress(m.Headers["F"], fromAddress);
	
	if (fromExists)
		m.KeyFrom = fromAddress.Id + "@" + fromAddress.Domain;

	if (toExists)
		m.Key = toAddress.Id + "@" + toAddress.Domain;

	if ((m.mCommand != C_REGISTER) && fromAddress.isExpired(now))
	{
		if (authenticate(getCommandName(m.mCommand), m.Headers))
		{
			// update location
			updateLocation(sender, m, r, now);
		}
		else
		{
			addWWWAuthenticate(svcsocket, REGISTER, m.Headers, r);
			r.Sdp = "";
			result.push_back(r);
			return result;
		}
	}

	switch (m.mCommand)
	{
	case C_ACK:
	case C_CANCEL:
		/*
		r.addHeaderParameter("V", "received", addr2String(sender));
		r.Sdp = null;
		rr.push_back(r);
		*/
		r.Headers["L"] = "0";
		if (!toRegistered)
		{
			// inform caller not registered
			r.mCode = NOT_FOUND;
			r.Sdp = "";
			result.push_back(r);
			// Log.e(TAG, m.Headers["T"] + " not found in locations");
			break;
		}

		// inform callee
		{
			SipMessage ackCancel(m);
			ackCancel.mCommandParam = toAddress.getAddress();
			if (!string2addr(toAddress.Host.c_str(), &ackCancel.Address))
				break;
			ackCancel.Proto = toAddress.Proto;
			ackCancel.setPort(toAddress.Port);
			ackCancel.Headers["T"] = toAddress.getAddressTag();
			validateTransport(toAddress.Proto, ackCancel.Headers);
			ackCancel.addHeaderParameter("V", "received", addr2String(sender));
			result.push_back(ackCancel);
		}
		break;
	case C_INVITE:
		if (!toRegistered)
		{
			// inform caller not registered
			r.mCode = NOT_FOUND;
			r.Sdp = "";
			r.Headers["L"] = "0";
			result.push_back(r);
			// Log.e(TAG, m.Headers["T") + " not found in locations");
			break;
		}

		{
			// inform callee
			SipMessage invite(m);
			invite.mCommand = C_INVITE;
			invite.mCommandParam = toAddress.getAddress();
			if (!string2addr(toAddress.Host.c_str(), &invite.Address))
				break;
			invite.Proto = toAddress.Proto;
			invite.setPort(toAddress.Port);
			invite.Headers["T"] = toAddress.getAddressTag();
			validateTransport(toAddress.Proto, invite.Headers);
			std::string a;
			if (sender->sin_port == 0)
			{
				// received from WS/WSS, set address of UDP/TCP to the default IP address
				a = mRegistry->Ip;
			}
			else
				a = addr2String(sender);
			invite.addHeaderParameter("V", "received", a);
			result.push_back(invite);
		}
		break;
	case C_REGISTER:
		// update location of client registered to me
		// it can be broadcast REGISTER packet, NO skip it
		// if (!isHostLocal(m.Address)) {	
		{
			if (!authenticate(REGISTER, m.Headers)) 
				addWWWAuthenticate(svcsocket, REGISTER, m.Headers, r);
			else 
			{
				// update location
				updateLocation(sender, m, r, now);
			}
		}
		r.Sdp = "";
		result.push_back(r);
		break;
	case C_OPTIONS:
		{
			SipAddress aFrom;
			if (!mRegistry->getByAddress(m.Headers["F"], aFrom))
				break;
			if (aFrom.Id == toAddress.Id) {
				// request server
				r.Sdp = "";
				r.Headers.erase(r.Headers.find("Max-Forwards"));
				r.addHeaderParameter("V", "received", addr2String(sender));
				r.Headers["Allow"] = ALLOWED_METHODS;
				r.Headers["Accept"] = "application/sdp";
				r.Headers["Accept-Encoding"] = "";
				r.Headers["Accept-Language"] = "en";
				r.Headers["K"] = "";	// Supported
				r.Headers["Allow-Events"] = "";	// Allow-Events: presence, message-summary, refer

				result.push_back(r);
				break;
			}
			// ?!! request callee?
			SipMessage options(m);
			options.mCommandParam = toAddress.getAddress();
			if (!string2addr(toAddress.Host.c_str(), &options.Address))
				break;
			options.Proto = toAddress.Proto;
			options.setPort(toAddress.Port);
			options.Headers["T"] = toAddress.getAddressTag();
			validateTransport(toAddress.Proto, options.Headers);
			options.addHeaderParameter("V", "received", addr2String(sender));
			result.push_back(options);
		}
		break;
	case C_BYE:
		{
			// re-translate
			// inform callee
			SipMessage bye(m);
			if (!toRegistered)
				break;
			if (!string2addr(toAddress.Host.c_str(), &bye.Address))
				break;
			mRegistry->setAvailability(m.Proto, fromAddress.Id, mRegistry->Domain, AVAIL_NO);

			bye.Proto = toAddress.Proto;
			bye.setPort(toAddress.Port);
			bye.Headers["T"] = toAddress.getAddressTag();
			validateTransport(toAddress.Proto, bye.Headers);
			// bye.addHeaderParameter("V", "received", addr2String(sender));
			result.push_back(bye);
		}
		break;
	case C_MESSAGE:
		if (!toRegistered) {
			// inform caller not registered
			r.mCode = NOT_FOUND;
			r.Headers["L"] = "0";
			r.Sdp = "";
			result.push_back(r);
			// Log.e(TAG, m.Headers["T"] + "not found in locations");
			break;
		}
		// return to caller 
		r.mCode = OK;
		{
			std::string len = r.Headers["L"];
			r.Headers["L"] = "0";
			result.push_back(r);
			// send message to callee
			SipMessage message(m);
			// message.mCommandParam = "sip:" + toAddress.getAddress();
			if (!string2addr(toAddress.Host.c_str(), &message.Address))
				break;
			message.Proto = toAddress.Proto;
			message.setPort(toAddress.Port);
			message.Headers["T"] = toAddress.getAddressTag();
			// validateTransport(toAddress.Proto, message.Headers);
			
			message.Headers["V"] = "SIP/2.0/" + toUpper(toString(toAddress.Proto)) + " "
				+ addr2String(svcsocket) + ":" + int2Dec(svcsocket->sin_port) + ";" + branch;
			message.Headers["L"] = len;
			result.push_back(message);
		}
		break;
	case C_SUBSCRIBE:
		result.push_back(r);
		break;
	case C_INVALID:
	default:
		r.mCode = NOT_IMPLEMENTED;
		r.Sdp = "";
		result.push_back(r);
	}
	return result;
}

/**
* Validate transport
* @param proto transport protocol
* @param headers headers to correct
*/
void SipDialog::validateTransport(TProto proto, std::map<std::string, std::string> &headers) 
{
	std::string v = headers["V"];
	int p = v.find(' ');
	if (p > 0) {
		v = "SIP/2.0/" + toUpper(toString(proto)) + v.substr(p);
		headers["V"] = v;
	}
}

/**
* process responses
* @param proto
* @param sender
* @param senderPort
* @param m	{@link SipMessage}
* @return
*/
std::vector<SipMessage> &SipDialog::processResponse(struct sockaddr_in *svcsocket, TProto proto, struct sockaddr_in *sender, int senderPort, SipMessage &m, std::vector<SipMessage> &result, time_t now)
{
	//  180 RINGING Via: SIP/2.0/UDP 192.168.1.11:5061;rport=5060;b
	// Log.i(TAG, "processResponse: sender " + addr2String(sender) + ":" + Integer.toString(senderPort) + m.toString());
	// messages to skip Dont do it! Client must receive it
	/*
	if (m.mCode == 481) {
	return rr;
	}
	*/

	// get route
	std::string from = m.Headers["F"];
	if (&from == NULL) {
		// Log.e(TAG, "processResponse: From missed");
		return result;
	}

	SipAddress aFrom;
	if (!mRegistry->getByAddress(from, aFrom))
	{
		// Log.e(TAG, "processResponse: From unknown " + from + ", processing ");
		return result;
	}

	std::string keyfrom = aFrom.Id + "@" + aFrom.Domain;
	if ((aFrom.Proto == PROTO_UDP) || (aFrom.Proto == PROTO_TCP))
	{
		if (equalsIgnoreCase(aFrom.Host, addr2String(sender)) && (aFrom.Port == senderPort)) {
			// Log.e(TAG, "processResponse: cyclic response from " + from);
			return result;
		}
	}
	else 
	{
		/*
		if (keyfrom == "")
		{
		return result;
		}
		*/
	}

	// update location database when service receive response to the broadcast REGISTER message from any others 
	if (m.mCode == 200) {
		std::string cseq = m.Headers["CSeq"];
		if (&cseq != NULL) {
			if (endsWith(toUpper(cseq), "REGISTER")) {
				// if REGISTER response
				// Via: SIP/2.0/WS h078u4rlnb99.invalid;branch=z9hG4bK6761194;rport=51161;received=192.168.1.11
				std::string via = m.Headers["V"];
				if (&via != NULL) 
				{
					try 
					{
						int port = parseInt(m.getHeaderParameter("V", "rport"));
						// InetAddress a; if(!string2addr(m.getHeaderParameter("V", "received"), a));
						SipAddress fr(proto, m.Headers["T"]);
						// update location
						mRegistry->put(proto, fr.Id, mRegistry->Domain, fr.Tag, addr2String(&m.Address), port, m.getExpires(), now); // was "a"
					}
					catch (...)
					{
					}
				}
			}
		}
	}

	SipMessage retranslate(m);
	// retranslate.Headers["L", "0"];

	/*
	2015/03/04
	// Sometimes Android SIP client provide incorrect contact- like 5b..@192.168.1.33:5060 - port of SIP server on a client! So it fix. 
	std::string contact = retranslate.Headers["M"];
	if (&contact != NULL) 
	{
		SipAddress aContact(PROTO_UNKN, contact);
		if (mRegistry->getById(aContact.Id, addr2String(svcsocket), aContact))
			retranslate.Headers["M"] = aContact.toContact();
	}
	*/

	retranslate.Proto = aFrom.Proto;
	retranslate.Key = keyfrom;
	if (string2addr(aFrom.Host.c_str(), &retranslate.Address))
	{
		retranslate.setPort(aFrom.Port);
	}
	else
	{
		// Log.e(TAG, "processResponse: incorrect address");
		return result;
	}

	if (!(addrEquils(&retranslate.Address, svcsocket) && (retranslate.Address.sin_port == 5060)))
	{
		// check recursive
		retranslate.addHeaderParameter("V", "received", aFrom.Host);
		//validateTransport(proto, retranslate.Headers);
		result.push_back(retranslate);
	}
	return result;
}

/*
	Return 401 Unathorized and add WWW-Authenticate header
*/
void SipDialog::addWWWAuthenticate(sockaddr_in *svcsocket, const std::string &method, std::map<std::string, std::string> &headers, SipMessage &r)
{
	SipAddress a(PROTO_UNKN, headers["F"]);
	// no Authorization header, send WWW-Authenticate
	r.mCode = UNAUTHORIZED;
	std::string realm = mRegistry->Domain;
	std::string uri = "sip:" + addr2String(svcsocket);
	std::string s = mAuthAcct->getAuthenticate(a.Id, uri, realm);
	r.Headers[WWWAuthenticate] = s;
}

/*
	Return true if Authorization is valid
*/
bool SipDialog::authenticate(const std::string &method, std::map<std::string, std::string> &headers)
{
	AuthParams authparams(mRegistry->Domain, headers);
	// check is there Authorization header, check user
	std::string user = authparams.Username;
	if (user.empty())
		return false;
	// check does user exists
	SipAddress address;
	if (!mRegistry->getById(authparams.Username, authparams.Domain, address))
		return false;
	// check password for "owner" user over UDP or of all over web socket connection 
	if (!mAuthAcct->isAuthorized(authparams, method, address.Password, ""))
		return false;
	return true;
}


