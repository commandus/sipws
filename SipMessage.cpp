#include "SipMessage.h"

#include <cctype>
#include <iostream>
#include <sstream>
#include <vector>
#include <string.h>

const static std::map<std::string, std::string> HeaderShort {
	{ "Ñ", "Content-Type" },
	{ "E", "Content-Encoding" },
	{ "F", "From" },
	{ "I", "Call-ID" },
	{ "K", "Supported" },
	{ "L", "Content-Length" },
	{ "M", "Contact" },
	{ "S", "Subject" },
	{ "O", "Event" },
	{ "R", "Refer-To" },
	{ "T", "To" },
	{ "U", "Allow-Events" },
	{ "V", "Via" }
};

const static std::map<std::string, std::string> HeaderFull {
	{ "Content-Type", "Ñ" },
	{ "Content-Encoding", "E" },
	{ "From", "F" },
	{ "Call-ID", "I" },
	{ "Supported", "K" },
	{ "Content-Length", "L" },
	{ "Contact", "M" },
	{ "Subject", "S" },
	{ "Event", "O" },
	{ "Refer-To", "R" },
	{ "To", "T" },
	{ "Allow-Events", "U" },
	{ "Via", "V" }
};

SipMessage::SipMessage() : mCommand(C_INVALID), mCode(0), mCommandParam(""), Proto(PROTO_UNKN), Key(""), KeyFrom(""), sentCode(0), conn(NULL)
{
	memset(&Address, 0, sizeof(struct sockaddr_in));
}

SipMessage::SipMessage(TProto proto, struct sockaddr_in *svcaddr, struct sockaddr_in *sender, const SipMessage &request)
	: Proto(proto), Address(*sender), mCommand(C_RESPONSE), mCode(OK), sentCode(0), conn(NULL)
{
	if (request.mCommand == C_INVALID)
		return;
	if (&request.Headers != NULL)
		Headers = request.Headers;
	if (Headers.find("L") == Headers.end())
		Headers["L"] = "0";	// Content-Length
	Key = request.Key;
	KeyFrom = request.KeyFrom;
	Headers["User-Agent"] ="sipws";
	Headers["Allow"] = ALLOWED_METHODS;
}

SipMessage::SipMessage(const SipMessage &m) 
{
	if (&m.Headers != NULL)
		Headers = m.Headers;
	Sdp = m.Sdp;
	mCommand = m.mCommand;
	mCode = m.mCode;
	mCommandParam = m.mCommandParam;
	Proto = m.Proto;
	Address = m.Address;
	Key = m.Key;
	KeyFrom = m.KeyFrom;
	sentCode = m.sentCode;
	conn = m.conn;
}

SipMessage::~SipMessage()
{
}

void SipMessage::setPort(int value)
{
	Address.sin_port = value;
}

/*
	Add parameter to the header
*/
void SipMessage::addHeaderParameter(const std::string &header, const std::string &parameter, const std::string &value) {
	std::string v = Headers[header];
	if (&v == NULL)
		v = "";
	else
		v = v + ";";
	v += parameter + "=" + value;
	Headers[header] = v;
}

bool SipMessage::posHeaderParameter(const std::string &header, const std::string &parameter, std::pair<int, int> &ret)
{
	std::string v = Headers[header];
	if (&v == NULL)
		return false;
	int start = 0;
	int p;
	while (true) {
		p = v.find(parameter, start);
		if (p < 0)
			break;
		if ((p > 0) && (std::isalpha(v.at(p - 1))))
		{
			start = p + 1;
			continue;
		}
		if (((p + parameter.length()) < v.length()) && (std::isalpha(v.at(p + parameter.length())))) {
			start = p + 1;
			continue;
		}
		break;
	}

	if (p >= 0) {
		int last = v.length();
		for (int i = p + parameter.length(); i < v.length(); i++) {
			char c = v.at(i);
			if (c == ';') {
				last = i + 1;
				break;
			}
		}
		ret.first = p;
		ret.second = last;
		return true;
	}
	return false;
}

void SipMessage::rmHeader(const std::string &header)
{
	std::map<std::string, std::string>::iterator it = Headers.find(header);
	if (it == Headers.end())
		return;
	Headers.erase(it);
}

void SipMessage::rmHeaderParameter(const std::string &header, const std::string &parameter) {
	std::string v = Headers[header];
	std::pair<int, int> p;
	if (!posHeaderParameter(header, parameter, p))
		return;
	Headers[header] = v.substr(0, p.first).append(v.substr(p.second));
}

std::string SipMessage::getHeaderParameter(const std::string &header, const std::string &parameter) {
	std::string v = Headers[header];
	std::pair<int, int> p;
	if (!posHeaderParameter(header, parameter, p))
		return "";
	int posval = p.first + parameter.length() + 1;
	if (posval >= p.second)
		return "";
	std::string r = v.substr(posval, p.second);
	/*
	if (r.at(r.length - 1) == ';')
		r = r.substr(0, r.length() - 1);
	*/
	return r;
}

bool SipMessage::existHeaderParameter(const std::string &header, const std::string &parameter) {
	std::pair<int, int> p;
	return posHeaderParameter(header, parameter, p);
}

void SipMessage::replaceHeaderParameter(const std::string &header, const std::string &parameter, const std::string &value) {
	std::string v = Headers[header];
	std::pair<int, int> p;
	if (!posHeaderParameter(header, parameter, p))
		Headers[header] = v + ";" + value;
	else {
		std::string sfx = v.substr(p.second);
		Headers[header] = v.substr(0, p.first) + value + 
			(((sfx.length() > 0) && (sfx.at(0) != ';')) ? ";" : "") + sfx ;
	}
}

/*
	Adds an extra CRLF
*/
std::string SipMessage::getHeaders() {
	std::string r = "";
	if (&Headers == NULL)
		return r;

	for (std::map<std::string, std::string>::iterator it = Headers.begin(); it != Headers.end(); ++it) {
		r += it->first + ": " + it->second + CRLF;
	}

	// delete last CRLF
	if (r.length() >= 2) 
	{
		r = r.substr(0, r.length() - 2);
	}

	return r;
}

std::string SipMessage::toString() 
{
	std::stringstream r;
	switch (mCommand) {
	case C_INVALID:
		r << "";
		break;
	case C_RESPONSE:
		r << "SIP/2.0 " << mCode << " "
			<< getReasonPhrase(mCode) << CRLF
			<< getHeaders() << CRLF << (&Sdp == NULL ? "" : Sdp);
		break;
	default:
		r << getCommandName(mCommand)
			<< " " << mCommandParam << " SIP/2.0\r\n"
			<< getHeaders() << CRLF << (&Sdp == NULL ? "" : Sdp);
		break;
	}
	return r.str();
}

std::string SipMessage::toString(bool fullheader)
{
	std::stringstream r;
	switch (mCommand) {
	case C_INVALID:
		r << "";
		break;
	case C_RESPONSE:
		r << "SIP/2.0 " << mCode << " "
			<< getReasonPhrase(mCode) << CRLF
			<< getHeaders(this, fullheader) << CRLF << (&Sdp == NULL ? "" : Sdp);
		break;
	default:
		r << getCommandName(mCommand)
			<< " " << mCommandParam << " SIP/2.0\r\n"
			<< getHeaders(this, fullheader) << CRLF << (&Sdp == NULL ? "" : Sdp);
		break;
	}
	return r.str();
}

/**
* Return "Expires" value
* @return expiration period in seconds
*/
int SipMessage::getExpires() 
{
	return parseInt(Headers["Expires"], DEF_EXPIRES);
}

/**
* Return true if "Expires" equals 0
* @return
*/
bool SipMessage::isExpired() 
{
	int expires = -1;
	try {
		expires = parseInt(Headers["Expires"]);
	}
	catch (...) {
	}
	return expires == 0;
}

/**
* Return true if it is message packet for client at specified IP address
* @param mIP Internet address
* @return true if it is message packet for client at specified IP address
*/
bool SipMessage::isMessageFor(struct sockaddr_in &from) 
{
	return ((mCommand == C_MESSAGE) && (from.sin_family == Address.sin_family)
		&& (memcmp(&from.sin_addr, &Address.sin_addr, sizeof(from.sin_addr))));
}

/**
* Return Max-Forwards value
* @return Max-Forwards value
*/
int SipMessage::getMaxForwards() 
{
	return parseInt(Headers["Max-Forwards"], 1);
}

/**
* Set Max-Forwards header
* @param value Max-Forwards value
*/
void SipMessage::setMaxForwards(int value) {
	Headers["Max-Forwards"] = int2Dec(value);
}

/**
* Extract port number from "V" (Via)
* V: SIP/2.0/UDP 192.168.1.37:32867;branch=z9hG4bK8b5cb1ad0affaa4ee99ac5f70c0e3647333632;rport=32867;received=192.168.1.37
* @param defaultValue
* @return port number
*/
int SipMessage::getViaPort(int defaultValue) {
	int r = defaultValue;
	std::string v = Headers["V"];
	if (&v == NULL)
		return r;
	int p;
	p = v.find(' ', 0);
	if (p == std::string::npos)
		return r;
	p = v.find(':', p + 1);
	if (p == -1)
		return r;
	int start = p + 1;
	p = start;
	while (p < v.length()) {
		if (!std::isdigit(v.at(p)))
			break;
		p++;
	}
	try {
		r = parseInt(v.substr(start, p));
	}
	catch (...) {
	}
	return r;
}

/*
	Adds an extra CRLF
*/
std::string SipMessage::getHeaders(SipMessage *m, bool fullheader)
{
	std::stringstream r;
	if ((m == NULL) || (&m->Headers == NULL))
		return r.str();
	
	bool need2replace;
	std::map<std::string, std::string>::const_iterator it = m->Headers.begin();
	while (it != m->Headers.end())
	{
		std::string newkey;
		if (fullheader)
		{
			std::map<std::string, std::string>::const_iterator its = HeaderShort.find(it->first);
			if (its != HeaderShort.end())
			{
				newkey = its->second;
			}
			else
				newkey = it->first;
		}
		else
		{
			std::map<std::string, std::string>::const_iterator itf = HeaderFull.find(it->first);
			if (itf != HeaderFull.end())
			{
				newkey = itf->second;
			}
			else
				newkey = it->first;
		}
			
		r << newkey << ":" << it->second << CRLF;
		++it;
	}
	return r.str();
}

bool SipMessage::parse(const std::string message, SipMessage &result)
{
	std::vector<std::string> lines;
	split(message, CRLF.c_str(), lines);
	std::vector<std::string> headers;
	int mBodyStart;
	int mHeaderCount;

	// find out header end
	mBodyStart = 0;
	for (int i = 0; i < lines.size(); i++) {
		if (lines[i].length() == 0)
			if (mBodyStart == 0) {
				mBodyStart = i + 1;
				break;
			}
	}
	if (mBodyStart == 0)
		mBodyStart = lines.size() + 1;

	result.mCommand = C_INVALID;
	if (mBodyStart <= 0)
		return false;

	// concatenate headers if header start with whitespace or \t
	std::string line = "";
	for (int i = 0; i < mBodyStart - 1; i++) {
		char c = lines[i].at(0);
		if ((c == ' ') || (c == '\t'))
			line += " " + trim(lines[i]);
		else
			headers.push_back(line + lines[i]);
	}
	if (line.length() > 0) // if last line is continued
		headers.push_back(line);
	mHeaderCount = headers.size();


	if (mHeaderCount > 0) {
		// SIP/2.0 481 Call leg/Transaction does not exist
		std::string *c = &headers.at(0);
		if (c->find("SIP/") == 0)
		{
			result.mCode = 0;
			try 
			{
				size_t p0 = c->find(' ', 4);
				size_t p1 = c->find(' ', p0 + 2);
				result.mCode = parseInt(c->substr(p0 + 1, p1 - p0 - 1));
			}
			catch (...) 
			{
			}
			result.mCommand = C_RESPONSE;
		}
		else {
			// ACK sip:10BF48186B5B@192.168.1.33 SIP/2.0
			size_t p0 = c->find(' ');
			size_t p1 = c->find(' ', p0 + 1);
			// size_t p2 = c->find(' ', p1 + 1);
			try
			{
				result.mCommand = getCommand(c->substr(0, p0));
				result.mCommandParam = c->substr(p0 + 1, p1 - p0 - 1);
			}
			catch (...) 
			{
			}
		}
	}
	if (result.mCommand != C_INVALID) {
		for (int i = 1; i < mHeaderCount; i++) {
			std::string s = headers.at(i);
			int p = s.find(':');
			if (p > 0) {
				// replace full header to the short 
				std::string h = s.substr(0, p);

				std::map<std::string, std::string>::const_iterator it = HeaderFull.find(h);

				if (it != HeaderFull.end())
					h = it->second;
				result.Headers[h] = trim(s.substr(p + 1));
			}
		}
		result.Sdp = "";
		for (int i = mBodyStart; i < lines.size(); i++) {
			result.Sdp += lines[i] + CRLF;
		}
	}
	return true;
}
