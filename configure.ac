# sipws SIP server
AC_PREREQ([2.63])
AC_INIT([sipws], [0.5], [support@commandus.com])
AC_CONFIG_FILES([Makefile])
AM_INIT_AUTOMAKE([1.11 foreign no-define])
AC_CONFIG_SRCDIR([sipws.cpp])
AC_CONFIG_HEADERS([config.h])

m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_pthread.m4])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

# C++ 11
AX_CXX_COMPILE_STDCXX_11

# Checks for libraries.
AC_CHECK_LIB([pthread], [main])
AX_BOOST_BASE()
AC_CHECK_LIB([boost_system], [main])
AC_CHECK_LIB([boost_random], [main])
AC_CHECK_LIB([boost_thread], [main])

AX_PTHREAD()
AC_CHECK_HEADERS([argtable2.h], [], AC_MSG_ERROR([Could not find argtable2.h. Try $ ./configure CFLAGS='-Iyour-argtable2-include-path]))
#AC_CHECK_HEADERS([json/json.h], [], AC_MSG_ERROR([Could not find json/json.h. Try $ ./configure CFLAGS='-Iyour-jsoncpp-include-path]))
AC_CHECK_LIB(argtable2, arg_parse, [], AC_MSG_ERROR([Could not find libargtable2. Try $ ./configure LDFLAGS='-Lyour-argtable2-lib-path']))
AC_CHECK_LIB(jsoncpp, main, [], AC_MSG_ERROR([Could not find libjsoncpp. Try $ ./configure LDFLAGS='-Lyour-jsoncpp-lib-path']))
AC_CHECK_LIB(ssl, main, [], AC_MSG_ERROR([Could not find libssl. Try $ ./configure LDFLAGS='-Lyour-ssl-lib-path']))
AC_CHECK_LIB(crypto, main, [], AC_MSG_ERROR([Could not find libssl. Try $ ./configure LDFLAGS='-Lyour-ssl-lib-path']))
AC_CHECK_LIB(event, main, [], AC_MSG_ERROR([Could not find libevent. Try $ ./configure LDFLAGS='-Lyour-event-lib-path']))
#AC_CHECK_HEADERS([boost/thread.hpp], [], AC_MSG_ERROR([Could not find boost/*.hpp. Try $ ./configure CFLAGS='-Iyour-boost-include-path]))

AC_CHECK_HEADERS([fcntl.h])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h])
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([websocketpp/version.hpp], [], AC_MSG_ERROR([Could not find websocketpp/*.hpp. Try $ ./configure CFLAGS='-Iyour-websocketpp-include-path]))
AC_LANG_POP([C++])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_HEADER_STDBOOL
AC_TYPE_INT64_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MKTIME
AC_CHECK_FUNCS([gethostbyname inet_ntoa memset strtol])

AC_OUTPUT
