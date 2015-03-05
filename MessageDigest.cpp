#include "MessageDigest.h"


MessageDigest::MessageDigest()
{
	MD5_Init(&ctx);
}

MessageDigest::~MessageDigest()
{
}

void MessageDigest::reset()
{
	MD5_Init(&ctx);
}

int MessageDigest::update(const void *data, size_t size)
{
	return MD5_Update(&ctx, data, size);
}

unsigned char *MessageDigest::digest(const void *data, size_t size)
{
	update(data, size);
	MD5_Final(&md5[0], &ctx);
	return &md5[0];
}

