#include "config.h"
#include "Task.h"
#include "Output.h"
#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
using namespace boost::algorithm;

// -------------------------------------------------------------------------- //
// HashTask implementation
// -------------------------------------------------------------------------- //
class HashTask::HashTaskImpl {
public:
	array<bool, H_COUNT> task;
};

HashTask::HashTask(): impl(new HashTaskImpl()) {
	FOREACH(bool& b, (impl->task))
		b = false;
}

void HashTask::add(uint32 id) {
	impl->task[id] = true;
}

void HashTask::remove(uint32 id) {
	impl->task[id] = false;
}

bool HashTask::isSet(uint32 id) {
	return impl->task[id];
}

size_t HashTask::size() {
	return impl->task.size();
}

ArrayList<Hash> HashTask::createHashList(uint64 totalLength) {
	ArrayList<Hash> result;
	for(uint32 i = 0; i < size(); i++)
		if(isSet(i))
			result.add(Hash(i, totalLength));
	return result;
}


// ------------------------------------------------------------------------- //
// OutputTask implementation
// ------------------------------------------------------------------------- //
class OutputTask::OutputTaskImpl {
public:
	array<bool, O_COUNT> task;
	array<string, O_COUNT> fileNames;
};

OutputTask::OutputTask(): impl(new OutputTaskImpl()) {
	FOREACH(bool& b, (impl->task))
		b = false;
}

void OutputTask::remove(uint32 id) {
	impl->task[id] = false;
}

bool OutputTask::isSet(uint32 id) {
	return impl->task[id];
}

size_t OutputTask::size() {
	return impl->task.size();
}

string OutputTask::getFileName(uint32 id) {
	return impl->fileNames[id];
}

void OutputTask::add(uint32 id, std::string fileName) {
	impl->task[id] = true;
	impl->fileNames[id] = fileName;
}

void OutputTask::perform(HashTask task, ArrayList<FileEntry> data) {
	if(data.empty())
		return;
	bool needsName = false;
	string starFileName;
	for(uint32 i = 0; i < size(); i++) {
		if(getFileName(i) == "*") {
			needsName = true;
			break;
		}
	}
	if(needsName) {
		string lastLeaf = data[0].getPath().leaf();
		size_t nameBegin = 0;
		size_t nameEnd = lastLeaf.size();
		for(size_t i = 1; i < data.size(); i++) {
			string leaf = data[i].getPath().leaf();
			nameEnd = min(nameEnd, leaf.size());
			for(; nameEnd >= 0 && leaf[nameEnd] != lastLeaf[nameEnd]; nameEnd--);
			for(; nameBegin < leaf.size() && nameBegin < lastLeaf.size() && leaf[nameBegin] != lastLeaf[nameBegin]; nameBegin++);
			if(nameEnd <= nameBegin)
				break;
		}
		starFileName = lastLeaf.substr(nameBegin, nameEnd - nameBegin);
	}
	for(uint32 i = 0; i < size(); i++)
		if(isSet(i))
			OutputFormat(i).output(task, data, createWriter((getFileName(i) == "*") ? (starFileName + "." + to_lower_copy(OutputFormat::getName(i))) : (getFileName(i))));
}

