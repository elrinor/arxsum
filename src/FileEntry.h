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
	FileEntry(boost::filesystem::path name);
	FileEntry(std::string name);
	FileEntry(char* name);
	bool isFailed() const;
	void setFailed(bool failed);
	boost::filesystem::path getPath() const;
	void setSize(uint64 size);
	uint64 getSize() const;
	void setDateTime(time_t dateTime);
	time_t getDateTime() const;
	void setDigest(uint32 id, std::string digestString);
	std::string getDigest(uint32 id) const;
	bool operator< (const FileEntry& that) const;
};

#endif
