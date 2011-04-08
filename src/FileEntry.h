#ifndef __FILEENTRY_H__
#define __FILEENTRY_H__

#include "config.h"
#include <string>
#include <boost/filesystem.hpp>
#include <boost/array.hpp>
#include "Hash.h"

class FileEntry {
private:
  class FileEntryImpl;
  boost::shared_ptr<FileEntryImpl> impl;
public:
  FileEntry() {};
  explicit FileEntry(boost::filesystem::wpath name);
  explicit FileEntry(std::wstring name);
  explicit FileEntry(wchar_t* name);

  bool isFailed() const;
  boost::filesystem::wpath getPath() const;
  uint64 getSize() const;
  time_t getDateTime() const;
  Digest getDigest(uint32 id) const;

  void setFailed(bool failed);
  void setSize(uint64 size);
  void setDateTime(time_t dateTime);
  void setDigest(uint32 id, Digest digest);

  bool operator< (const FileEntry& that) const;
};

#endif
