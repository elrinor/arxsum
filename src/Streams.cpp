#include "config.h"
#include "Streams.h"

#ifdef USE_WIN32_CHUNKINPUTSTREAM
#  include <Windows.h>
#endif

using namespace boost;
using namespace boost::filesystem;
using namespace std;

template<class FileStream>
class FileStreamDeleter{
public:
	void operator() (FileStream* p) {
		if(p->is_open())
			p->close();
		delete p;
	}
};

template<class T>
class EmptyDeleter{
public:
	void operator() (T* p) {
		return;
	}
};

InputStream createI(path fileName, ios_base::openmode openMode) {
	const static shared_ptr<istream> pCin(&cin, EmptyDeleter<istream>());
	if(fileName.native_file_string() == "-")
		return pCin;
	else
		return shared_ptr<istream>(new ifstream(fileName.native_file_string().c_str(), openMode), FileStreamDeleter<ifstream>());
}

InputStream createInputStream(path fileName) {
	return createI(fileName, ios_base::in | ios_base::binary);
}

Reader createReader(path fileName) {
		return createI(fileName, ios_base::in);
}

OutputStream createO(path fileName, ios_base::openmode openMode) {
	const static shared_ptr<ostream> pCout(&cout, EmptyDeleter<ostream>());
	if(fileName.native_file_string() == "-")
		return pCout;
	else
		return shared_ptr<ostream>(new ofstream(fileName.native_file_string().c_str(), openMode), FileStreamDeleter<ofstream>());
}

OutputStream createOutputStream(path fileName) {
	return createO(fileName, ios_base::out | ios_base::binary);
}

Writer createWriter(boost::filesystem::path fileName) {
	return createO(fileName, ios_base::out);
}

namespace streams {
	class ChunkInputStreamFactory {
	public:
		static ChunkInputStream create(boost::filesystem::path fileName) {
			if(fileName == "-")
				return ChunkInputStream(fileName, CONIOBUFSIZE);
			else
				return ChunkInputStream(fileName, FILEBUFSIZE);
		}
	};
};

ChunkInputStream createChunkInputStream(boost::filesystem::path fileName) {
	return streams::ChunkInputStreamFactory::create(fileName);
}

ChunkInputStream::ChunkInputStream(path fileName, size_t chunkSize) {
	this->chunkSize = chunkSize;
#ifdef USE_WIN32_CHUNKINPUTSTREAM
	this->isEof = false;
	if(fileName == "-")
		this->fileHandle = GetStdHandle(STD_INPUT_HANDLE);
	else
		this->fileHandle = CreateFile(fileName.string().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#else
	this->stream = createInputStream(fileName);
#endif
}

uint32 ChunkInputStream::read(byte* buf, size_t bufSize) {
#ifdef USE_WIN32_CHUNKINPUTSTREAM
	DWORD n;
	BOOL bResult = ReadFile(this->fileHandle, buf, (DWORD) min(bufSize, this->chunkSize), &n, NULL);
	if(n <= 0 && bResult != 0)
		this->isEof = true;
	if(bResult == 0)
		this->fileHandle = INVALID_HANDLE_VALUE;
	return n;
#else
	stream->read((char*) buf, (streamsize) min(bufSize, this->chunkSize));
	return stream->gcount();
#endif
}

bool ChunkInputStream::eof() {
#ifdef USE_WIN32_CHUNKINPUTSTREAM
	return this->isEof;
#else
	return stream->eof();
#endif
}

bool ChunkInputStream::fail() {
#ifdef USE_WIN32_CHUNKINPUTSTREAM
	return (this->fileHandle == INVALID_HANDLE_VALUE);
#else
	return stream->fail();
#endif
}


#if 0

// -------------------------------------------------------------------------- //
// InputStream
// -------------------------------------------------------------------------- //
unsigned int InputStream::read(unsigned char* buf, unsigned int len) {
	unsigned int totalRead = 0;
	int byte = 0;
	while(totalRead < len && (byte = read()) != -1) {
		*buf = static_cast<unsigned char>(byte);
		buf++;
		totalRead++;
	}
	return static_cast<unsigned int>(totalRead);
}


unsigned long long InputStream::skip(unsigned long long n) {
	static const unsigned int MAX_BUF_SIZE = 1024 * 1024;
	unsigned long long totalRead = 0;
	unsigned int chunkSize = (n > MAX_BUF_SIZE) ? MAX_BUF_SIZE : n;
	unsigned char* buf = new unsigned char[chunkSize];
	unsigned int result;
	while(totalRead < n && (result = read(buf, chunkSize)) != -1)
		totalRead += result;
	delete[] buf;
	return totalRead;
}

void InputStream::close() {
	return;
}


// -------------------------------------------------------------------------- //
// OutputStream
// -------------------------------------------------------------------------- //
void OutputStream::write(unsigned char* buf, unsigned int len) {
	while(len-- > 0)
		write(static_cast<int>(*buf++));
}

void OutputStream::flush() {
	return;
}

void OutputStream::close() {
	return;
}

#endif