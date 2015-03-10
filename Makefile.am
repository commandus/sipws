SUBDIRS = .

AM_CPPFLAGS = -I.
bin_PROGRAMS=sipws
sipws_SOURCES= \
	AuthHeaderMD5.cpp       HTTPListener.cpp     RegistryResponse.cpp  SipMessage.cpp   sipwsUtil.cpp \
	ConfigWSListener.cpp    MessageDigest.cpp    signalhandler.cpp     SipRegistry.cpp  strptime.cpp \
	connectionmetadata.cpp  RegistryEntry.cpp    SipAddress.cpp        SipServer.cpp    websocketendpoint.cpp \
	consolelocale.cpp       RegistryRequest.cpp  SipDialog.cpp         sipws.cpp        WSListener.cpp \
	Logger.cpp              LoggerFunc.cpp       SipListener.cpp       Deamonize.cpp    argsplit.cpp
LDFLAGS = -L/usr/local/lib
sipws_LDADD = \
	-L/usr/local/lib \
	-lpthread -levent -lboost_thread -lboost_random -ljsoncpp
