#include "config.h"
#include "Hash.h"
#include <string>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "hash/md4/md4.h"
#include "hash/md5/md5.h"
#include "hash/sha/sha.h"
#include "hash/tth/TigerTree.h"

using namespace std;
using namespace boost;
using namespace boost::algorithm;

// ------------------------------------------------------------------------- //
// Supplementary functions
// ------------------------------------------------------------------------- //
Digest calculateHash(const void* data, size_t len, uint32 hashId) {
  Hash hash(hashId, len);
  hash.update(data, len);
  return hash.finalize();
}

unsigned char hexUnDump(char a) {
  if(a >= '0' && a <= '9')
    return (a - '0');
  if(a >= 'a' && a <= 'z')
    return a - 'a' + 10;
  return a - 'A' + 10;
}

unsigned char hexUnDump(char a, char b) {
  return (hexUnDump(a) * 16) + hexUnDump(b);
}

void hexUnDump(const std::string& hex, unsigned char* dst) {
  for(unsigned int i = 0; i < hex.size(); i += 2)
    dst[i / 2] = hexUnDump(hex[i], hex[i + 1]);
}

std::string hexDump(unsigned char c) {
  static const char base16[] = "0123456789abcdef";
  return string() + base16[c / 16] + base16[c % 16];
}

std::string hexDump(const unsigned char* data, unsigned int size) {
  string result;
  for(unsigned int i = 0; i < size; i++)
    result += hexDump(data[i]);
  return result;
}

std::string base32Dump(unsigned char* data, unsigned int size) {
  static const char base32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567=";
  string result;
  int base32Chars = (size * 8 + 4) / 5;
  int shift = 11;
  int i = 0, ch = 0;
  do {
    result += base32[((data[i] * 256 + data[i + 1]) >> shift) & 0x1f];
    shift -= 5;
    if(shift <= 0) {
      shift += 8;
      ++i;
    }
  } while (++ch < base32Chars - 1);
  result += base32[(data[size - 1] << (base32Chars * 5 % CHAR_BIT)) & 0x1f];
  return result;
}


namespace detail {
// ------------------------------------------------------------------------- //
// HashImpl class
// ------------------------------------------------------------------------- //
  class HashImpl {
  public:
    virtual void update(const void* data, size_t len) = 0;
    virtual Digest finalize() = 0;
    virtual std::string getName() = 0;
    virtual ~HashImpl() {};
  };


// ------------------------------------------------------------------------- //
// CRC
// ------------------------------------------------------------------------- //
  static const uint32 s_crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,

    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,

    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,

    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
  };

  class CRC: public HashImpl {
  private:
    uint32 crc32;

  public:
    CRC() {
      this->crc32 = 0xFFFFFFFF;
    }

    void update(const void *data, size_t len) {
    #ifdef CRC_ASM
      if(len == 0)
        return;
      // There is a bug in the Microsoft compilers where inline assembly
      // code cannot access static member variables.  This is a work around
      // for that bug.  For more info see Knowledgebase article Q88092
      void* crc32Table = (void*) s_crc32Table;
      uint32* c = &(this->crc32); //pointer to crc32

      // Register use:
      //    eax - CRC32 value
      //    ebx - a lot of things
      //    ecx - CRC32 value
      //    edx - address of end of buffer
      //    esi - address of start of buffer
      //    edi - CRC32 table
      //
      // assembly part by Brian Friesen. Visit CodeGuru.com
      __asm {
        // Save the esi and edi registers
        push esi
        push edi

        mov eax, c            // Load the pointer to c
        mov ecx, [eax]        // Dereference the pointer to load Crc32

        mov edi, crc32Table    // Load the CRC32 table

        mov esi, data          // Load buffer
        mov ebx, len          // Load dwBytesRead
        lea edx, [esi + ebx]  // Calculate the end of the buffer


        crc32loop:
        xor eax, eax          // Clear the eax register
        mov bl, byte ptr [esi]// Load the current source byte

        mov al, cl            // Copy crc value into eax
        inc esi                // Advance the source pointer

        xor al, bl            // Create the index into the CRC32 table
        shr ecx, 8

        mov ebx, [edi + eax * 4]  // Get the value out of the table
        xor ecx, ebx          // xor with the current byte

        cmp edx, esi          // Have we reached the end of the buffer?
        jne crc32loop

        // Restore the edi and esi registers
        pop edi
        pop esi

        mov eax, c            // Load the pointer to c
        mov [eax], ecx        // Write the result
      }
    #else
      unsigned char* buf = (unsigned char*) data;
      while(len--)
        crc32 = (crc32 >> 8) ^ s_crc32Table[(crc32 ^ *buf++) & 0xff];
    #endif
    }

    Digest finalize() {
      this->crc32 = ~(this->crc32);
      byte md[4];
      md[0] = ((byte*) &this->crc32)[3];
      md[1] = ((byte*) &this->crc32)[2];
      md[2] = ((byte*) &this->crc32)[1];
      md[3] = ((byte*) &this->crc32)[0];
      return Digest(md, 4);
    }

    std::string getName() {
      return "CRC";
    }
  };

// ------------------------------------------------------------------------- //
// MD4
// ------------------------------------------------------------------------- //
  class MD4: public HashImpl {
  private:
    MD4_CTX ctx;
  public:
    MD4() {
      init();
    }

    void init() {
      MD4_Init(&this->ctx);
    }

    void update(const void* data, size_t len) {
      MD4_Update(&this->ctx, data, len);
    }

    Digest MD4::finalize() {
      byte md[SHA_DIGEST_LENGTH];
      MD4_Final(md, &this->ctx);
      return Digest(md, MD4_DIGEST_LENGTH);
    }

    std::string getName() {
      return "MD4";
    }
  };

// ------------------------------------------------------------------------- //
// ED2K
// ------------------------------------------------------------------------- //
  class ED2K: public HashImpl {
  private:
    MD4 mainMD4, finMD4;
    uint64 pos;
    uint32 blockPos;
    static const uint32 ed2kBorder = 9728000;
  
  public:
    ED2K() {
      this->pos = 0;
      this->blockPos = 0;
    }

    void update(const void* data, size_t len) {
      const byte* pData = (const byte*) data;
      this->pos += len;
      while(len > 0) {
        size_t dlen = min(len, ed2kBorder - this->blockPos);
        mainMD4.update(pData, dlen);
        len -= dlen;
        pData += dlen;
        this->blockPos = (this->blockPos + dlen) % ed2kBorder;
        if(this->blockPos == 0) {
          finMD4.update((const void*) mainMD4.finalize().get(), MD4_DIGEST_LENGTH);
          mainMD4.init();
        }
      }
    }

    Digest ED2K::finalize() {
      // Elric [1/14/2007]:
      // I've just found out that there are 2 different ways of dealing with files,
      // which size is the multiple of 9728000.
      // I've switched to the second one, which is used in eMule:
      // If a filesize is a multiple of 9728000, append a md4 of null to hashlist, 
      // and then hash it.
      //
      // File of zeros with size 9728000:
      // 1st method: d7def262a127cd79096a108e7a9fc138
      // 2nd method: fc21d9af828f92a8df64beac3357425d
      //
      // see http://wiki.anidb.info/w/AniDB:Ed2k-hash for details
      if(this->pos >= ed2kBorder) {
        this->finMD4.update((const void*) this->mainMD4.finalize().get(), MD4_DIGEST_LENGTH);
        return this->finMD4.finalize();
      } else
        return this->mainMD4.finalize();
    }

    std::string getName() {
      return "ED2K";
    }
  };

// ------------------------------------------------------------------------- //
// MD5
// ------------------------------------------------------------------------- //
  class MD5: public HashImpl {
  private:
    MD5_CTX ctx;

  public:
    MD5() {
      MD5_Init(&this->ctx);
    }

    void update(const void* data, size_t len) {
      MD5_Update(&this->ctx, data, len);
    }

    Digest finalize() {
      byte md[MD5_DIGEST_LENGTH];
      MD5_Final(md, &this->ctx);
      return Digest(md, MD5_DIGEST_LENGTH);
    }

    std::string getName() {
      return "MD5";
    }
  };

// ------------------------------------------------------------------------- //
// SHA1
// ------------------------------------------------------------------------- //
  class SHA1: public HashImpl {
  private:
    SHA_CTX ctx;

  public:
    SHA1() {
      SHA1_Init(&this->ctx);
    }

    void update(const void* data, size_t len) {
      SHA1_Update(&this->ctx, data, len);
    }

    Digest finalize() {
      byte md[SHA_DIGEST_LENGTH];
      SHA1_Final(md, &this->ctx);
      return Digest(md, SHA_DIGEST_LENGTH);
    }

    std::string getName() {
      return "SHA1";
    }
  };

// ------------------------------------------------------------------------- //
// SHA256
// ------------------------------------------------------------------------- //
  class SHA256: public HashImpl {
  private:
    SHA256_CTX ctx;

  public:
    SHA256() {
      SHA256_Init(&this->ctx);
    }

    void update(const void* data, size_t len) {
      SHA256_Update(&this->ctx, data, len);
    }

    Digest finalize() {
      byte md[SHA256_DIGEST_LENGTH];
      SHA256_Final(md, &this->ctx);
      return Digest(md, SHA256_DIGEST_LENGTH);
    }

    std::string getName() {
      return "SHA256";
    }
  };

// ------------------------------------------------------------------------- //
// SHA512
// ------------------------------------------------------------------------- //
  class SHA512: public HashImpl {
  private:
    SHA512_CTX ctx;

  public:
    SHA512() {
      SHA512_Init(&this->ctx);
    }

    void update(const void* data, size_t len) {
      SHA512_Update(&this->ctx, data, len);
    }

    Digest finalize() {
      byte md[SHA512_DIGEST_LENGTH];
      SHA512_Final(md, &this->ctx);
      return Digest(md, SHA512_DIGEST_LENGTH);
    }

    std::string getName() {
      return "SHA512";
    }
  };

// ------------------------------------------------------------------------- //
// TTH
// ------------------------------------------------------------------------- //
  class TTH: public HashImpl {
  private:
    CTigerTree ctx;

  public:
    TTH(unsigned long long fileSize) {
      this->ctx.BeginFile(9, fileSize);
    }

    void update(const void* data, size_t len) {
      this->ctx.AddToFile(data, (DWORD) len);
    }

    Digest finalize() {
      byte md[24];
      this->ctx.FinishFile();
      this->ctx.GetRoot(md);
      return Digest(md, 24);
    }

    std::string getName() {
      return "TTH";
    }
  };

}; // namespace hashes

// ------------------------------------------------------------------------- //
// Hash class
// ------------------------------------------------------------------------- //
Hash::Hash(uint32 hashId, uint64 totalLen): hashId(hashId) {
  switch(hashId) {
  case H_CRC:
    impl.reset(new ::detail::CRC());
    break;
  case H_MD4:
    impl.reset(new ::detail::MD4());
    break;
  case H_ED2K:
    impl.reset(new ::detail::ED2K());
    break;
  case H_MD5:
    impl.reset(new ::detail::MD5());
    break;
  case H_SHA1:
    impl.reset(new ::detail::SHA1());
    break;
  case H_SHA256:
    impl.reset(new ::detail::SHA256());
    break;
  case H_SHA512:
    impl.reset(new ::detail::SHA512());
    break;
  case H_TTH:
    impl.reset(new ::detail::TTH(totalLen));
    break;
  default:
    throw new std::runtime_error("Unknowh Hash Id: " + lexical_cast<string>(hashId));
  }
}

void Hash::update(const void* data, size_t len) {
  this->impl->update(data, len);
}

Digest Hash::finalize() {
  return this->impl->finalize();
}

std::string Hash::getName() {
  return this->impl->getName();
}

uint32 Hash::getId() {
  return this->hashId;
}

std::string Hash::getName(uint32 hashId) {
  // TODO: Awful! Fix it!
  return Hash(hashId, 0).getName();
}

uint32 Hash::getId(std::string name) {
  // TODO: aren't you tired of writing "TODO" comments? <_<
  trim(name);
  to_upper(name);
  if(name == "CRC" || name == "CRC32")
    return H_CRC;
  else if(name == "MD4")
    return H_MD4;
  else if(name == "MD5")
    return H_MD5;
  else if(name == "ED2K")
    return H_ED2K;
  else if(name == "SHA1")
    return H_SHA1;
  else if(name == "SHA256")
    return H_SHA256;
  else if(name == "SHA512")
    return H_SHA512;
  else if(name == "TTH")
    return H_TTH;
  else
    return H_UNKNOWN;
}



// -------------------------------------------------------------------------- //
// Digest class
// -------------------------------------------------------------------------- //
class Digest::DigestImpl {
public:
  unsigned int len;
  unsigned char* data;

  DigestImpl(const unsigned char* data, unsigned int len) {
    this->len = len;
    this->data = new unsigned char[this->len];
    memcpy(this->data, data, this->len);
  }

  DigestImpl() {
    return;
  }

  ~DigestImpl() {
    delete[] this->data;
  }

  static DigestImpl* fromHexString(const std::string& hex) {
    DigestImpl* result = new DigestImpl();
    result->len = (unsigned int) hex.size() / 2;
    result->data = new unsigned char[result->len];
    hexUnDump(hex, result->data);
    return result;
  }
};

Digest::Digest(const unsigned char* data, unsigned int len): impl(new DigestImpl(data, len)) {
  return;
}

Digest::Digest() {
  return;
}

bool Digest::operator== (const Digest& that) const {
  return (this->impl->len == that.impl->len && memcmp(this->impl->data, that.impl->data, this->impl->len) == 0);
}

bool Digest::operator!= (const Digest& that) const {
  return !this->operator== (that);
}

unsigned int Digest::size() const {
  return this->impl->len;
}

const unsigned char* Digest::get() const {
  return this->impl->data;
}

const std::string Digest::toHexString() const {
  return hexDump(this->impl->data, this->impl->len);
}

Digest Digest::fromHexString(const std::string& hex) {
  Digest result;
  result.impl.reset(DigestImpl::fromHexString(hex));
  return result;
}