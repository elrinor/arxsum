#ifndef __HASH_H__
#define __HASH_H__

#include "config.h"
#include <string>
#include <boost/shared_ptr.hpp>

enum {
  H_CRC,
  H_MD4,
  H_ED2K,
  H_MD5,
  H_SHA1,
  H_SHA256,
  H_SHA512,
  H_TTH,
  H_COUNT,
  H_UNKNOWN = -1
};

namespace __hashes {
	class HashImpl;
};

class Hash {
private:
	boost::shared_ptr<__hashes::HashImpl> impl;
	uint32 hashId;
public:
	Hash(uint32 hashId, uint64 totalLen);
	void update(const void* data, size_t len);
	std::string finalize();
	std::string getName();
	uint32 getId();
	static std::string getName(uint32 hashId);
};

std::string calculateHash(const void* data, size_t len, uint32 hashId);

#endif
