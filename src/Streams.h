#ifndef __STREAMS_H__
#define __STREAMS_H__

#include "config.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

// The Basic Idea:
// Streams operate with binary data.
// Readers&Writers operate with character data.

typedef boost::shared_ptr<std::istream> InputStream;
typedef boost::shared_ptr<std::ostream> OutputStream;
typedef boost::shared_ptr<std::istream> Reader;
typedef boost::shared_ptr<std::ostream> Writer;

namespace streams {
	class ChunkInputStreamFactory;
}

class ChunkInputStream {
private:
#ifdef USE_WIN32_CHUNKINPUTSTREAM
	void* fileHandle; // HANDLE == void*
	bool isEof;
#else
	InputStream stream;
#endif
	size_t chunkSize;
	ChunkInputStream(boost::filesystem::path fileName, size_t chunkSize);
	friend class streams::ChunkInputStreamFactory;
public:
	uint32 read(byte* buf, size_t bufSize);
	bool eof();
	bool fail();
};


InputStream createInputStream(boost::filesystem::path fileName);

OutputStream createOutputStream(boost::filesystem::path fileName);

Reader createReader(boost::filesystem::path fileName);

Writer createWriter(boost::filesystem::path fileName);

ChunkInputStream createChunkInputStream(boost::filesystem::path fileName);

#if 0
namespace arx {

	class InputStream() {
	public:
		/**
		 * Read one byte from the stream. 
		 * @returns the next byte of data, or -1 if the end of stream was reached.
		 */
		virtual int read() = 0;

		/**
		 * Read up to len bytes and store them into the buffer buf.
		 * The read(unsigned char*, unsigned int) method for class InputStream simply calls the method read() repeatedly. The subclasses are encouraged to 
		 * provide a more efficient implementation.
		 * @param buf the buffer to read bytes to
		 * @param len the length of the buffer
		 * @returns the total number of bytes read, or -1 if the end of file was reached
		 */
		virtual int read(unsigned char* buf, unsigned int len);

		/**
		 * Skip up to n bytes. The actual number of bytes skipped can be less than n for a variety of reasons, for example because of reaching the end of file.
		 * The skip(unsigned long long) method for class InputStream is implemented using read(unsigned char*, unsigned int) and a temporary buffer. The subclasses are encouraged to 
		 * provide a more efficient implementation.
		 * @param n the number of bytes to be skipped
		 * @returns the number of actually skipped bytes. The negative value indicated, that no bytes were skipped.
		 */
		virtual unsigned long long skip(unsigned long long n);

		/**
		* Close the stream and release all the system resources associated with this stream. No more reading will be possible.
		* The close() method for class InputStream does nothing.
		*/
		virtual void close();
	};

	class OutputStream {
	public:
		/**
		 * Writes one byte to the stream. The byte to be written is the eight low-order bits of argument byte.
		 * @param byte the byte to be written
		 */
		virtual void write(int byte) = 0;

		/**
		 * Writes len bytes from the buffer buf to the stream.
		 * The write(unsigned char*, unsigned int) method for class OutputStream simply calls the method write(int) repeatedly. The subclasses are encouraged to 
		 * provide a more efficient implementation.
		 * @param buf the data buffer
		 * @param len length of the data buffer
		 */
		virtual void write(unsigned char* buf, unsigned int len);

		/**
		 * Flush the stream and force any output bytes to be written out.
		 * The flush() method for class OutputStream does nothing.
		 */
		virtual void flush();

		/**
		* Close the stream and release all the system resources associated with this stream. No more writing will be possible.
		* The close() method for class OutputStream does nothing.
		*/
		virtual void close();
	};

	namespace detail {
		class FileInputStreamImpl;
	}

	class FileInputStream {
	private:
		detail::FileInputStreamImpl impl;
	public:
		void close() {
			impl->close();
		}
	};
}

#endif


#endif
