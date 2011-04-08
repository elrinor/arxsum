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

class Digest {
private:
  class DigestImpl;
  boost::shared_ptr<DigestImpl> impl;
public:
  Digest();
  Digest(const unsigned char* data, unsigned int len);
  std::size_t size() const;
  const unsigned char* get() const;
  bool operator== (const Digest& that) const;
  bool operator!= (const Digest& that) const;

  const std::string toHexString() const; 
  static Digest fromHexString(const std::string& hex);
};

namespace detail {
  class HashImpl;
};

class Hash {
private:
  boost::shared_ptr<detail::HashImpl> impl;
  uint32 hashId;
public:
  Hash(uint32 hashId, uint64 totalLen);
  void update(const void* data, size_t len);
  Digest finalize();
  std::string getName();
  uint32 getId();
  static std::string getName(uint32 hashId);
};

Digest calculateHash(const void* data, size_t len, uint32 hashId);

#endif
