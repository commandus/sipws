#ifndef AUTHHEADERMD5_H
#define AUTHHEADERMD5		1

#include <string>
#include <map>
#include "sipwsUtil.h"
#include "MessageDigest.h"

const static std::string WWWAuthenticate = "WWW-Authenticate";
const static std::string Authorization = "Authorization";
// const static std::string AuthorizationLowecase = "authorization";
const static std::string Digest = "Digest";
const static std::string MD5Sess = "MD5-sess";

/**
* RFC 2617, Http Authentication using MD5 algorithm.
* <br/>
* 1. create a = AuthHeaderMD5(domain, options...) for SIP or WWW domain.
* SIP domain: sip:xx.xx.xx.xx:5060, WWW domain: http://sip:xx.xx.xx.xx:8888;
* <br/>
* 2. get WWW-Authenticate header a.getAuthenticate(uri, realm)
* where realm is user identifier, e.g. 102 for sip:102@:xx.xx.xx.xx;
* <br/>
* Check Authorization header:<br/>
* 1. create another b = AuthHeaderMD5(extracted Authorization header value);<br/>
* 2. Get user name get b.getUsername(), read password from the server storage;<br/>
* 3. check isAuthorized(std::string method, std::string user, std::string password, std::string body);<br/>
* 4. Optionally compare a.getDomain().equals(b..., getUri() and method (GET, POST for WWW or REFGISTER for SIP)
*/
class AuthParams
{
private:
	/**
	* Parse Authorization header, Digest MD5 only!
	* @param header header like Authorization: Digest username="Alice", realm="atlanta.com", nonce="84a4cc6f3082121f32b42a2187831a9e", response="7587245234b3434cc3412213e5f113a5432"
	* @return
	*/
	static bool parseAuthorizationHeader(const std::string &header, AuthParams *retval);
	/**
	* Parse single authentication scheme parameter.
	* @param name parameter name
	* @param value parameter value as std::string
	*/
	static void parseParam(const std::string &name, const std::string &value, AuthParams &retval);

public:
	std::string Nonce;			// base64 server-generated unique std::string
	std::string Opaque;			// base64 server-generated unique std::string, client must return opaque as-is  
	bool Qop;					// true- auth-int authentication integrity, false- authentication only 	
	bool isMD5Sess;				// false- algorithm MD5, true- MD5-sess

	std::string Domain;			// space separated list of WWW or SIP domain , e.g. sip:host.com for client information only
	std::string Realm;			// where we are, e.g. user@gotham.com
	std::string Uri;			// query

	// Authorization only (sent by client)
	std::string Username;
	std::string ClientNonce;	// "cnonce" - unique client nonce
	std::string Response;		// 32 chars
	int NonceCount;				// "nc" count of use nonce

	AuthParams();
	AuthParams(const std::string &domain, std::map<std::string, std::string> &headers);
	~AuthParams();
	void reset();
};

class AuthHeaderMD5
{
private:
	MessageDigest mDigest;
	std::string mDomain;	// space separated list of WWW or SIP domain , e.g. sip:host.com for client information only
	/**
	* Return
	* 		- Digest realm="atlanta.com", domain="sip:boxesbybob.com", qop="auth", nonce="f84f1cec41e6cbe5aea9c8e88d359", opaque="", stale=false, algorithm=MD5
	* 		or
	* 		- Digest username="Alice", realm="atlanta.com", nonce="84a4cc6f3082121f32b42a2187831a9e", response="7587245234b3434cc3412213e5f113a5432"
	*/
	std::string getValue(const AuthParams &value, bool isAuthenticate);
	std::string H(const std::string &data);
	std::string H(const char* data, size_t size);
	std::string KD(const std::string &secret, const std::string &data);
	
	/**
	* Calculate the nonce based on current time-stamp upto the second, and a
	* random seed
	*/
	std::string newNonce();
protected:

	/**
	* Get credentials for user
	* @param domain
	* @param uri	requested resource Uri
	* @param realm	user identifier e.g. sip:102@192.168.1.20 or 102
	* Return
	* 		WWW-Authenticate: Digest realm="atlanta.com", domain="sip:boxesbybob.com", qop="auth", nonce="f84f1cec41e6cbe5aea9c8e88d359", opaque="", stale=false, algorithm=MD5
	*/
	std::string getAuthenticate(const AuthParams &value);

	/**
	* Return
	* 		Authorization: Digest username="Alice", realm="atlanta.com", nonce="84a4cc6f3082121f32b42a2187831a9e", response="7587245234b3434cc3412213e5f113a5432"
	*/
	std::string getAuthorization(const AuthParams &value);
	std::string getAuthorizationResponse(const std::string &method, const std::string &password, const std::string &body, const AuthParams &value);

public:
	/**
	* @param domain
	* @param realm
	, const std::string &realm
	*/
	AuthHeaderMD5(const std::string &domain);
	~AuthHeaderMD5();
	std::string getDomain();

	/**
	* Get WWW-Authenticate for user
	* like WWW-Authenticate: Digest realm="atlanta.com", domain="sip:boxesbybob.com", qop="auth", nonce="f84f1cec41e6cbe5aea9c8e88d359", opaque="", stale=false, algorithm=MD5
	* @param domain
	* @param uri	requested resource Uri
	* @param realm	user identifier e.g. sip:102@192.168.1.20 or 102
	*/
	std::string getAuthenticate(const std::string &username, const std::string &uri, const std::string &realm);

	/**
	* Check Authorization header
	* @param authorizationHeaderValue
	* @param method
	* @param user
	* @param password
	* @param body
	* @return
	*/
	bool isAuthorized(const std::string &method, const std::string &username, const std::string &password, 
		const std::string &uri, const std::string &realm, 
		const std::string &nonce, const std::string &clientnonce, size_t noncecount,
		const std::string &body,
		const std::string &response);
	bool isAuthorized(const AuthParams &params, const std::string &method, const std::string &password, const std::string &body);
};

#endif