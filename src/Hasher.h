#ifndef __HASHER_H__
#define __HASHER_H__

#include "config.h"
#include "Task.h"
#include "FileEntry.h"
#include "Streams.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/timer.hpp>
#include <boost/filesystem.hpp>

namespace __hasher {
	class HasherImpl;
}

class HasherCallBack {
public:
	virtual void operator() (uint64 justProcessed) = 0;
};

class StreamOutCallBack: public HasherCallBack {
private:
	uint64 sumSize;
	uint64 processed;
	uint64 lastProcessed;
	boost::timer t;
	double lastOutputTime;
	Writer writer;
public:
	StreamOutCallBack(uint64 sumSize, Writer writer): sumSize(sumSize), processed(0), lastProcessed(0), lastOutputTime(0.0), writer(writer) {}
	void operator() (uint64 justProcessed);
	void clear();
};

class Hasher: private boost::noncopyable {
private:
	boost::shared_ptr<__hasher::HasherImpl> impl;
public:
	Hasher(HashTask task, bool isMultiThreaded, boost::shared_ptr<HasherCallBack> callBack = boost::shared_ptr<HasherCallBack>());
	void hash(FileEntry entry);
	static std::string hash(uint32 hashId, boost::filesystem::path filePath, boost::shared_ptr<HasherCallBack> callBack);
};

#endif
