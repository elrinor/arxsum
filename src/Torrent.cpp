#include "config.h"
#include "Torrent.h"
#include <string>
#include <boost/shared_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include "libtorrent/include/bencode.hpp"
#include "arx/Exception.h"
#include "arx/Collections.h"
#include "arx/Streams.h"
#include "arx/Converter.h"
#include "Hash.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace arx;
using namespace libtorrent;

class MemBuffer {
private:
  static const unsigned int align = 1024; // TODO: that's a hack, damn!
  unsigned int pos;
  unsigned int size;
  unsigned int maxSize;
  unsigned char *buf, *realBuf;

  MemBuffer& operator= (const MemBuffer& that);
  MemBuffer(const MemBuffer& that);

public:
  MemBuffer(unsigned int size) {
    this->pos = 0;
    this->size = 0;
    this->maxSize = size;
    this->realBuf = new unsigned char[this->maxSize + this->align];
    this->buf = (unsigned char*) (((intptr_t) this->realBuf + this->align - 1) & (-this->align));
  }

  ~MemBuffer() {
    delete[] this->realBuf;
  }

  int updateFromStream(InputStream stream) {
    unsigned int read = stream.read(this->buf + this->size, this->maxSize - this->size);
    if(read == EOF)
      return EOF;
    this->size = read;
    return read;
  }

  int fillFromStream(InputStream stream) {
    unsigned int oldSize = this->size;
    unsigned int read;
    while(true) {
      read = stream.read(this->buf + this->size, this->maxSize - this->size);
      if(read == EOF || this->size == this->maxSize)
        break;
      this->size += read;
    }
    return (read == EOF) ? EOF : (this->size - oldSize);
  }

  const unsigned char* getBuf() const {
    return this->buf;
  }

  const unsigned char* getBufEnd() const {
    return this->buf + this->size;
  }

  unsigned int getPos() const {
    return this->pos;
  }

  void setPos(unsigned int pos) {
    this->pos = pos;
  }

  void advancePos(unsigned int dpos) {
    this->pos += dpos;
  }

  unsigned int getSize() const {
    return this->size;
  }

  unsigned int getLeft() const {
    return this->size - this->pos;
  }

  void reset() {
    this->pos = 0;
    this->size = 0;
  }
};

ArrayList<CheckError> errorList(CheckError error) {
  ArrayList<CheckError> result;
  result.push_back(error);
  return result;
}

void checkTorrent(wpath torrentFile, boost::shared_ptr<CheckResultReporter> reporter) {
  wpath dir = torrentFile.branch_path();
  if(dir == _T(""))
    dir = _T(".");

  string pieces;
  unsigned int pieceLen;
  ArrayList<wstring> files;
  ArrayList<uint64> fileSizes;

  reporter->begin(torrentFile);

  try {
    if(!exists(torrentFile)) {
      reporter->error(CheckError(CE_NOACCESS));
      reporter->end();
      return;
    }

    unsigned long long torrentSizeL = file_size(torrentFile);
    if(torrentSizeL > MAX_TORRENT_FILE_SIZE) {
      reporter->error(_T("Torrent file too big: ") + torrentFile.native_file_string());
      reporter->end();
      return;
    }

    MemBuffer torrentFileBuf((unsigned int) torrentSizeL);
    torrentFileBuf.fillFromStream(FileInputStream(torrentFile.native_file_string(), OM_SEQUENTIAL_SCAN));
    if(torrentFileBuf.getSize() != torrentSizeL) {
      reporter->error(_T("Error while reading torrent file: ") + torrentFile.native_file_string());
      reporter->end();
      return;
    }

    entry infoEntry = bdecode(torrentFileBuf.getBuf(), torrentFileBuf.getBufEnd())["info"];
    //entry::dictionary_type info = infoEntry.dict();

    pieceLen = (unsigned int) infoEntry["piece length"].integer();
    pieces = infoEntry["pieces"].string();

    bool singleFileMode = false;
    if(infoEntry.find_key("files") == NULL)
      singleFileMode = true;

    if(singleFileMode) {
      files.push_back(toWideString(infoEntry["name"].string(), "utf-8"));
      fileSizes.push_back(infoEntry["length"].integer());
    } else {
      entry::list_type fileList = infoEntry["files"].list();
      for(entry::list_type::iterator i = fileList.begin(); i != fileList.end(); i++) {
        std::wstring path;
        entry::list_type pathList = (*i)["path"].list();
        for(entry::list_type::iterator j = pathList.begin(); j != pathList.end(); j++)
          path += toWideString((*j).string(), "utf-8") + _T("/");
        files.push_back(path.substr(0, path.size() - 1));
        fileSizes.push_back((*i)["length"].integer());
      }
    }
  } catch (...) {
    reporter->error(CheckError(CE_WRONGFORMAT, 0));
    reporter->end();
    return;
  }

  if(files.empty()) {
    reporter->end();
    return;
  }

  int pieceIndex = 0;
  int fileIndex = -1;

  MemBuffer readBuf(FILEBUFSIZE);
  InputStream stream;
  bool hadFails = false;
  bool firstChunk = false;
  bool starting = true;

  Hash sha1hash = Hash(H_SHA1, 0);
  uint32 hashed = 0;

  ArrayList<wstring> currentFiles;
  ArrayList<wpath> currentFilePaths;

  unsigned int read = 0;

  while(true) {
    if(starting || read == EOF) {
      starting = false;

      fileIndex++;
      if(fileIndex == files.size())
        break;

      try {
        stream = FileInputStream(files[fileIndex], OM_SEQUENTIAL_SCAN | OM_UNBUFFERED);
      } catch (...) {
        reporter->beginFile(wpath(), files[fileIndex]);
        reporter->endFile(errorList(CheckError(CE_NOACCESS)));
        reporter->error(CheckError(CE_TOOMANYERRORS));
        reporter->end();
        return;
      }

      if(!exists(files[fileIndex])) {
        reporter->beginFile(wpath(), files[fileIndex]);
        reporter->endFile(errorList(CheckError(CE_NOACCESS)));
        reporter->error(CheckError(CE_TOOMANYERRORS));
        reporter->end();
        return;
      }

      wpath filePath = wpath(files[fileIndex]).is_complete() ? files[fileIndex] : (dir / files[fileIndex]);
      uint64 realFileSize;
      try {
        realFileSize = file_size(filePath);
      } catch (...) {
        realFileSize = (uint64) -1;
      }
      
      if(realFileSize != fileSizes[fileIndex]) {
        reporter->beginFile(wpath(), files[fileIndex]);
        reporter->endFile(errorList(CheckError(CE_WRONGSIZE, fileSizes[fileIndex], realFileSize)));
        reporter->error(CheckError(CE_TOOMANYERRORS));
        reporter->end();
        return;
      }

      if(currentFiles.empty())
        reporter->beginFile(filePath, files[fileIndex]);

      currentFiles.push_back(files[fileIndex]);
      currentFilePaths.push_back(filePath);

      if(hashed != 0)
        firstChunk = true;
    }
    
    read = 0;
    if(readBuf.getLeft() == 0) {
      readBuf.reset();
      read = readBuf.updateFromStream(stream);
      if(read != EOF)
        reporter->update(read);
    }

    uint32 toHash = min(readBuf.getLeft(), pieceLen - hashed);
    sha1hash.update(readBuf.getBuf() + readBuf.getPos(), toHash);

    hashed += toHash;
    readBuf.advancePos(toHash);

    if((read == EOF && fileIndex == files.size() - 1) || hashed == pieceLen) {
      Digest chunkDigest = sha1hash.finalize();
      Digest realDigest((const byte*) pieces.c_str() + (pieceIndex++) * 20, 20);
      sha1hash = Hash(H_SHA1, 0);

      if((read == EOF && fileIndex == files.size() - 1) || (read == EOF && hashed == pieceLen) || firstChunk) {
        bool currentHadFails = hadFails || (chunkDigest != realDigest);
        if(currentHadFails)
          reporter->endFile(errorList(CheckError(CE_WRONGHASH, H_SHA1, Digest(), Digest())));
        else
          reporter->endFile(ArrayList<CheckError>());

        int32 sizeDec = ((read == EOF && hashed == pieceLen) || (read == EOF && fileIndex == files.size() - 1)) ? 0 : 1;

        hadFails = chunkDigest != realDigest;
        for(uint32 i = 1; i < currentFiles.size() - sizeDec; i++) { // ? -1
          reporter->beginFile(currentFilePaths[i], currentFiles[i]);
          if(hadFails)
            reporter->endFile(errorList(CheckError(CE_WRONGHASH, H_SHA1, Digest(), Digest())));
          else
            reporter->endFile(ArrayList<CheckError>());
        }
        firstChunk = false;

        currentFiles.erase(0, currentFiles.size() - sizeDec);
        currentFilePaths.erase(0, currentFilePaths.size() - sizeDec);

        if(!currentFiles.empty())
          reporter->beginFile(currentFilePaths[0], currentFiles[0]);
      }

      if(chunkDigest != realDigest)
        hadFails = true;
      hashed = 0;
    }
  }

  reporter->end();
}