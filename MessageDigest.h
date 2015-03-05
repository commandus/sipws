#ifndef MESSAGEDIGEST_H
#define MESSAGEDIGEST_H	1

#include <openssl/md5.h>

class MessageDigest
{
private:
	MD5_CTX ctx;
	unsigned char md5[16];
public:
	MessageDigest();
	~MessageDigest();
	void reset();
	int update(const void *data, size_t size);
	unsigned char *digest(const void *data, size_t size);
};

#endif