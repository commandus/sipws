#include "sipwsutil.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <clocale>
#include <algorithm> 
#include <functional> 
#ifdef WIN32
#include "strptime.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include<sys/socket.h>
#include<netdb.h>	//hostent
#include<arpa/inet.h>
#endif
#include<string.h> //memset
#include <fstream>
#include <streambuf>

static const char* cmdlist[] {"put", "list", "start", "stop", "clear", ""};
static const char* oplist[] { "+", "-", "" };
static const char* protolist[] { "", "udp", "tcp", "ws" };
static const char* prefixlist[] { "", "sip", "sips", "ws", "wss" };
static const char* originlist[] { "registry", "network" };

std::string int2Dec(int value)
{
	std::ostringstream os;
	os << value;
	return os.str();
}

std::string size_t2Dec(size_t value)
{
	std::ostringstream os;
	os << value;
	return os.str();
}

std::string time_t2Dec(time_t value)
{
	std::ostringstream os;
	os << value;
	return os.str();
}

std::string size_t2Hex(size_t value, int width)
{
	std::ostringstream os;
	os << std::hex << std::setw(width) << value;
	return os.str();
}

std::string toString(ECommand value)
{
	if ((value < commandput) || (value > commandnop))
		return "";
	return cmdlist[(int) value];
}

std::string bytes2Hex(const unsigned char *bytes, size_t size)
{
	std::ostringstream os;
	os.fill('0');
	os << std::hex;
	for (const unsigned char *ptr = bytes; ptr<bytes + size; ptr++)
		os << std::setw(2) << (unsigned int)*ptr;
	return os.str();
}

/*
	ANSI
*/
bool equalsIgnoreCase(const std::string &a, const std::string &b)
{
	unsigned int sz = a.size();
	if (b.size() != sz)
		return false;
	for (unsigned int i = 0; i < sz; ++i)
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	return true;
}

std::string trimDoubleQuotes(const std::string &value)
{
	std::string r(value);
	if (r.front() == '"')
	{
		r.erase(0, 1); // erase the first character
		r.erase(r.size() - 1); // erase the last character
	}
	return r;
}

std::string toString(EOperation value)
{
	if ((value < entryadd) || (value > entrynop))
		return "";
	return oplist[(int)value];
}

std::string toString(TProto value)
{
	if ((value < PROTO_UNKN) || (value > PROTO_WS))
		return "";
	return protolist[(int)value];
}

std::string toString(TPrefix value)
{
	if ((value < PREFIX_UNKN) || (value > PREFIX_WSS))
		return "";
	return prefixlist[(int)value];
}


std::string toString(TOrigin value)
{
	if ((value < ORIGIN_REGISTRY) || (value > ORIGIN_REGISTRY))
		return "";
	return originlist[(int)value];
}

ECommand parseCommand(const std::string &value)
{
	if (&value == NULL)
		return commandnop;
	if (value.compare(cmdlist[0]) == 0)
		return commandput;
	else
		if (value.compare(cmdlist[1]) == 0)
			return commandlist;
		else
			if (value.compare(cmdlist[2]) == 0)
				return commandstart;
			else
				if (value.compare(cmdlist[3]) == 0)
					return commandstop;
				else
					if (value.compare(cmdlist[4]) == 0)
						return commandclear;
	return commandnop;
}

EOperation parseOperation(const std::string &value)
{
	if (&value == NULL)
		return entrynop;
	if (value.compare(oplist[0]) == 0)
		return entryadd;
	else
		if (value.compare(oplist[1]) == 0)
			return entryremove;
	return entrynop;
}

TProto parseProto(const std::string &value)
{
	if (&value == NULL)
		return PROTO_UNKN;
	if (value.compare(protolist[1]) == 0)
		return PROTO_UDP;
	else
		if (value.compare(protolist[2]) == 0)
			return PROTO_TCP;
		else
			if (value.compare(protolist[3]) == 0)
				return PROTO_WS;
			else
					return PROTO_UNKN;
}

TPrefix parsePrefix(const std::string &value)
{
	if (&value == NULL)
		return PREFIX_UNKN;
	if (value.compare(prefixlist[1]) == 0)
		return PREFIX_SIP;
	else
		if (value.compare(prefixlist[2]) == 0)
			return PREFIX_SIPS;
		else
			if (value.compare(prefixlist[3]) == 0)
				return PREFIX_WS;
			else
				if (value.compare(prefixlist[4]) == 0)
					return PREFIX_WSS;
				else
					return PREFIX_UNKN;
}

TOrigin parseOrigin(const std::string &value)
{
	if (&value == NULL)
		return ORIGIN_NETWORK;
	if (value.compare(originlist[0]) == 0)
		return ORIGIN_REGISTRY;
	return ORIGIN_NETWORK;
}

size_t parseSize_t(const std::string &value)
{
	try
	{
		return std::stoul(value);
	}
	catch (...)
	{
	}
	return 0;
}

int parseInt(const std::string &value)
{
	return parseInt(value, 0);
}

int parseInt(const std::string &value, int defval)
{
	try
	{
		return std::stoi(value);
	}
	catch (...)
	{
	}
	return defval;
}


size_t parseHex(const std::string &value)
{
	size_t r;
	std::stringstream ss;
	ss << std::hex << value;
	ss >> r;
	return r;
}

time_t parseTime_t(const std::string &value)
{
	if (&value == NULL)
		return 0;
	struct tm t;
	char b[80];
	strftime(b, sizeof(b), "%Y-%m-%d %H:%M", &t);
	return mktime(&t);
}

/*
specifier	Replaced by	Example
%a	Abbreviated weekday name *	Thu
%A	Full weekday name *	Thursday
%b	Abbreviated month name *	Aug
%B	Full month name *	August
%c	Date and time representation *	Thu Aug 23 14:55:02 2001
%C	Year divided by 100 and truncated to integer (00-99)	20
%d	Day of the month, zero-padded (01-31)	23
%D	Short MM/DD/YY date, equivalent to %m/%d/%y	08/23/01
%e	Day of the month, space-padded ( 1-31)	23
%F	Short YYYY-MM-DD date, equivalent to %Y-%m-%d	2001-08-23
%g	Week-based year, last two digits (00-99)	01
%G	Week-based year	2001
%h	Abbreviated month name * (same as %b)	Aug
%H	Hour in 24h format (00-23)	14
%I	Hour in 12h format (01-12)	02
%j	Day of the year (001-366)	235
%m	Month as a decimal number (01-12)	08
%M	Minute (00-59)	55
%n	New-line character ('\n')
%p	AM or PM designation	PM
%r	12-hour clock time *	02:55:02 pm
%R	24-hour HH:MM time, equivalent to %H:%M	14:55
%S	Second (00-61)	02
%t	Horizontal-tab character ('\t')
%T	ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S	14:55:02
%u	ISO 8601 weekday as number with Monday as 1 (1-7)	4
%U	Week number with the first Sunday as the first day of week one (00-53)	33
%V	ISO 8601 week number (00-53)	34
%w	Weekday as a decimal number with Sunday as 0 (0-6)	4
%W	Week number with the first Monday as the first day of week one (00-53)	34
%x	Date representation *	08/23/01
%X	Time representation *	14:55:02
%y	Year, last two digits (00-99)	01
%Y	Year	2001
%z	ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)
If timezone cannot be termined, no characters	+100
%Z	Timezone name or abbreviation *
If timezone cannot be termined, no characters	CDT
%%	A % sign	%
*/
std::string fmtTime(const time_t value, const std::string &fmt, bool useglobaltime)
{
	struct tm *timeinfo;
	char buffer[80];
	if (useglobaltime)
		timeinfo = gmtime(&value);
	else
		timeinfo = localtime(&value);
	strftime(buffer, 80, fmt.c_str(), timeinfo);
	return std::string(&buffer[0]);
}


std::string getCommandName(int command)
{
	switch (command) 
	{
		case C_RESPONSE: return "RESPONSE";
		case C_ACK: return ACK;
		case C_BYE: return BYE;
		case C_CANCEL: return CANCEL;
		case C_INVITE: return INVITE;
		case C_OPTIONS: return OPTIONS;
		case C_REGISTER: return REGISTER;
		case C_NOTIFY: return NOTIFY;
		case C_SUBSCRIBE: return SUBSCRIBE;
		case C_MESSAGE: return MESSAGE;
		case C_REFER: return REFER;
		case C_INFO: return INFO;
		case C_PRACK: return PRACK;
		case C_UPDATE: return UPDATE;
		case C_PUBLISH: return PUBLISH;
		default:
			return "";
	}
}

int getCommand(const std::string &command) {
	if (command == REGISTER)
		return C_REGISTER;
	else
		if (command == INVITE)
			return C_INVITE;
		else
			if (command == REFER)
				return C_REFER;
			else
				if (command == BYE)
					return C_BYE;
				else
					if (command == NOTIFY)
						return C_NOTIFY;
					else
						if (command == ACK)
							return C_ACK;
						else
							if (command == CANCEL)
								return C_CANCEL;
							else
								if (command == OPTIONS)
									return C_OPTIONS;
								else
									if (command == SUBSCRIBE)
										return C_SUBSCRIBE;
									else
										if (command == MESSAGE)
											return C_MESSAGE;
										else
											if (command == INFO)
												return C_INFO;
											else
												if (command == PRACK)
													return C_PRACK;
												else
													if (command == UPDATE)
														return C_UPDATE;
													else
														if (command == PUBLISH)
															return C_PUBLISH;
	return C_INVALID;
}

std::string getReasonPhrase(int rc) 
{
	std::string retval = "";
	switch (rc) {
	case TRYING:
		retval = "Trying";
		break;
	case DIALOG_ESTABLISHEMENT:
		retval = "Dialog Establishement";
		break;
	case RINGING:
		retval = "Ringing";
		break;
	case CALL_IS_BEING_FORWARDED:
		retval = "Call is being forwarded";
		break;
	case QUEUED:
		retval = "Queued";
		break;
	case SESSION_PROGRESS:
		retval = "Session progress";
		break;
	case OK:
		retval = "OK";
		break;
	case ACCEPTED:
		retval = "Accepted";
		break;
	case MULTIPLE_CHOICES:
		retval = "Multiple choices";
		break;
	case MOVED_PERMANENTLY:
		retval = "Moved permanently";
		break;
	case MOVED_TEMPORARILY:
		retval = "Moved Temporarily";
		break;
	case USE_PROXY:
		retval = "Use proxy";
		break;
	case ALTERNATIVE_SERVICE:
		retval = "Alternative service";
		break;
	case BAD_REQUEST:
		retval = "Bad request";
		break;
	case UNAUTHORIZED:
		retval = "Unauthorized";
		break;
	case PAYMENT_REQUIRED:
		retval = "Payment required";
		break;
	case FORBIDDEN:
		retval = "Forbidden";
		break;
	case NOT_FOUND:
		retval = "Not found";
		break;
	case METHOD_NOT_ALLOWED:
		retval = "Method not allowed";
		break;
	case NOT_ACCEPTABLE:
		retval = "Not acceptable";
		break;
	case PROXY_AUTHENTICATION_REQUIRED:
		retval = "Proxy Authentication required";
		break;
	case REQUEST_TIMEOUT:
		retval = "Request timeout";
		break;
	case GONE:
		retval = "Gone";
		break;
	case TEMPORARILY_UNAVAILABLE:
		retval = "Temporarily Unavailable";
		break;
	case REQUEST_ENTITY_TOO_LARGE:
		retval = "Request entity too large";
		break;
	case REQUEST_URI_TOO_LONG:
		retval = "Request-URI too large";
		break;
	case UNSUPPORTED_MEDIA_TYPE:
		retval = "Unsupported media type";
		break;
	case UNSUPPORTED_URI_SCHEME:
		retval = "Unsupported URI Scheme";
		break;
	case BAD_EXTENSION:
		retval = "Bad extension";
		break;
	case EXTENSION_REQUIRED:
		retval = "Etension Required";
		break;
	case INTERVAL_TOO_BRIEF:
		retval = "Interval too brief";
		break;
	case CALL_OR_TRANSACTION_DOES_NOT_EXIST:
		retval = "Call leg/Transaction does not exist";
		break;
	case LOOP_DETECTED:
		retval = "Loop detected";
		break;
	case TOO_MANY_HOPS:
		retval = "Too many hops";
		break;

	case ADDRESS_INCOMPLETE:
		retval = "Address incomplete";
		break;

	case AMBIGUOUS:
		retval = "Ambiguous";
		break;

	case BUSY_HERE:
		retval = "Busy here";
		break;

	case REQUEST_TERMINATED:
		retval = "Request Terminated";
		break;

		//Issue 168, Typo fix reported by fre on the retval
	case NOT_ACCEPTABLE_HERE:
		retval = "Not Acceptable here";
		break;

	case BAD_EVENT:
		retval = "Bad Event";
		break;

	case REQUEST_PENDING:
		retval = "Request Pending";
		break;

	case SERVER_INTERNAL_ERROR:
		retval = "Server Internal Error";
		break;

	case UNDECIPHERABLE:
		retval = "Undecipherable";
		break;

	case NOT_IMPLEMENTED:
		retval = "Not implemented";
		break;

	case BAD_GATEWAY:
		retval = "Bad gateway";
		break;

	case SERVICE_UNAVAILABLE:
		retval = "Service unavailable";
		break;

	case SERVER_TIMEOUT:
		retval = "Gateway timeout";
		break;

	case VERSION_NOT_SUPPORTED:
		retval = "SIP version not supported";
		break;

	case MESSAGE_TOO_LARGE:
		retval = "Message Too Large";
		break;

	case BUSY_EVERYWHERE:
		retval = "Busy everywhere";
		break;

	case DECLINE:
		retval = "Decline";
		break;

	case DOES_NOT_EXIST_ANYWHERE:
		retval = "Does not exist anywhere";
		break;
	case SESSION_NOT_ACCEPTABLE:
		retval = "Session Not acceptable";
		break;
	case CONDITIONAL_REQUEST_FAILED:
		retval = "Conditional request failed";
		break;
	default:
		retval = "Unknown Status";
	}
	return retval;
}

std::vector<std::string> split(char* str, const char* delim, std::vector<std::string> &result)
{
	char* token = strtok(str, delim);
	while (token != NULL)
	{
		result.push_back(token);
		token = strtok(NULL, delim);
	}
	return result;
}

std::vector<std::string> split(const std::string &str, const std::string &delim, std::vector<std::string> &result)
{
	std::string::size_type pos, lastPos = 0;
	const char *data = str.data();
	while (true)
	{
		pos = str.find_first_of(delim, lastPos);
		if (pos == std::string::npos)
		{
			pos = str.length();
			// if (pos != lastPos)
			result.push_back(std::string(data + lastPos, pos - lastPos));
			break;
		}
		else
		{
			// if (pos != lastPos)
			result.push_back(std::string(data + lastPos, pos - lastPos));
		}
		lastPos = pos + 2;
	}
	return result;
}

// Java like string functions

// trim from start
std::string ltrim(const std::string &value)
{
	std::string s(value);
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
std::string rtrim(const std::string &value) 
{
	std::string s(value);
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}


// trim from both ends
std::string trim(const std::string &value)
{
	return ltrim(rtrim(value));
}

std::string toLower(const std::string &value)
{
	std::string r(value);
	std::transform(r.begin(), r.end(), r.begin(), ::tolower);
	return r;
}

std::string toUpper(const std::string &value)
{
	std::string r(value);
	std::transform(r.begin(), r.end(), r.begin(), ::toupper);
	return r;
}

bool contains(const std::string &str, const std::string &value)
{
	return str.find(value) != std::string::npos;
}

std::string replaceChar(const std::string &value, char cfrom, char cto)
{
	std::string r(value);
	std::replace(r.begin(), r.end(), cfrom, cto);
	return r;
}

/*
startsWith("012345", "45")
*/
bool startsWith(const std::string &str, const std::string &value)
{
	// return str.find(value) == 0;
	return str.compare(0, value.length(), value) == 0;
}

/*
	endsWith("012345", "45")
	6 - 2 = 4
*/
bool endsWith(const std::string &str, const std::string &value)
{
	return str.rfind(value) == str.length() - value.length();
}

// get sockaddr, IPv4 or IPv6:
std::string addr2String(struct sockaddr_in *addr)
{
	return std::string(inet_ntoa(addr->sin_addr));
}

/*
	Resolve host name or IPv4 address string to IPv4 address.
	Return true, if at least one address is resolved
	Return false, if host is unknown
*/
bool char2addrIP4(const char *hostname, struct sockaddr_in *result)
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
	if ((he = gethostbyname(hostname)) == NULL)
	{
		return false;
	}
	addr_list = (struct in_addr **) he->h_addr_list;
	for (i = 0; addr_list[i] != NULL; i++)
	{
		// Return the first one;
		result->sin_family = AF_INET;
		result->sin_port = 0;
		result->sin_addr = *addr_list[i];
		return true;
	}
	return false;
}

/*
Resolve host name or IPv4 address string to IPv4 address.
Return true, if at least one address is resolved
Return false, if host is unknown
*/
bool string2addrIP4(const std::string &hostname, struct sockaddr_in *result)
{
	return char2addrIP4(hostname.c_str(), result);
}

/*
	Resolve host name or IPv4 address string to IPv4 address.
	Return true, if at least one address is resolved
	Return false, if host is unknown
*/
bool string2addr(const char *hostname, struct sockaddr_in *result)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, "http", &hints, &servinfo)) != 0)
	{
		return false;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		memcpy(result, (struct sockaddr_in *) p->ai_addr, sizeof(struct sockaddr_in));
		break;
	}

	freeaddrinfo(servinfo); // all done with this structure
	return true;
}

/*
	TODO IPv4 only
*/
bool addrEquils(struct sockaddr_in *a, struct sockaddr_in *b)
{
	if (a->sin_family != a->sin_family)
		return false;
	if (a->sin_addr.s_addr != a->sin_addr.s_addr)
		return false;
	return true;

}

// Return "" if file does not exists
std::string readFile(const std::string &filename)
{
	FILE* f = fopen(filename.c_str(), "r");
	if (!f)
		return "";
	// Determine file size
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	char* buf = new char[size + 1];
	rewind(f);
	fread(buf, sizeof(char), size, f);
	buf[size] = '\0';
	fclose(f);
	std::string r(buf);
	delete[] buf;
	return r;
	/*
	std::ifstream t(filename);
	std::stringstream buffer;
	buffer << t.rdbuf();
	std::string r(buffer.str());
	t.close();
	return r;
	*/
}

int writeFile(const std::string &filename, const std::string &data)
{
	std::ofstream t(filename, std::ofstream::out);
	t << data;
	t.close();
	return 0;
}

int appendFile(const std::string &filename, const std::string &data)
{
	std::ofstream t(filename, std::ofstream::out | std::ofstream::app);
	t << data;
	t.close();
	return 0;
}

std::string getSystemErrorDescription(int errorcode)
{
#ifdef WIN32
	char lpBuffer[256];
	::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), lpBuffer, 255, NULL);
	return std::string(lpBuffer);
#else
	// TODO
	return "";
#endif
}
