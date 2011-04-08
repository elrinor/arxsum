#include "config.h"
#include "Hasher.h"
#include "Hash.h"
#include "FileEntry.h"
#include "Streams.h"
#include "Task.h"
#include "arx/Exception.h"
#include <boost/array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace arx;

array<unsigned char, FILEBUFSIZE> buf, buf_;

namespace detail {
  class HasherImpl {
  protected:
    HashTask task;
    shared_ptr<HasherCallBack> callBack;
    void doCallBack(uint64 justProcessed) {
      if(callBack)
        callBack->operator() (justProcessed);
    }
  public:
    HasherImpl(HashTask task, shared_ptr<HasherCallBack> callBack) : task(task), callBack(callBack) {}
    virtual void hash(FileEntry entry) = 0;
  };

  class SimpleHasher: public HasherImpl {
  private:
  public:
    SimpleHasher(HashTask task, shared_ptr<HasherCallBack> callBack): HasherImpl(task, callBack) {}
    void hash(FileEntry entry) {
      InputStream stream;
      try {
        stream = createInputStream(entry.getPath(), true);
      } catch(...) {
        entry.setFailed(true);
        return;
      }
      ArrayList<Hash> hashList = task.createHashList(entry.getSize());
      uint64 totalRead = 0;
      doCallBack(0);
      uint32 read;
      while((read = stream.read(buf.c_array(), (unsigned int) buf.size())) != EOF) {
        totalRead += read;
        doCallBack(read);
        FOREACH(Hash hash, hashList)
          hash.update(buf.c_array(), read);
      }
      if(totalRead < entry.getSize())
        doCallBack(entry.getSize() - totalRead);
      entry.setSize(totalRead);
      FOREACH(Hash hash, hashList)
        entry.setDigest(hash.getId(), hash.finalize());
    }
  };

  class MultiThreadedHasher: public HasherImpl {
  private:
    bool fileEnd;
    bool terminating;
    unsigned char* bufToHash;
    uint32 bufToHashSize;
    barrier* beginBarrier;
    barrier* readBarrier;
    barrier* hashBarrier;
    FileEntry currentEntry;

    class ThreadClass {
    private:
      MultiThreadedHasher* owner;
      uint32 hashId;
    public:
      ThreadClass(MultiThreadedHasher* owner, uint32 hashId): owner(owner), hashId(hashId) {}
      void operator() () {
        while(true) {
          owner->beginBarrier->wait(); // wait for new file
          if(owner->terminating)
            return;
          Hash hash(this->hashId, owner->currentEntry.getSize());
          owner->hashBarrier->wait(); // let owner pass
          while(!owner->fileEnd) {
            owner->readBarrier->wait(); // wait for owner to read
            hash.update(owner->bufToHash, owner->bufToHashSize);
            owner->hashBarrier->wait(); // let owner pass
          }
          owner->currentEntry.setDigest(hash.getId(), hash.finalize());
          owner->beginBarrier->wait(); // notify owner about end of work
        }
      }
    };

    thread_group threadGroup;
    ArrayList<ThreadClass> threads;

  public:
    MultiThreadedHasher(HashTask task, shared_ptr<HasherCallBack> callBack): HasherImpl(task, callBack) {
      this->terminating = false;
      uint32 numThreads = 0;
      for(uint32 i = 0; i < task.size(); i++) {
        if(task.isSet(i)) {
          numThreads++;
          this->threads.add(ThreadClass(this, i));
        }
      }
      this->beginBarrier = new barrier(numThreads + 1);
      this->readBarrier = new barrier(numThreads + 1);
      this->hashBarrier = new barrier(numThreads + 1);
      FOREACH(ThreadClass& singleThread, threads)
        this->threadGroup.create_thread(bind(&ThreadClass::operator(), &singleThread));
    }
    void hash(FileEntry entry) {
      InputStream stream;
      try {
        stream = createInputStream(entry.getPath(), true);
      } catch(...) {
        entry.setFailed(true);
        return;
      }
      this->fileEnd = false;
      this->currentEntry = entry;
      unsigned char* buf0 = buf.c_array();
      unsigned char* buf1 = buf_.c_array();
      size_t bufSize = buf.size();
      this->beginBarrier->wait(); // start hash thread execution
  
      uint64 totalRead = 0;
      doCallBack(0);
      uint32 read;
      while((read = stream.read(buf0, (unsigned int) bufSize)) != EOF) {
        totalRead += read;
        doCallBack(read);
        this->hashBarrier->wait(); // wait for end of hashing
        this->bufToHash = buf0;
        this->bufToHashSize = read;
        this->readBarrier->wait(); // start hashing of a new chunk
        swap(buf0, buf1);
      }
      fileEnd = true;
      this->hashBarrier->wait(); // notify children about file end
      if(totalRead < entry.getSize())
        doCallBack(entry.getSize() - totalRead);
      entry.setSize(totalRead);
      this->beginBarrier->wait(); // wait for children to fill up digest fields
    }
  };
};

Hasher::Hasher(HashTask task, bool isMultiThreaded, shared_ptr<HasherCallBack> callBack) {
  if(isMultiThreaded)
    impl.reset(new ::detail::MultiThreadedHasher(task, callBack));
  else
    impl.reset(new ::detail::SimpleHasher(task, callBack));
}

void Hasher::hash(FileEntry entry) {
  impl->hash(entry);
}

Digest Hasher::hash(uint32 hashId, wpath filePath, shared_ptr<HasherCallBack> callBack) {
  HashTask task;
  task.add(hashId);
  ::detail::SimpleHasher hasher(task, callBack);
  FileEntry entry(filePath);
  entry.setSize(file_size(filePath));
  hasher.hash(entry);
  if(entry.isFailed())
    throw wruntime_error(_T("could not open file ") + filePath.string());
  return entry.getDigest(hashId);
}

void StreamOutHasherCallBack::operator() (uint64 justProcessed) {
  processed += justProcessed;
  int progress;
  if(sumSize == 0)
    progress = 100;
  else
    progress = (int) (100 * processed / sumSize);
  *printer << "\rProgress: " << setw(3) << progress << "%";
  double currentTime = t.elapsed();
  if(currentTime - lastOutputTime > OUTPUTPERIOD) {
    double speed = (processed - lastProcessed) / (currentTime - lastOutputTime);
    lastOutputTime = currentTime;
    lastProcessed = processed;
    if(speed != 0.0)
      *printer << "   Speed: " << fixed << showpoint << setprecision(3) << speed / (1024 * 1024) << "Mbps       ";
  }
}

void StreamOutHasherCallBack::clear() {
  *printer << "\r                                        \r";
}



