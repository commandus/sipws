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

std::string SipDialog::getTag(SipMessage &m)
{
	std::string t = m.getHeaderParameter("F", "tag");
	if (t.empty())
	{
		mTag++;
		if (mTag > 999999)
			mTag = 1;
		return size_t2Dec(mTag);
	}
	else
		return t;
}

std::vector<SipMessage> &SipDialog::mkResponse(struct sockaddr_in *svcsocket, SipMessage &m, std::vector<SipMessage> &result, time_t now)
{
	return mkResponse(svcsocket, &m.Address, m, result, now);
}

/*
	update location
	Parameters
		m			incoming message
		r			answer
		registered	current time
*/
void SipDialog::updateLocation(struct sockaddr_in *sender, SipMessage &m, SipMessage &r, time_t registered)
{
	// "T" not "M" Contact
	SipAddress *a = mRegistry->getByAddress(m.Headers["T"]);
	if (!a)
		return;
	SipLocation *l = a->getLocation(registered);
	l->Expire = m.getExpires();
	if (l->Expire <= 0)
	{
		// delete location
		a->rmLocation(l, registered);
	}
	else
	{
		l->Port = sender->sin_port;
		a->Domain = mRegistry->Domain;
		// if (l.Port == 0) l.Port = parseInt(m.getHeaderParameter("V", "rport"), sender->sin_port);
		l->Proto = m.Proto;
		l->Host = addr2String(sender);
		l->Registered = registered;
		l->Prefix = PREFIX_SIP;	//	?!!
	}

	// delete Authorization header
	r.rmHeader("Authorization");
	// change expires
	r.Headers["Expires"] = int2Dec(l->Expire);
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
		r.addHeaderParameter("T", "tag", getTag(m));
	r.replaceHeaderParameter("V", "rport", "rport=" + int2Dec(m.Address.sin_port));// m.Port senderPort
	// r.rmHeaderParameter("V", "rport");

	std::string branch = m.getHeaderParameter("V", "branch");
	if (branch.length() == 0)
		branch = "branch=z9hG4bK" + getTag(m);
	else
		branch = "branch=" + branch;

	SipAddress *fromAddress = mRegistry->getByAddress(m.Headers["F"]);
	SipLocation *fromLocation = NULL;
	if (fromAddress)
	{
		m.KeyFrom = fromAddress->Id + "@" + fromAddress->Domain;
		fromLocation = fromAddress->getLocation(now);
		if ((m.mCommand != C_REGISTER) && fromAddress->isExpired(now))
		{
			AuthParams authparams(mRegistry->Domain, m.Headers);
			if (isByUser(authparams.Username, mRegistry->Domain, m) && authenticate(getCommandName(m.mCommand), authparams))
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
	}

	SipAddress *toAddress = mRegistry->getByAddress(m.Headers["T"]);
	bool toRegistered = (toAddress) && (!toAddress->isExpired(now));
	SipLocation *toLocation = NULL;
	if (toAddress)
	{
		m.Key = toAddress->Id + "@" + toAddress->Domain;
		toLocation = toAddress->getLocation(now);
	}

	switch (m.mCommand)
	{
	case C_ACK:
	case C_CANCEL:
		if (!toLocation)
			break;
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
			// not found in locations
			break;
		}

		// inform callee
		{
			SipMessage ackCancel(m);
			ackCancel.mCommandParam = toAddress->getAddress();
			if (!string2addr(toLocation->Host.c_str(), &ackCancel.Address))
				break;
			ackCancel.Proto = toLocation->Proto;
			ackCancel.setPort(toLocation->Port);
			ackCancel.Headers["T"] = toAddress->getAddressTag();
			validateTransport(toLocation->Proto, ackCancel.Headers);
			ackCancel.addHeaderParameter("V", "received", addr2String(sender));
			result.push_back(ackCancel);
		}
		break;
	case C_INVITE:
		if (!toLocation)
			break;
		if (!toRegistered)
		{
			// inform caller not registered
			r.mCode = NOT_FOUND;
			r.Sdp = "";
			r.Headers["L"] = "0";
			result.push_back(r);
			break;
		}

		{
			// inform callee
			SipMessage invite(m);
			invite.mCommand = C_INVITE;
			invite.mCommandParam = toAddress->getAddress();
			if (!string2addr(toLocation->Host.c_str(), &invite.Address))
				break;
			invite.Proto = toLocation->Proto;
			invite.setPort(toLocation->Port);
			invite.Headers["T"] = toAddress->getAddressTag();
			validateTransport(toLocation->Proto, invite.Headers);
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
			AuthParams authparams(mRegistry->Domain, m.Headers);
			if (isByUser(authparams.Username, mRegistry->Domain, m) && authenticate(REGISTER, authparams))
				updateLocation(sender, m, r, now);
			else 
				addWWWAuthenticate(svcsocket, REGISTER, m.Headers, r);
		}
		r.Sdp = "";
		result.push_back(r);
		break;
	case C_OPTIONS:
		{
			if (!toLocation)
				break;
			if (!fromAddress)
				break;
			if (fromAddress->Id == toAddress->Id)
			{
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
			options.mCommandParam = toAddress->getAddress();
			if (!string2addr(toLocation->Host.c_str(), &options.Address))
				break;
			options.Proto = toLocation->Proto;
			options.setPort(toLocation->Port);
			options.Headers["T"] = toAddress->getAddressTag();
			validateTransport(toLocation->Proto, options.Headers);
			options.addHeaderParameter("V", "received", addr2String(sender));
			result.push_back(options);
		}
		break;
	case C_BYE:
		{
			if (!toLocation)
				break;
			// re-translate
			// inform callee
			SipMessage bye(m);
			if (!toRegistered)
				break;
			if (!string2addr(toLocation->Host.c_str(), &bye.Address))
				break;
			bye.Proto = toLocation->Proto;
			bye.setPort(toLocation->Port);
			bye.Headers["T"] = toAddress->getAddressTag();
			validateTransport(toLocation->Proto, bye.Headers);
			// bye.addHeaderParameter("V", "received", addr2String(sender));
			result.push_back(bye);
		}
		break;
	case C_MESSAGE:
		if (!toLocation)
			break;
		if (!toRegistered) {
			// inform caller not registered
			r.mCode = NOT_FOUND;
			r.Headers["L"] = "0";
			r.Sdp = "";
			result.push_back(r);
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
			if (!string2addr(toLocation->Host.c_str(), &message.Address))
				break;
			message.Proto = toLocation->Proto;
			message.setPort(toLocation->Port);
			message.Headers["T"] = toAddress->getAddressTag();
			// validateTransport(toAddress.Proto, message.Headers);
			
			message.Headers["V"] = "SIP/2.0/" + toUpper(toString(toLocation->Proto)) + " "
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
* process responses like 180 RINGING Via: SIP/2.0/UDP 192.168.1.11:5061;rport=5060;b
* @param proto
* @param sender
* @param senderPort
* @param m	{@link SipMessage}
* @return
*/
std::vector<SipMessage> &SipDialog::processResponse(struct sockaddr_in *svcsocket, TProto proto, struct sockaddr_in *sender, int senderPort, SipMessage &m, std::vector<SipMessage> &result, time_t now)
{
	// get route
	SipAddress *aFrom = mRegistry->getByAddress(m.Headers["F"]);
	if (aFrom == NULL)
		return result;

	std::string keyfrom = aFrom->Id + "@" + aFrom->Domain;
	SipLocation *aFromLocation = aFrom->getLocation(now);
	if ((aFromLocation->Proto == PROTO_UDP) || (aFromLocation->Proto == PROTO_TCP))
	{
		if (equalsIgnoreCase(aFromLocation->Host, addr2String(sender)) && (aFromLocation->Port == senderPort)) 
		{
			// cyclic response
			return result;
		}
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
						SipAddress fr(proto, m.Headers["T"], now);
						// update location
						fr.Domain = mRegistry->Domain;
						SipLocation *l = fr.getLocation(now);
						l->Host = addr2String(&m.Address);
						l->Port = port;
						l->Expire = m.getExpires();
						// mRegistry->put(fr);
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
	// Sometimes SIP client provide external contact- like 5b..@192.168.1.33:5060
	std::string contact = retranslate.Headers["M"];
	if (&contact != NULL) 
	{
		SipAddress aContact(PROTO_UNKN, contact);
		if (mRegistry->getById(aContact.Id, addr2String(svcsocket), aContact))
			retranslate.Headers["M"] = aContact.toContact();
	}
	*/

	retranslate.Proto = aFromLocation->Proto;
	retranslate.Key = keyfrom;
	if (string2addr(aFromLocation->Host.c_str(), &retranslate.Address))
		retranslate.setPort(aFromLocation->Port);
	else
		// incorrect address
		return result;

	if (!(addrEquils(&retranslate.Address, svcsocket) && (retranslate.Address.sin_port == 5060)))
	{
		// check recursive
		retranslate.addHeaderParameter("V", "received", aFromLocation->Host);
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
	SipAddress a(PROTO_UNKN, headers["F"], 0);
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
bool SipDialog::authenticate(const std::string &method, const AuthParams &authparams)
{
	// check is there Authorization header, check user
	std::string user = authparams.Username;
	if (user.empty())
		return false;
	// check does user exists
	SipAddress *address = mRegistry->getById(authparams.Username, authparams.Domain);
	if (address == NULL)
		return false;
	// check password for "owner" user over UDP or of all over web socket connection 
	if (!mAuthAcct->isAuthorized(authparams, method, address->Password, ""))
		return false;
	return true;
}

/*
	Check is message issued by user 
*/
bool SipDialog::isByUser(const std::string &id, const std::string &domain, SipMessage &m)
{
	SipAddress *a = mRegistry->getByAddress(m.Headers["F"]);
	return (a != NULL) && (a->Id == id) && (a->Domain == domain);
}


