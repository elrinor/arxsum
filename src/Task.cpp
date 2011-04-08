#include "config.h"
#include "Task.h"
#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include "arx/Converter.h"
#include "Output.h"
#include "ArrayList.h"
#include "Streams.h"
#include "Options.h"

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace arx;

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
  array<wstring, O_COUNT> fileNames;
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

wstring OutputTask::getFileName(uint32 id) {
  return impl->fileNames[id];
}

void OutputTask::add(uint32 id, std::wstring fileName) {
  impl->task[id] = true;
  impl->fileNames[id] = fileName;
}

void OutputTask::perform(HashTask task, ArrayList<FileEntry> data) {
  if(data.empty())
    return;
  bool needsName = false;
  wstring starFileName;
  for(uint32 i = 0; i < size(); i++) {
    if(getFileName(i) == _T("*")) {
      needsName = true;
      break;
    }
  }
  if(needsName) {
    wstring lastLeaf = data[0].getPath().filename().wstring();
    size_t nameLen = lastLeaf.size();
    for(size_t i = 1; i < data.size(); i++) {
      wstring leaf = data[i].getPath().filename().wstring();
      nameLen = min(nameLen, leaf.size());
      size_t newNameLen;
      for(newNameLen = 0; newNameLen < nameLen; newNameLen++)
        if(leaf[newNameLen] != lastLeaf[newNameLen])
          break;
      nameLen = min(nameLen, newNameLen);
    }
    starFileName = trim_copy(lastLeaf.substr(0, nameLen));
    if(starFileName.empty())
      starFileName = _T("sum");
  }
  for(uint32 i = 0; i < size(); i++)
    if(isSet(i)) {
      Writer writer;
      try {
        if(getFileName(i) == _T("*"))
          writer = createWriter(starFileName + _T(".") + toWideString(to_lower_copy(OutputFormat::getName(i))), options.getOutputEncoding());
        else 
          writer = createWriter(getFileName(i), options.getOutputEncoding());
      } catch (...) {
        continue;
      }
      Printer printer(writer);
      OutputFormat(i).output(task, data, &printer);
    }
}

// -------------------------------------------------------------------------- //
// CheckTask
// -------------------------------------------------------------------------- //
class CheckTask::CheckTaskImpl {
public:
  HashTask hashTask;
  FileEntry rightEntry;
  bool sizeNeedsChecking;

  CheckTaskImpl(std::wstring fileName): rightEntry(fileName) {
    return;
  }
};

CheckTask::CheckTask() {
  return; // we don't initialize impl here to avoid accidental misuse of this constructor
}

CheckTask::CheckTask(std::wstring fileName): impl(new CheckTaskImpl(fileName)) {
  this->impl->sizeNeedsChecking = false;
}

HashTask CheckTask::getHashTask() {
  return this->impl->hashTask;
}

FileEntry CheckTask::getRightEntry() {
  return this->impl->rightEntry;
}

bool CheckTask::isSizeNeedsChecking() {
  return this->impl->sizeNeedsChecking;
}

void CheckTask::setSizeNeedsChecking(bool value) {
  this->impl->sizeNeedsChecking = value;
}
