#include "config.h"
#include "FileEntry.h"

#include <boost/filesystem.hpp>
#include <boost/array.hpp>

using namespace std;
using namespace boost;
using namespace boost::filesystem;

class FileEntry::FileEntryImpl {
private:
  boost::filesystem::wpath name;
  boost::array<Digest, H_COUNT> digest;
  unsigned long long size;
  time_t dateTime;
  bool failed;
public:
  FileEntryImpl(wpath name) : name(name), failed(false) {};
  FileEntryImpl(wstring name): name(boost::filesystem::wpath(name)), failed(false) {};
  FileEntryImpl(wchar_t* name): name(boost::filesystem::wpath(name)), failed(false) {};
  bool isFailed() const {
    return this->failed;
  }
  void setFailed(bool failed) {
    this->failed = failed;
  }
  wpath getPath() const {
    return this->name;
  }
  void setSize(uint64 size) {
    this->size = size;
  }
  uint64 getSize() const {
    return this->size;
  }
  void setDateTime(time_t dateTime) {
    this->dateTime = dateTime;
  }
  time_t getDateTime() const {
    return this->dateTime;
  }
  void setDigest(uint32 id, Digest digestString) {
    digest[id] = digestString;
  }
  Digest getDigest(uint32 id) const {
    return digest[id];
  }
  bool operator< (const FileEntryImpl& that) const {
    return this->name < that.name;
  }
};

FileEntry::FileEntry(wpath name): impl(new FileEntryImpl(name)) {};
FileEntry::FileEntry(wstring name): impl(new FileEntryImpl(name)) {};
FileEntry::FileEntry(wchar_t* name): impl(new FileEntryImpl(name)) {};
bool FileEntry::isFailed() const {
  return impl->isFailed();
}
void FileEntry::setFailed(bool failed) {
  impl->setFailed(failed);
}
wpath FileEntry::getPath() const {
  return impl->getPath();
}
void FileEntry::setSize(uint64 size) {
  impl->setSize(size);
}
uint64 FileEntry::getSize() const {
  return impl->getSize();
}
void FileEntry::setDateTime(time_t dateTime) {
  impl->setDateTime(dateTime);
}
time_t FileEntry::getDateTime() const {
  return impl->getDateTime();
}
void FileEntry::setDigest(uint32 id, Digest digestString) {
  impl->setDigest(id, digestString);
}
Digest FileEntry::getDigest(uint32 id) const {
  return impl->getDigest(id);
}
bool FileEntry::operator< (const FileEntry& that) const {
  return impl->operator< (*that.impl);
}


