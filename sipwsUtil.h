#ifndef SIPWSUTIL_H
#define SIPWSUTIL_H	1

#include <string>
#include <vector>

#ifdef WIN32
#include <winsock2.h>
#include "ws2ipdef.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR   -1
#endif

#define		ERR_KEY		1

#define DEF_EXPIRES	3600

#define	CR	'\r'
#define	LF	'\n'

const std::string CRLF("\r\n");
const std::string ALLOWED_METHODS = "REGISTER, INVITE, ACK, BYE, OPTIONS, CANCEL, MESSAGE";
// Request Constants

const std::string ACK = "ACK";
const std::string BYE = "BYE";
const std::string CANCEL = "CANCEL";
const std::string INVITE = "INVITE";
const std::string OPTIONS = "OPTIONS";
const std::string REGISTER = "REGISTER";
const std::string NOTIFY = "NOTIFY";
const std::string SUBSCRIBE = "SUBSCRIBE";
const std::string MESSAGE = "MESSAGE";
const std::string REFER = "REFER";
const std::string INFO = "INFO";
const std::string PRACK = "PRACK";
const std::string UPDATE = "UPDATE";
const std::string PUBLISH = "PUBLISH";

const std::string PING = "jaK";

const int C_INVALID = -1;
const int C_RESPONSE = 0;

const int C_ACK = 1;
const int C_BYE = 2;
const int C_CANCEL = 3;
const int C_INVITE = 4;
const int C_OPTIONS = 5;
const int C_REGISTER = 6;
const int C_NOTIFY = 7;
const int C_SUBSCRIBE = 8;
const int C_MESSAGE = 9;
const int C_REFER = 10;
const int C_INFO = 11;
const int C_PRACK = 12;
const int C_UPDATE = 13;
const int C_PUBLISH = 14;

// Response status codes
const int TRYING = 100;
const int DIALOG_ESTABLISHEMENT = 101;
const int RINGING = 180;
const int CALL_IS_BEING_FORWARDED = 181;
const int QUEUED = 182;
const int SESSION_PROGRESS = 183;
const int OK = 200;
const int ACCEPTED = 202;
const int MULTIPLE_CHOICES = 300;
const int MOVED_PERMANENTLY = 301;
const int MOVED_TEMPORARILY = 302;
const int USE_PROXY = 305;
const int ALTERNATIVE_SERVICE = 380;
const int BAD_REQUEST = 400;
const int UNAUTHORIZED = 401;
const int PAYMENT_REQUIRED = 402;
const int FORBIDDEN = 403;
const int NOT_FOUND = 404;
const int METHOD_NOT_ALLOWED = 405;
const int NOT_ACCEPTABLE = 406;
const int PROXY_AUTHENTICATION_REQUIRED = 407;
const int REQUEST_TIMEOUT = 408;
const int GONE = 410;
const int CONDITIONAL_REQUEST_FAILED = 412;
const int REQUEST_ENTITY_TOO_LARGE = 413;
const int REQUEST_URI_TOO_LONG = 414;
const int UNSUPPORTED_MEDIA_TYPE = 415;
const int UNSUPPORTED_URI_SCHEME = 416;
const int BAD_EXTENSION = 420;
const int EXTENSION_REQUIRED = 421;
const int INTERVAL_TOO_BRIEF = 423;
const int TEMPORARILY_UNAVAILABLE = 480;
const int CALL_OR_TRANSACTION_DOES_NOT_EXIST = 481;
const int LOOP_DETECTED = 482;
const int TOO_MANY_HOPS = 483;
const int ADDRESS_INCOMPLETE = 484;
const int AMBIGUOUS = 485;
const int BUSY_HERE = 486;
const int REQUEST_TERMINATED = 487;
const int NOT_ACCEPTABLE_HERE = 488;
const int BAD_EVENT = 489;
const int REQUEST_PENDING = 491;
const int UNDECIPHERABLE = 493;
const int SERVER_INTERNAL_ERROR = 500;
const int NOT_IMPLEMENTED = 501;
const int BAD_GATEWAY = 502;
const int SERVICE_UNAVAILABLE = 503;
const int SERVER_TIMEOUT = 504;
const int VERSION_NOT_SUPPORTED = 505;
const int MESSAGE_TOO_LARGE = 513;
const int BUSY_EVERYWHERE = 600;
const int DECLINE = 603;

/**
* The server has authoritative information that the user indicated in the
* Request-URI does not exist anywhere.
*/
const int DOES_NOT_EXIST_ANYWHERE = 604;
const int SESSION_NOT_ACCEPTABLE = 606;

enum ECommand { commandput = 0, commandlist = 1, commandstart = 2, commandstop = 3, commandclear = 4, commandnop = 5 };
enum EOperation { entryadd = 0, entryremove = 1, entrynop = 3 };

enum TAvailability { AVAIL_NO = 0, AVAIL_YES = 1 };
#define PROTO_SIZE	4
enum TProto { PROTO_UNKN = 0, PROTO_UDP = 1, PROTO_TCP = 2, PROTO_WS = 3 };

#define PREFIX_SIZE	5
enum TPrefix { PREFIX_UNKN = 0, PREFIX_SIP = 1, PREFIX_SIPS = 2, PREFIX_WS = 3, PREFIX_WSS = 4 };
enum TOrigin { ORIGIN_REGISTRY, ORIGIN_NETWORK };

std::string int2Dec(int value);
std::string size_t2Dec(size_t value);
std::string size_t2Hex(size_t value, int width);
std::string time_t2Dec(time_t value);
std::string toString(ECommand value);
std::string toString(EOperation value);
std::string toString(TAvailability value);
std::string toString(TProto value);
std::string toString(TPrefix value);
std::string toString(TOrigin value);
std::string bytes2Hex(const unsigned char *bytes, size_t size);

/*
ANSI compare ignore case
*/
bool equalsIgnoreCase(const std::string &a, const std::string &b);

size_t parseSize_t(const std::string &value);
int parseInt(const std::string &value);
int parseInt(const std::string &value, int defval);
size_t parseHex(const std::string &value);
ECommand parseCommand(const std::string &value);
EOperation parseOperation(const std::string &value);
TAvailability parseAvailability(const std::string &value);
TProto parseProto(const std::string &value);
TPrefix parsePrefix(const std::string &value);
TOrigin parseOrigin(const std::string &value);
time_t parseTime_t(const std::string &value);
std::string fmtTime(const time_t value, const std::string &fmt, bool useglobaltime = false);
std::string getCommandName(int command);
int getCommand(const std::string &command);
std::string getReasonPhrase(int rc);

std::vector<std::string> split(char* str, const char* delim, std::vector<std::string> &result);
std::vector<std::string> split(const std::string &str, const std::string &delim, std::vector<std::string> &result);

// trim from start
std::string ltrim(const std::string &value);

// trim from end
std::string rtrim(const std::string &value);

// trim from both ends
std::string trim(const std::string &value);

// trim " if exists
std::string trimDoubleQuotes(const std::string &value);
// tolower
std::string toLower(const std::string &value);
// toupper
std::string toUpper(const std::string &value);
// contains
bool contains(const std::string &str, const std::string &value);

/*
startsWith("012345", "45")
*/
bool startsWith(const std::string &str, const std::string &value);

/*
endsWith("012345", "45")
6 - 2 = 4
*/
bool endsWith(const std::string &str, const std::string &value);

std::string replaceChar(const std::string &value, char cfrom, char cto);

// IP address routines 

// get sockaddr, IPv4 or IPv6:
std::string addr2String(struct sockaddr_in *addr);
/*
Resolve host name or IPv4 address string to IPv4 address.
Return true, if at least one address is resolved
Return false, if host is unknown
*/
bool char2addrIP4(const char *hostname, struct sockaddr_in *result);
bool string2addrIP4(const std::string &hostname, struct sockaddr_in *result);

/*
Resolve host name or IPv4 address string to IPv4 address.
Return true, if at least one address is resolved
Return false, if host is unknown
*/
bool string2addr(const char *hostname, struct sockaddr_in *result);
/*
TODO IPv4 only
*/
bool addrEquils(struct sockaddr_in *a, struct sockaddr_in *b);

// Return "" if file does not exists
std::string readFile(const std::string &filename);
int writeFile(const std::string &filename, const std::string &data);
int appendFile(const std::string &filename, const std::string &data);

std::string getSystemErrorDescription(int errorcode);

#endif

