SIPWS
=====

SIP server websocket transport
------------------------------

Simple all-in-one SIP server UDP, TCP, WS (websocket) transport.

SIP accounts managed by POST Json requests served by embedded HTTP server.

This is beta.

Build
-----

./configure  
make  

### Building Windows


Open sipws.sln VC solution, change include and library path.

Dependencies
------------

Libraries
* [WebSocket++]	(http://www.zaphoyd.com/websocketpp/) and boost
* [argtable2] (http://argtable.sourceforge.net/) 
* [OpenSSL] (https://www.openssl.org/) 
* [libevent] (http://libevent.org/) 
* [JsonCpp] (https://github.com/open-source-parsers/jsoncpp) 

Start
-----

Start as usual (foreground) process with -f option:

./sipws -f

By default, sipws running as deamon (or Windows service). Skip -f option:

./sipws

Create service in Windows:  
sc create "sipws" binPath= <path>\sipws.exe  
sc delete "sipws"  


### Enable SIP transports

* UDP -u 5060
* TCP -t 5060
* WS -p 8080

-u, -t, -p -s port options enables UDP, TCP, WS, WSS transports.

Enable at least one transport.

### Enable HTTP server

HTTP server provide account control.
* -r 8082 


### Set specific network inteface

-i 192.168.43.113  

By default, sipws listen 0.0.0.0 address (all interfases).

### Enable debug output

* -vvv	debug info
* -vv	more detailed log
* -v	enable logging

In deamon mode, output routes to syslog (*nix) or <binary path>\sipws.log file. 
Please DO NOT use logging in production.

Otherwise fatal errors only.

Load options from file
======================

sipws search sipws.cfg in the sipws binary file directory, then in the current directory, last in the /etc or windows directory.

sipws.cfg is a file with command line string. 
Create sipws.cfg to start daemon (service).

If file sipws.cfg exists, skip reading config with  --skipconfig option.


Provision accounts
==================

Create "database" file:

{  
	"data":[  
		{"avail":"0","cn":"Alice","description":"","domain":"192.168.43.113","expire":3600,"id":"100","image":"http://acme.com/brb/sipws/i/100.png","origin":"registry","password":"password","port":5060},  
		...  
	],  
	"domain":"192.168.43.113",  
	"ip":"192.168.43.113"  
}  

Assign "database" file :  -b, --db=<file>

Provision accounts at startup
=============================

In addition to -b, there is -d, --data=<file> option loads accounts from the "request" file.

Provision accounts dynamically
==============================

When -r, --httpport=<port> option is set, POST Json requests to add or delete SIP accounts, start or stop service, clear accounts.
Key must be provided. sipws compare key with valid keys. If key is valid, operation is permitted.

### Start, stop, clear

#### Request

{“q”:”<signal>”, “domain”:””, “key”:”...”}

<signal> = start,stop,clear

#### Response

{“q”:<signal>, “errorcode”:”1”}

### List

#### Request

{“q”:”<signal>”, “domain”:””, “key”:”...”}

<signal> = list

#### Response

Success:

{“q”:”list”, “errorcode”:”1”} -invalid key

Error:

{“q”:”list”, “registry”:[<address>,..]} 

<address>: { "avail", "proto", "origin", "key", "cn", "description", "id", "domain", "host", "line", "tag", "rinstance", "image", "port",	"expire", "registered",	"updated"
}

### Provide user registry

#### Request

{“q”:”<signal>”, “domain”:””, “key”:”...”, “data”:[<entry>,...], “v”:124}

<signal> = put

<entry>: {“o”:”+”, “u”:”<user name>”, “k”:”<password>”, “cn”:”common name”, “description”:””, “i”:”<image uri>”}

c: +,- - add(edit), delete

#### Response

{“q”:”put”, “errorcode”:”1”, “v”:123}

v: version number before update (0..)


When -r, --httpport=<port> option is set, GET Json requests registered accounts.


Other options
=============

./sipws -h

Usage sipws  
 [-fv8h] [-i <IP>] [-r <port>] [-p <port>] [-s <port>] [-u <port>] [-t <port>] [-c <file>] [-k <file>] [-w <key>] [-d <file>] [-b <file>] [-l <locale>] [--skipconfig] <key> [<key>]...   

simple websocket SIP service

  -i, --interface=<IP>      host name or IP address. Default 0.0.0.0- all interfaces  
  -r, --httpport=<port>     HTTP register control port number, e.g. 8082  
  -p, --port=<port>         SIP WS port number, e.g. 8080  
  -s, --tlsport=<port>      SIP WSS port number. e.g. 443  
  -u, --udpport=<port>      SIP UDP port number. Disable UDP transport if not specified  
  -t, --tcpport=<port>      SIP TCP port number. Disable TCP transport if not specified  
  -c, --certificate=<file>  certificate file, default server.pem  
  -k, --pk=<file>           private key file, default server.pem  
  -w, --password=<key>      certificate password, e.g. test  
  -d, --data=<file>         initial registry request JSON file  
  -b, --db=<file>           writeable database file name  
  -f, --foreground          Do not start deamon  
  -v, --verbose             severity: -v: error, -vv: warning, -vvv: debug. Default fatal error only.  
  -l, --locale=<locale>     e.g. russian_russia.1251, ru_RU.UTF-8  
  -8, --utf8                locale use UTF-8  
  --skipconfig              Skip reading argruments from sipws.cfg  
  -h, --help                print this help and exit  
  <key>                     update registry valid keys (-r)  


