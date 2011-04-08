#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include "config.h"
#include <boost/shared_ptr.hpp>
#include <string>
#include "Streams.h"
#include "Collections.h"
#include "FileEntry.h"
#include "Task.h"

enum {
	O_SFV,
	O_MD5,
	O_ED2K,
	O_COUNT,
	O_UNKNOWN = -1
};

extern const uint32 outTaskRequirement[];

namespace __output {
	class OutputFormatImpl;
};

class OutputFormat {
private:
	boost::shared_ptr<__output::OutputFormatImpl> impl;
	uint32 ofId;
public:
	OutputFormat(uint32 ofId);
	void output(HashTask task, ArrayList<FileEntry> data, Writer stream);
	std::string getName();
	uint32 getId();
	static std::string getName(uint32 ofId);
};

#endif