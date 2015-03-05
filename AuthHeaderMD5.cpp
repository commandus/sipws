#include "AuthHeaderMD5.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm> 
#include <functional> 
#include "MessageDigest.h"

AuthParams::AuthParams():
	Nonce(""),
	Opaque(""),
	Qop(false),
	isMD5Sess(false),
	Realm(""),
	Uri(""),
	Username(""),
	ClientNonce(""),
	Response(""),
	NonceCount(0)
{
}

AuthParams::~AuthParams()
{
}

AuthParams::AuthParams(const std::string &domain, std::map<std::string, std::string> &headers)
{
	reset();
	std::map<std::string, std::string>::const_iterator it = headers.find(Authorization);
	if (it == headers.end()) 
		return;
	parseAuthorizationHeader(it->second, this);
	if (Domain.empty())
		Domain = domain;
}

/**
* Parse Authorization header, Digest MD5 only!
* @param header header like Authorization: Digest username="Alice", realm="atlanta.com", nonce="84a4cc6f3082121f32b42a2187831a9e", response="7587245234b3434cc3412213e5f113a5432"
* @return
*/
bool AuthParams::parseAuthorizationHeader(const std::string &header, AuthParams *retval)
{
	// reset();
	if ((&header == NULL) || header.empty())
		return false;
	int pos = header.find(' ');
	if (pos < 0)
		return false;


	std::string scheme = trim(header.substr(0, pos));
	if (!equalsIgnoreCase(scheme, Digest))
		return false;
	std::string parameters = header.substr(pos + 1);
	int start = 0;
	bool cont = true;
	do {
		pos = parameters.find(',', start);

		if (pos < 0) {
			pos = parameters.length();
			cont = false;
		}

		int eqPos = parameters.find('=', start);
		if (eqPos >= 0)
			parseParam(trim(parameters.substr(start, eqPos - start)),
			trimDoubleQuotes(trim(parameters.substr(eqPos + 1, pos - eqPos - 1))), *retval);
		start = pos + 1;
	} while (cont);
	return (!retval->Username.empty()) && (!retval->Response.empty());
}

/**
* Parse single authentication scheme parameter.
* @param name parameter name
* @param value parameter value as std::string
*/
void AuthParams::parseParam(const std::string &name, const std::string &value, AuthParams &retval)
{
	if (equalsIgnoreCase(name, "nonce"))
		retval.Nonce = value;
	else
		if (equalsIgnoreCase(name, "opaque"))
			retval.Opaque = value;
		else
			if (equalsIgnoreCase(name, "qop"))
				retval.Qop = contains(toLower(value), "auth-int");
			else
				if (equalsIgnoreCase(name, "algorithm"))
					retval.isMD5Sess = equalsIgnoreCase(value, MD5Sess);
				else
					if (equalsIgnoreCase(name, "realm"))
						retval.Realm = value;
					else
						if (equalsIgnoreCase(name, "username"))
							retval.Username = value;
						else
							if (equalsIgnoreCase(name, "domain"))
								retval.Domain = value;
							else
								if (equalsIgnoreCase(name, "nc"))
								{
									try
									{
										retval.NonceCount = parseHex(value);
									}
									catch (...)
									{
										retval.NonceCount = 0;
									}
								}
								else
									if (equalsIgnoreCase(name, "cnonce"))
										retval.ClientNonce = value;
									else
										if (equalsIgnoreCase(name, "uri"))
											retval.Uri = value;
										else
											if (equalsIgnoreCase(name, "response"))
												retval.Response = value;
}

void AuthParams::reset()
{
	Qop = false;
	isMD5Sess = false;

	Realm = "";
	Username = "";

	NonceCount = 0;
	ClientNonce = "";
	Uri = "";

	Nonce = "";
	Opaque = "";
	Response = "";
}

AuthHeaderMD5::~AuthHeaderMD5()
{
}

/**
	username
	uri
	realm
	nonce
	opaque
* Return
*		
* 		- Digest realm="atlanta.com", domain="sip:boxesbybob.com", qop="auth", nonce="f84f1cec41e6cbe5aea9c8e88d359", opaque="", stale=false, algorithm=MD5
* 		or
* 		- Digest username="Alice", realm="atlanta.com", nonce="84a4cc6f3082121f32b42a2187831a9e", response="7587245234b3434cc3412213e5f113a5432"
*/
std::string AuthHeaderMD5::getValue(const AuthParams &value, bool isAuthenticate)
{
	std::string opaque = H(value.Domain + value.Nonce);
	std::ostringstream b;
	b << "Digest ";
	if (!value.Domain.empty())
		b << "domain=\"" << value.Domain << "\", ";
	if (!value.Uri.empty())
		b << "uri=\"" << value.Uri << "\", ";
	if (!value.Realm.empty())
		b << "realm=\"" << value.Realm << "\", ";
	if (!value.Nonce.empty())
		b << "nonce=\"" + value.Nonce << "\", ";
	if (!value.Opaque.empty())
		b << "opaque=\"" << value.Opaque << "\", ";
	if (value.Qop)
		b << "qop=auth-int, ";
	b << "algorithm=" << (value.isMD5Sess ? MD5Sess : "MD5") << ", ";

	if (isAuthenticate) {
		if (!value.Username.empty())
			b << "username=\"" << value.Username << "\", ";
		if (!value.Response.empty())
			b << "response=\"" << value.Response << "\", ";
		if (value.NonceCount != 0)
			b << "nc=" + size_t2Hex(value.NonceCount, 8) + ", ";
		if (!value.ClientNonce.empty())
			b << "cnonce=\"" << value.ClientNonce << "\", ";
	}
	std::string r(b.str());
	int len = r.length();
	if (len > 0) {
		r.erase(len - 2, 2); // erase the last 2 characters '",' 
	}
	return r;
}

std::string AuthHeaderMD5::H(const std::string &data)
{
	return H(data.data(), data.length());
}

std::string AuthHeaderMD5::H(const char* data, size_t size)
{
	mDigest.reset();
	return bytes2Hex(mDigest.digest(data, size), 16);
}

std::string AuthHeaderMD5::KD(const std::string &secret, const std::string &data)
{
	return H(secret + ":" + data);
}

/**
* Calculate the nonce based on current time-stamp upto the second, and a
* random seed
*
* @return
*/
std::string AuthHeaderMD5::newNonce() 
{
	time_t t;
	time(&t);
	std::string fmtDate = fmtTime(t, "%Y:%m:%d:%H:%M:%S");
	int randomInt = rand() % 100000;
	std::string s = fmtDate + int2Dec(randomInt);
	return H(s);
}

std::string AuthHeaderMD5::getDomain() 
{
	return mDomain;
}

/**
* Authenticate: use constructor to produce WWW-Authenticate
* ah = new AuthHeaderMD5(options,...); header = ah.getAuthenticate(uri, realm)
* @param domain
* @param realm
*/
AuthHeaderMD5::AuthHeaderMD5(const std::string &domain) : mDomain(domain)
{
}

/**
* Get credentials for user
* @param domain
* @param uri	requested resource Uri
* @param realm	user identifier e.g. sip:102@192.168.1.20 or 102
*/
std::string AuthHeaderMD5::getAuthenticate(const std::string &username, const std::string &uri, const std::string &realm)
{
	AuthParams params;
	params.Username = username;
	params.Uri = uri;
	params.Realm = realm;
	// get a new nonce
	params.Nonce = newNonce();
	params.Opaque = H(mDomain + params.Nonce);
	// produce WWW-Authenticate header to send user in 401 response
	return getAuthenticate(params);
}

/**
* Check Authorization header
* @param authorizationHeaderValue
* @param method
* @param user
* @param password
* @param body
* @return
*/
bool AuthHeaderMD5::isAuthorized(
	const std::string &method, const std::string &username, const std::string &password, 
	const std::string &uri, const std::string &realm, 
	const std::string &nonce, const std::string &clientnonce, size_t noncecount,
	const std::string &body,
	const std::string &response)
{
	// calculate hash
	AuthParams params;
	params.Username = username;
	params.Uri = uri;
	params.Realm = realm;
	params.Response = response;
	std::string resp = getAuthorizationResponse(method, password, body, params);
	return isAuthorized(params, method, password, body);
}

/**
* Check Authorization header
* @param authorizationHeaderValue
* @param method
* @param user
* @param password
* @param body
* @return
*/
bool AuthHeaderMD5::isAuthorized(const AuthParams &params, const std::string &method, const std::string &password, const std::string &body)
{
	std::string resp = getAuthorizationResponse(method, password, body, params);
	return resp == params.Response; // && (user == mUsername)); 
}

std::string AuthHeaderMD5::getAuthorizationResponse(const std::string &method, const std::string &password, const std::string &body, const AuthParams &value)
{
	std::string HA1;

	if (value.isMD5Sess)
		HA1 = H(H((&value.Username == NULL ? "" : value.Username) + ":" + value.Realm + ":" + (&password == NULL ? "" : password)) + ":" + value.Nonce + ":" + value.ClientNonce);
	else
		HA1 = H((&value.Username == NULL ? "" : value.Username) + ":" + value.Realm + ":" + (&password == NULL ? "" : password));
	
	std::string HA2;
	if (value.Qop) {
		HA2 = H(((&method == NULL) ? "" : method) + ":" + (&value.Uri == NULL ? "" : value.Uri) + ":" + H((&body == NULL ? "" : body)));
	}
	else {
		HA2 = H(((&method == NULL) ? "" : method) + ":" + (&value.Uri == NULL ? "" : value.Uri));
	}

	std::string r;
	if (!value.Qop) {
		r = KD(HA1, value.Nonce + ":" + HA2);
	}
	else {
		r = KD(HA1,
			value.Nonce + ":"
			+ size_t2Hex(value.NonceCount, 8) + ":"
			+ value.ClientNonce + ":"
			+ (value.Qop ? "auth-int" : "auth") + ":"
			+ HA2);
	}
	return r;
}

/**
* Return
* 		WWW-Authenticate: Digest realm="atlanta.com", domain="sip:boxesbybob.com", qop="auth", nonce="f84f1cec41e6cbe5aea9c8e88d359", opaque="", stale=false, algorithm=MD5
*/
std::string AuthHeaderMD5::getAuthenticate(const AuthParams &value)
{
	std::ostringstream b;
	b << getValue(value, true);
	return b.str();
}

/**
* Return
* 		Authorization: Digest username="Alice", realm="atlanta.com", nonce="84a4cc6f3082121f32b42a2187831a9e", response="7587245234b3434cc3412213e5f113a5432"
*/
std::string AuthHeaderMD5::getAuthorization(const AuthParams &value)
{
	std::ostringstream b;
	b << Authorization << getValue(value, false);
	return b.str();
}



