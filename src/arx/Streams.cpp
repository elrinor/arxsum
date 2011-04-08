#include "config.h"
#include "Streams.h"

#ifdef ARX_WIN
#  include <Windows.h>
#  undef min
#  undef max
#endif

#ifdef ARX_LINUX
#  define _FILE_OFFSET_BITS 64
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#endif

#include "Converter.h"
#include <algorithm>
#include <limits>

#ifdef _DEBUG
template<class A, class B> 
A dynamic_cast_dbg(B b) {
	A a = dynamic_cast<A>(b);
	return a;
}
#  define dynamic_cast dynamic_cast_dbg
#endif

using namespace std;

namespace arx {

	namespace detail {

// -------------------------------------------------------------------------- //
// Supplementary
// -------------------------------------------------------------------------- //
		template<class charT, class streamT> 
		int read(streamT* stream, charT* buf, unsigned int len) {
			unsigned int totalRead = 0;
			int character = stream->read();
			if(character == EOF)
				return EOF;
			while(totalRead < len && character != EOF) {
				*buf = static_cast<charT>(character);
				buf++;
				totalRead++;
				character = stream->read();
			}
			return static_cast<int>(totalRead);
		}

		template<class charT, class streamT> 
		int read(streamT* stream) {
			charT c;
			int result = stream->read(&c, 1);
			if(result == EOF)
				return EOF;
			return static_cast<int>(c);
		}

		template<class charT, class streamT> 
		long long skip(streamT* stream, long long n) {
			static const unsigned int MAX_BUF_SIZE = 1024 * 1024;
			long long totalRead = 0;
			unsigned int chunkSize = static_cast<unsigned int>((n > MAX_BUF_SIZE) ? MAX_BUF_SIZE : n);
			charT* buf = new charT[chunkSize];
			unsigned int result;
			while(totalRead < n && (result = stream->read(buf, chunkSize)) != -1)
				totalRead += result;
			delete[] buf;
			return totalRead;
		}

		template<class charT, class streamT> 
		void write(streamT* stream, int character) {
			charT c = static_cast<charT>(character);
			stream->write(&c, 1);
		}

		template<class charT, class streamT> 
		void write(streamT* stream, const charT* buf, unsigned int len) {
			while(len-- > 0)
				stream->write(static_cast<int>(*buf++));
		}

		bool isUTF16(const char* encoding) {
			const string s(encoding);
			if(s == "UTF-16" || s == "UTF16" || s == "UCS-2" || s == "UCS2")
				return true;
			return false;
		}

		bool isLineEnding(wchar_t c) {
			/*
			TODO:
			LF:    Line Feed, U+000A
			CR:    Carriage Return, U+000D
			CR+LF: CR followed by LF, U+000D followed by U+000A
			NEL:   Next Line, U+0085
			FF:    Form Feed, U+000C
			LS:    Line Separator, U+2028
			PS:    Paragraph Separator, U+2029
			*/
			if(c == L'\n' || c == L'\r')
				return true;
			return false;
		}

		bool isWhiteSpace(wchar_t c) {
			return isLineEnding(c) || c == L' ' || c == L'\t';
		}

		const wchar_t* getDefaultLineSeparator() {
#ifdef ARX_WIN
			return L"\r\n";
#endif
#ifdef ARX_LINUX
			return L"\n";
#endif
		}

		template<class charT, class readerImplT>
		unsigned int bufferedRead(readerImplT* reader, charT* dst, unsigned int len) {
			unsigned int initialLen = len;
			//while(len > 0) {
				if(reader->bufLeft() <= 0) {
					int status = reader->fillBuf();
					if(status == EOF) {
						if(initialLen == len)
							return EOF;
						return initialLen - len;
					}
				}
				if(reader->bufLeft() > 0) {
					unsigned int toMove = min(reader->bufLeft(), len);
					memcpy(dst, reader->getBuf() + reader->getBufPos(), toMove * sizeof(wchar_t));
					reader->setBufPos(reader->getBufPos() + toMove);
					len -= toMove;
					dst += toMove;
				}
			//}
			return initialLen - len;
		}

		class StreamManager {
		public:
			static InputStreamInterface* getImpl(InputStream stream) {
				return stream.getImpl();
			}

			static OutputStreamInterface* getImpl(OutputStream stream) {
				return stream.getImpl();
			}

			static ReaderInterface* getImpl(Reader reader) {
				return reader.getImpl();
			}

			static WriterInterface* getImpl(Writer writer) {
				return writer.getImpl();
			}
		};

// -------------------------------------------------------------------------- //
// InputStreamInterface
// -------------------------------------------------------------------------- //
		class InputStreamInterface {
		public:
			/**
			 * The read() method for class InputStreamInterface simply calls the method read(unsigned char*, unsigned int).
			 * Since the read(unsigned char*, unsigned int) method for class InputStreamInterface is implemented using a sequence of calls to read(), the 
			 * subclasses must provide an implementation for at least one of these methods.
			 */
			virtual int read() {
				return detail::read<unsigned char>(this);
			}
			
			/**
			* The read(unsigned char*, unsigned int) method for class InputStreamInterface simply calls the method read() repeatedly.
			* Since the read() method for class InputStreamInterface is implemented using a call to read(unsigned char*, unsigned int), the 
			* subclasses must provide an implementation for at least one of these methods.
			*/
			virtual int read(unsigned char* buf, unsigned int len) {
				return detail::read(this, buf, len);
			}
			
			/**
			 * The skip(long long) method for class InputStreamInterface creates a temporary buffer and then repeatedly reads into it until n bytes have been read or the end of 
			 * the stream has been reached. Subclasses are encouraged to provide a more efficient implementation.
			 */
			virtual long long skip(long long n) {
				return detail::skip<unsigned char>(this, n);
			}

			virtual void close() {
				return;
			}
		};

// -------------------------------------------------------------------------- //
// OutputStreamInterface
// -------------------------------------------------------------------------- //
		class OutputStreamInterface {
		public:
			/**
			 * The write(int) method for class OutputStreamInterface simply calls the method write(unsigned char*, unsigned int).
			 * Since the write(unsigned char*, unsigned int) method for class OutputStreamInterface is implemented using a sequence of calls to write(int), the 
			 * subclasses must provide an implementation for at least one of these methods.
			 */
			virtual void write(int byte) {
				detail::write<unsigned char>(this, byte);
			}

			/**
			* The write(unsigned char*, unsigned int) method for class OutputStreamInterface simply calls the method write(int) repeatedly.
			* Since the write(int) method for class OutputStreamInterface is implemented using a call to write(unsigned char*, unsigned int), the 
			* subclasses must provide an implementation for at least one of these methods.
			*/
			virtual void write(const unsigned char* buf, unsigned int len) {
				detail::write(this, buf, len);
			}

			virtual void flush() {
				return;
			}

			virtual void close() {
				return;
			}
		};
	
	} // namespace detail

// -------------------------------------------------------------------------- //
// InputStream
// -------------------------------------------------------------------------- //
	detail::InputStreamInterface* InputStream::getImpl() const {
		return this->impl.get();
	}

	void InputStream::setImpl(detail::InputStreamInterface* impl) {
		this->impl.reset(impl);
	}

	InputStream::InputStream() {
		return;
	}

	int InputStream::read() {
		return this->impl->read();
	}

	int InputStream::read(unsigned char* buf, unsigned int len) {
		return this->impl->read(buf, len);
	}

	long long InputStream::skip(long long n) {
		return this->impl->skip(n);
	}

	void InputStream::close() {
		this->impl->close();
	}


// -------------------------------------------------------------------------- //
// OutputStream
// -------------------------------------------------------------------------- //
	detail::OutputStreamInterface* OutputStream::getImpl() const {
		return this->impl.get();
	}

	void OutputStream::setImpl(detail::OutputStreamInterface* impl) {
		this->impl.reset(impl);
	}

	OutputStream::OutputStream() {
		return;
	}

	void OutputStream::write(int byte) {
		this->impl->write(byte);
	}

	void OutputStream::write(const unsigned char* buf, unsigned int len) {
		this->impl->write(buf, len);
	}

	void OutputStream::flush() {
		this->impl->flush();
	}

	void OutputStream::close() {
		this->impl->close();
	}


	namespace detail {
// -------------------------------------------------------------------------- //
// Convenience functions
// -------------------------------------------------------------------------- //
#ifdef ARX_WIN
		DWORD prepareFlags(FileStreamOpenMode openMode) {
			DWORD flags = FILE_ATTRIBUTE_NORMAL;
			if(openMode & OM_SEQUENTIAL_SCAN)
				flags |= FILE_FLAG_SEQUENTIAL_SCAN;
			if(openMode & OM_UNBUFFERED)
				flags |= FILE_FLAG_NO_BUFFERING;
			return flags;
		}

		void throwOnInvalidHandle(string fileName, HANDLE handle) {
			if(handle == INVALID_HANDLE_VALUE)
				throw new runtime_error("Could not open file \"" + fileName + "\".");
		}
#endif

// -------------------------------------------------------------------------- //
// FileInputStreamImpl
// -------------------------------------------------------------------------- //
		class FileInputStreamImpl: public InputStreamInterface {
#ifdef ARX_WIN
		private:
			HANDLE handle;

		protected:
			FileInputStreamImpl(HANDLE handle) {
				this->handle = handle;
			}

		public:
			FileInputStreamImpl(wstring fileName, FileStreamOpenMode openMode) {
				this->handle = CreateFileW(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, prepareFlags(openMode), NULL);
				throwOnInvalidHandle("", this->handle); // TODO
			}

			FileInputStreamImpl(string fileName, FileStreamOpenMode openMode) {
				this->handle = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, prepareFlags(openMode), NULL);
				throwOnInvalidHandle(fileName, this->handle);
			}

			int read(unsigned char* buf, unsigned int len) {
				DWORD n;
				BOOL bResult = ReadFile(this->handle, buf, len, &n, NULL);
				if(n <= 0 && bResult != 0)
					return EOF;
				if(bResult == 0)
					throw runtime_error("Error while reading file: ReadFile failed");
				return n;
			}

			// TODO:
			// long long skip(long long n);

			void close() {
				if(this->handle == INVALID_HANDLE_VALUE)
					return;
				if(!CloseHandle(this->handle))
					throw runtime_error("Error while closing file: CloseHandle failed");
			}
#endif // ARX_WIN
#ifdef ARX_LINUX
		private:
			int fd;
			
			init(string fileName, FileStreamOpenMode openMode) {
				int flags = O_LARGEFILE | O_RDONLY;
				if(openMode & OM_UNBUFFERED)
					flags |= O_DIRECT;
				this->fd = open(fileName.c_str(), flags);
				if(this->fd == -1)
					throw new runtime_error("Could not open file \"" + fileName + "\" for reading.").
			}

		protected:
			FileInputStreamImpl(int fd) {
				this->fd = fd;
			}

		public:

			FileInputStreamImpl(string fileName, FileStreamOpenMode openMode) {
				init(fileName, openMode);
			}

			FileInputStreamImpl(wstring fileName, FileStreamOpenMode openMode) {
				// TODO: is that ok?
				FromUTF16Converter conv;
				init(conv.convert(fileName).c_str(), openMode);
			}

			int read(unsigned char* buf, unsigned int len) {
				int result = read(this->fd, buf, len);
				if(result == 0)
					return EOF;
				if(result < 0)
					throw runtime_error("Error while reading file: read failed");
				return result;
			}

			// TODO:
			// long long skip(long long n);

			void close() {
				if(this->fd == -1)
					return;
				if(close(this->fd) != 0)
					throw runtime_error("Error while closing file: close failed");
			}
#endif // ARX_LINUX
		};

	} // namespace detail


// -------------------------------------------------------------------------- //
// FileInputStream
// -------------------------------------------------------------------------- //
	FileInputStream::FileInputStream() {
		return;
	}

	FileInputStream::FileInputStream(string fileName, FileStreamOpenMode openMode) {
		this->setImpl(new detail::FileInputStreamImpl(fileName, openMode));
	}

	FileInputStream::FileInputStream(wstring fileName, FileStreamOpenMode openMode) {
		this->setImpl(new detail::FileInputStreamImpl(fileName, openMode));
	}


// -------------------------------------------------------------------------- //
// FileOutputStreamImpl
// -------------------------------------------------------------------------- //
	namespace detail {
		class FileOutputStreamImpl: public OutputStreamInterface {
#ifdef ARX_WIN
		private:
			HANDLE handle;

		protected:
			FileOutputStreamImpl(HANDLE handle) {
				this->handle = handle;
			}

		public:

			FileOutputStreamImpl(wstring fileName, FileStreamOpenMode openMode) {
				this->handle = CreateFileW(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, prepareFlags(openMode), NULL);
				throwOnInvalidHandle("", this->handle); // TODO
			}

			FileOutputStreamImpl(string fileName, FileStreamOpenMode openMode) {
				this->handle = CreateFileA(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, prepareFlags(openMode), NULL);
				throwOnInvalidHandle(fileName, this->handle);
			}

			void write(const unsigned char* buf, unsigned int len) {
				DWORD written;
				do {
					BOOL status = WriteFile(this->handle, buf, len, &written, NULL);
					if(status == 0)
						throw runtime_error("Error while writing to file: WriteFile failed");
					len -= written;
					buf += written;
				} while(len != 0);
			}

			void flush() {
				BOOL status = FlushFileBuffers(this->handle);
				if(status == 0)
					throw runtime_error("Error while flushing file: FlushFileBuffers failed");
			}

			void close() {
				if(this->handle == INVALID_HANDLE_VALUE)
					return;
				if(!CloseHandle(this->handle))
					throw runtime_error("Error while closing file: CloseHandle failed");
			}

#endif // ARX_WIN
#ifdef ARX_LINUX
		private:
			int fd;

			init(string fileName, FileStreamOpenMode openMode) {
				int flags = O_LARGEFILE | O_WRONLY;
				if(openMode & OM_UNBUFFERED)
					flags |= O_DIRECT;
				this->fd = open(fileName.c_str(), flags);
				if(this->fd == -1)
					throw new runtime_error("Could not open file \"" + fileName + "\" for writing.").
			}

		protected:
			FileOutputStreamImpl(int fd) {
				this->fd = fd;
			}

		public:

			FileOutputStreamImpl(string fileName, FileStreamOpenMode openMode) {
				init(fileName, openMode);
			}

			FileOutputStreamImpl(wstring fileName, FileStreamOpenMode openMode) {
				// TODO: is that ok?
				FromUTF16Converter conv;
				init(conv.convert(fileName).c_str(), openMode);
			}

			void write(const unsigned char* buf, unsigned int len) {
				ssize_t write(int fd, const void *buf, size_t count)
				DWORD written;
				do {
					ssize_t written = write(this->handle, buf, len);
					if(written == -1)
						throw runtime_error("Error while writing to file: write failed");
					len -= written;
					buf += written;
				} while(len != 0);
			}

			void flush() {
				// TODO!!!
				return;
			}

			void close() {
				if(this->fd == -1)
					return;
				if(close(this->fd) != 0)
					throw runtime_error("Error while closing file: close failed");
			}
#endif // ARX_LINUX
		};

	} // namespace detail


// -------------------------------------------------------------------------- //
// FileOutputStream
// -------------------------------------------------------------------------- //
	FileOutputStream::FileOutputStream() {
		return;
	}

	FileOutputStream::FileOutputStream(string fileName, FileStreamOpenMode openMode) {
		this->setImpl(new detail::FileOutputStreamImpl(fileName, openMode));
	}

	FileOutputStream::FileOutputStream(wstring fileName, FileStreamOpenMode openMode) {
		this->setImpl(new detail::FileOutputStreamImpl(fileName, openMode));
	}

	namespace detail {
// -------------------------------------------------------------------------- //
// ConsoleOutputStreamImpl
// -------------------------------------------------------------------------- //
		class ConsoleOutputStreamImpl: public FileOutputStreamImpl {
		public:
#ifdef ARX_WIN
			ConsoleOutputStreamImpl(): FileOutputStreamImpl(GetStdHandle(STD_OUTPUT_HANDLE)) {
				return;
			}
#endif
#ifdef ARX_LINUX
			ConsoleOutputStreamImpl(): FileOutputStreamImpl(1) {
				return;
			}
#endif
		};

// -------------------------------------------------------------------------- //
// ConsoleInputStreamImpl
// -------------------------------------------------------------------------- //
		class ConsoleInputStreamImpl: public FileInputStreamImpl {
		public:
#ifdef ARX_WIN
			ConsoleInputStreamImpl(): FileInputStreamImpl(GetStdHandle(STD_INPUT_HANDLE)) {
				return;
			}
#endif
#ifdef ARX_LINUX
			ConsoleInputStreamImpl(): FileInputStreamImpl(0) {
				return;
			}
#endif
		};
	} // namespace detail

// -------------------------------------------------------------------------- //
// ConsoleOutputStream
// -------------------------------------------------------------------------- //
	ConsoleOutputStream::ConsoleOutputStream() {
		this->setImpl(new detail::ConsoleOutputStreamImpl());
	}

// -------------------------------------------------------------------------- //
// ConsoleInputStream
// -------------------------------------------------------------------------- //
	ConsoleInputStream::ConsoleInputStream() {
		this->setImpl(new detail::ConsoleInputStreamImpl());
	}


	namespace detail {
// -------------------------------------------------------------------------- //
// ReaderInterface 
// -------------------------------------------------------------------------- //
		class ReaderInterface {
		public:
			virtual int read() {
				return detail::read<wchar_t>(this);
			}

			virtual int read(wchar_t* buf, unsigned int len) {
				return detail::read(this, buf, len);
			}

			virtual long long skip(long long n) {
				return detail::skip<wchar_t>(this, n);
			}

			virtual void close() {
				return;
			}
		};


// -------------------------------------------------------------------------- //
// WriterInterface 
// -------------------------------------------------------------------------- //
		class WriterInterface {
		public:
			virtual void write(wchar_t character) {
				detail::write<wchar_t>(this, character);
			}

			virtual void write(const wchar_t* buf, unsigned int len) {
				detail::write(this, buf, len);
			}

			virtual void flush() {
				return;
			}

			virtual void close() {
				return;
			}
		};
	} // namespace detail


// -------------------------------------------------------------------------- //
// Reader 
// -------------------------------------------------------------------------- //
	detail::ReaderInterface* Reader::getImpl() const {
		return this->impl.get();
	}

	void Reader::setImpl(detail::ReaderInterface* impl) {
		this->impl.reset(impl);
	}

	Reader::Reader() {
		return;
	}

	int Reader::read() {
		return this->impl->read();
	}

	int Reader::read(wchar_t* buf, unsigned int len) {
		return this->impl->read(buf, len);
	}

	long long Reader::skip(long long n) {
		return this->impl->skip(n);
	}

	void Reader::close() {
		this->impl->close();
	}

// -------------------------------------------------------------------------- //
// Writer 
// -------------------------------------------------------------------------- //
	detail::WriterInterface* Writer::getImpl() const {
		return this->impl.get();
	}

	void Writer::setImpl(detail::WriterInterface* impl) {
		this->impl.reset(impl);
	}

	Writer::Writer() {
		return;
	}

	void Writer::write(wchar_t character) {
		this->impl->write(static_cast<int>(character));
	}

	void Writer::write(const wchar_t* buf, unsigned int len) {
		this->impl->write(buf, len);
	}

	void Writer::flush() {
		this->impl->flush();
	}

	void Writer::close() {
		this->impl->close();
	}

	namespace detail {
#ifdef ARX_WIN
// -------------------------------------------------------------------------- //
// ConsoleWriterImpl
// -------------------------------------------------------------------------- //
		class ConsoleWriterImpl: public WriterInterface {
		public:
			ConsoleWriterImpl() {
				return;
			}

			void write(const wchar_t* buf, unsigned int len) {
				DWORD written;
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, &written, NULL);
				// TODO: WriteConsoleW may write not the whole string, but only a part of it
			}

			void flush() {
				return;
			}

			void close() {
				return;
			}
		};
#endif

// -------------------------------------------------------------------------- //
// InputStreamReaderImpl
// -------------------------------------------------------------------------- //
		class InputStreamReaderImpl: public ReaderInterface {
		private:
			bool readingUTF16;
			const static unsigned int READ_COUNT = 4096;
			InputStream stream;
			ToUTF16Converter conv;
			wstring buf;
			unsigned int bufPos;
			string cbuf;

		public:
			unsigned int getBufPos() {
				return this->bufPos;
			}

			void setBufPos(unsigned int bufPos) {
				this->bufPos = bufPos;
			}

			wchar_t* getBuf() {
				return const_cast<wchar_t*>(this->buf.c_str());
			}

			unsigned int bufLeft() {
				return static_cast<unsigned int>(this->buf.size() - this->bufPos);
			}

			int fillBuf() {
				assert(bufLeft() == 0);
				this->bufPos = 0;
				if(!this->readingUTF16) {
					unsigned int oldSize = static_cast<unsigned int>(this->cbuf.size());
					this->cbuf.resize(READ_COUNT + 1);
					unsigned int status = this->stream.read(reinterpret_cast<unsigned char*>(const_cast<char*>(this->cbuf.c_str())) + oldSize, READ_COUNT - oldSize);
					if(status == EOF)
						return EOF;
					this->cbuf.resize(oldSize + status);
					this->buf = conv.convertSome(cbuf);
					return status;
				} else {
					this->buf.reserve(READ_COUNT / sizeof(wchar_t) + 1);
					unsigned int status = this->stream.read(reinterpret_cast<unsigned char*>(const_cast<wchar_t*>(this->buf.c_str())), READ_COUNT);
					if(status == EOF)
						return EOF;
					this->buf[status] = 0;
					return status;
				}
			}

			InputStreamReaderImpl(InputStream stream, const char* encoding): conv(isUTF16(encoding) ? "DEFAULT" : encoding), stream(stream) {
				this->readingUTF16 = isUTF16(encoding);
				this->bufPos = 0;
			}

			int read(wchar_t* dst, unsigned int len) {
				return detail::bufferedRead(this, dst, len);
				/*
				unsigned int initialLen = len;
				while(len > 0) {
					if(bufLength() > 0) {
						unsigned int toMove = min(bufLength(), len);
						memcpy(dst, this->buf.c_str() + this->bufPos, toMove * sizeof(wchar_t));
						this->bufPos += toMove;
						len -= toMove;
						dst += toMove;
					}
					if(bufLength() <= 0) {
						int status = fillBuf();
						if(status == EOF && initialLen == len)
							return EOF;
						else if(status == EOF)
							return initialLen - len;
					}
				}
				return initialLen;
				*/
			}

			void close() {
				this->stream.close();
			}
		};

	} // namespace detail

// -------------------------------------------------------------------------- //
// InputStreamReader
// -------------------------------------------------------------------------- //
	InputStreamReader::InputStreamReader(InputStream stream, const char* encoding) {
		this->setImpl(new detail::InputStreamReaderImpl(stream, encoding));
	}


	namespace detail {
// -------------------------------------------------------------------------- //
// OutputStreamWriterImpl
// -------------------------------------------------------------------------- //
		class OutputStreamWriterImpl: public WriterInterface {
		private:
			bool writingUTF16;
			OutputStream stream;
			FromUTF16Converter conv;
		public:
			OutputStreamWriterImpl(OutputStream stream, const char* encoding): conv(isUTF16(encoding) ? "DEFAULT" : encoding), stream(stream) {
				this->writingUTF16 = isUTF16(encoding);
			}

			void write(const wchar_t* buf, unsigned int len) {
				if(!this->writingUTF16) {
					string s = conv.convert(wstring(buf, len));
					this->stream.write(reinterpret_cast<const unsigned char*>(s.c_str()), static_cast<unsigned int>(s.size()));
				} else
					this->stream.write(reinterpret_cast<const unsigned char*>(buf), len * sizeof(wchar_t));
			}

			void flush() {
				this->stream.flush();
			}

			void close() {
				this->stream.close();
			}

		};
	} // namespace detail


// -------------------------------------------------------------------------- //
// OutputStreamWriter
// -------------------------------------------------------------------------- //
	OutputStreamWriter::OutputStreamWriter(OutputStream stream, const char* encoding) {
#ifdef ARX_WIN
		detail::OutputStreamInterface* pInterface = detail::StreamManager::getImpl(stream);
		if(dynamic_cast<detail::ConsoleOutputStreamImpl*>(pInterface) != NULL)
			this->setImpl(new detail::ConsoleWriterImpl());
		else
			this->setImpl(new detail::OutputStreamWriterImpl(stream, encoding));
#endif
#ifdef ARX_LINUX
		this->setImpl(new detail::OutputStreamWriterImpl(stream, encoding));
#endif
	}


	namespace detail {
// -------------------------------------------------------------------------- //
// BufferedReaderInterface
// -------------------------------------------------------------------------- //
		class BufferedReaderInterface: public ReaderInterface {
		public:
			virtual int readLine(wstring& outString, bool withLineSeparator) = 0;
		};


// -------------------------------------------------------------------------- //
// BufferedWriterInterface
// -------------------------------------------------------------------------- //
		class BufferedWriterInterface: public WriterInterface {
		public:
			virtual void write(const std::wstring& s) {
				static_cast<WriterInterface*>(this)->write(s.c_str(), static_cast<unsigned int>(s.size()));
			}

			virtual void newLine() {
				static_cast<WriterInterface*>(this)->write(L"\n", 1);
			}
		};


// -------------------------------------------------------------------------- //
// BufferedReaderImpl
// -------------------------------------------------------------------------- //
		class BufferedReaderImpl: public BufferedReaderInterface {
		private:
			static const unsigned int DEFAULT_BUF_SIZE = 4096;
			static const unsigned int DEFAULT_LINE_LENGTH = 80;
			Reader reader;
			wchar_t* buf;
			unsigned int bufSize;
			unsigned int bufPos;
			unsigned int bufFill;

			void skipLineEnding(wstring& outString, bool withLineSeparator) {
				if(withLineSeparator)
					outString += this->buf[this->bufPos];
				this->bufPos++;
				if(this->buf[this->bufPos - 1] == L'\r') {
					// \r\n case
					if(bufLeft() == 0)
						fillBuf();
					if(bufLeft() != 0 && this->buf[this->bufPos] == L'\n') {
						if(withLineSeparator)
							outString += this->buf[this->bufPos];
						this->bufPos++;
					}
				}
			}

		public:
			unsigned int getBufPos() {
				return this->bufPos;
			}

			void setBufPos(unsigned int bufPos) {
				this->bufPos = bufPos;
			}

			wchar_t* getBuf() {
				return this->buf;
			}

			unsigned int bufLeft() {
				return this->bufFill - this->bufPos;
			}

			unsigned int fillBuf() {
				assert(bufLeft() == 0);
				int status = reader.read(buf, bufSize);
				if(status != EOF) {
					this->bufPos = 0;
					this->bufFill = static_cast<unsigned int>(status);
				}
				return status;
			}

			BufferedReaderImpl(Reader reader): reader(reader) {
				this->buf = new wchar_t[DEFAULT_BUF_SIZE];
				this->bufSize = DEFAULT_BUF_SIZE;
				this->bufPos = 0;
				this->bufFill = 0;
			}

			BufferedReaderImpl(Reader reader, unsigned int bufSize): reader(reader) {
				this->buf = new wchar_t[bufSize];
				this->bufSize = bufSize;
				this->bufPos = 0;
				this->bufFill = 0;
			}

			~BufferedReaderImpl() {
				delete[] this->buf;
			}

			int read(wchar_t* buf, unsigned int len) {
				return detail::bufferedRead(this, buf, len);
			}

			int readLine(wstring& outString, bool withLineSeparator) {
				unsigned int pos = 0;
				unsigned int size = DEFAULT_LINE_LENGTH;
				outString.resize(DEFAULT_LINE_LENGTH + 2); // +2 is for "\r\n"
				while(true) {
					if(bufLeft() == 0) {
						int status = fillBuf();
						if(status == EOF) {
							if(pos > 0) {
								outString.resize(pos);
								return pos;
							}
							outString.resize(0);
							return EOF;
						}
					}
					for(; this->bufPos < this->bufFill; this->bufPos++) {
						if(pos == size) {
							size *= 2;
							if(size > numeric_limits<unsigned int>::max() / 4) {
								outString.resize(pos);
								return pos; // line too long
							}
							outString.resize(size + 2);
						}
						if(isLineEnding(this->buf[this->bufPos])) {
							outString.resize(pos);
							skipLineEnding(outString, withLineSeparator);
							return static_cast<unsigned int>(outString.size());
						} else {
							outString[pos] = this->buf[this->bufPos];
							pos++;
						}
					}
				}
				// we won't get here
			}

			/*
			// Heh, i'd better comment it
			long long skip(long long n) {
				if(n > bufLeft()) {
					long long status = this->reader.skip(n - bufLeft());
					if(bufLeft() == 0)
						return status;
					else {
						unsigned int left = bufLeft();
						this->bufPos = this->bufFill;
						return (status > 0) ? (status + left) : (left);
					}
				} else {
					bufPos += static_cast<unsigned int>(n);
					return n;
				}
			}
			*/

			void close() {
				this->reader.close();
			}
		};
	} // namespace detail

// -------------------------------------------------------------------------- //
// BufferedReader
// -------------------------------------------------------------------------- //
	BufferedReader::BufferedReader() {
		return;
	}

	BufferedReader::BufferedReader(Reader reader) {
		this->setImpl(new detail::BufferedReaderImpl(reader));
	}

	BufferedReader::BufferedReader(Reader reader, unsigned int bufSize) {
		this->setImpl(new detail::BufferedReaderImpl(reader, bufSize));
	}

	int BufferedReader::readLine(wstring& outString, bool withLineSeparator) {
		return dynamic_cast<detail::BufferedReaderInterface*>(this->getImpl())->readLine(outString, withLineSeparator);
	}


	namespace detail {
// -------------------------------------------------------------------------- //
// BufferedWriterImpl
// -------------------------------------------------------------------------- //
		class BufferedWriterImpl: public BufferedWriterInterface {
		private:
			static const unsigned int DEFAULT_BUF_SIZE = 4096;
			Writer writer;
			wchar_t* buf;
			unsigned int bufSize;
			unsigned int bufPos;

			wstring lineSeparator;

			unsigned int bufLeft() {
				return this->bufSize - this->bufPos;
			}

			unsigned int writeToBuf(const wchar_t* buf, unsigned int len) {
				unsigned int toMove = min(len, bufLeft());
				memcpy(this->buf + this->bufPos, buf, toMove);
				this->bufPos += toMove;
				return toMove;
			}

			void writeBuf() {
				writer.write(this->buf, this->bufPos);
				this->bufPos = 0;
			}

			void init(unsigned int bufSize, const wstring& lineSeparator) {
				this->buf = new wchar_t[bufSize];
				this->bufSize = bufSize;
				this->bufPos = 0;
				this->lineSeparator = lineSeparator;
			}

		public:
			BufferedWriterImpl(Writer writer): writer(writer) {
				init(DEFAULT_BUF_SIZE, getDefaultLineSeparator());
			}

			BufferedWriterImpl(Writer writer, unsigned int bufSize): writer(writer) {
				init(bufSize, getDefaultLineSeparator());
			}

			BufferedWriterImpl(Writer writer, const wstring& lineSeparator): writer(writer) {
				init(DEFAULT_BUF_SIZE, lineSeparator);
			}

			BufferedWriterImpl(Writer writer, unsigned int bufSize, const wstring& lineSeparator): writer(writer) {
				init(bufSize, lineSeparator);
			}

			~BufferedWriterImpl() {
				delete[] this->buf;
			}

			void write(const wchar_t* buf, unsigned int len) {
				unsigned int written = 0;
				while(written != len) {
					written += writeToBuf(buf + written, len - written);
					if(bufLeft() == 0)
						writeBuf();
				}
			}

			void write(const std::wstring& s) {
				write(s.c_str(), static_cast<unsigned int>(s.size()));
			}

			void newLine() {
				write(this->lineSeparator);
			}

			void flush() {
				writeBuf();
				this->writer.flush();
			}

			void close() {
				writeBuf();
				this->writer.close();
			}
		};
	} // namespace detail


// -------------------------------------------------------------------------- //
// BufferedWriter
// -------------------------------------------------------------------------- //
	BufferedWriter::BufferedWriter() {
		return;
	}

	BufferedWriter::BufferedWriter(Writer writer) {
		this->setImpl(new detail::BufferedWriterImpl(writer));
	}

	BufferedWriter::BufferedWriter(Writer writer, unsigned int bufSize) {
		this->setImpl(new detail::BufferedWriterImpl(writer, bufSize));
	}

	BufferedWriter::BufferedWriter(Writer writer, const wstring& lineSeparator) {
		this->setImpl(new detail::BufferedWriterImpl(writer, lineSeparator));
	}

	BufferedWriter::BufferedWriter(Writer writer, unsigned int bufSize, const wstring& lineSeparator) {
		this->setImpl(new detail::BufferedWriterImpl(writer, bufSize, lineSeparator));
	}

	void BufferedWriter::write(const std::wstring& s) {
		dynamic_cast<detail::BufferedWriterInterface*>(this->getImpl())->write(s);
	}

	void BufferedWriter::newLine() {
		dynamic_cast<detail::BufferedWriterInterface*>(this->getImpl())->newLine();
	}


	namespace detail {
// -------------------------------------------------------------------------- //
// FormatReaderImpl
// -------------------------------------------------------------------------- //
		class FormatReaderImpl: public BufferedReaderInterface {
		private:
			BufferedReader reader;
			bool isFail;
			bool isEof;
			wstring line;
			unsigned int pos;

			wstring removeLineEnding(wstring& s) {
				wchar_t c;
				if(s.size() > 0 && isLineEnding(c = s[s.size() - 1]))
					s.resize(s.size() - 1);
				if(s.size() > 0 && c == L'\n' && s[s.size() - 1] == L'\r')
					s.resize(s.size() - 1);
				return s;
			}

			int nextLine() {
				int status = this->reader.readLine(line, true);
				if(status == EOF)
					this->isEof = true;
				this->pos = 0;
				return status;
			}

		public:
			FormatReaderImpl(BufferedReader reader): reader(reader) {
				this->isFail = false;
				this->isEof = false;
				this->pos = 0;
			}

			int fillBuf() {
				return nextLine();
			}

			void skipWhiteSpaces() {
				if(this->fail() || this->eof())
					return;
				while(true) {
					if(this->pos == this->line.size())
						nextLine();
					if(eof())
						return;
					if(isWhiteSpace(this->line[this->pos]))
						this->pos++;
					else
						return;
				}
			}

			unsigned int getBufPos() {
				return this->pos;
			}

			void setBufPos(unsigned int bufPos) {
				this->pos = bufPos;
			}

			wchar_t* getBuf() {
				return const_cast<wchar_t*>(this->line.c_str());
			}

			unsigned int bufLeft() {
				return static_cast<unsigned int>(this->line.size() - this->pos);
			}

			void setFail(bool fail) {
				this->isFail = fail;
			}

			operator void* () const {
				return fail() ? NULL : (void*)(1);
			}

			bool operator! () const {
				return fail();
			}

			bool fail() const {
				return this->isFail;
			}

			bool eof() const {
				return this->isEof;
			}

			int readLine(wstring& outString, bool withLineSeparator) {
				if(fail() || eof())
					return EOF;
				if(this->pos == this->line.size())
					nextLine();
				outString = this->line.substr(this->pos);
				if(!withLineSeparator)
					removeLineEnding(outString);
				return static_cast<int>(outString.size());
			}

			int read(wchar_t* buf, unsigned int len) {
				if(fail() || eof())
					return EOF;
				return detail::bufferedRead(this, buf, len);
			}

			void close() {
				this->reader.close();
			}
		};
	} // namespace detail

// -------------------------------------------------------------------------- //
// FormatReader
// -------------------------------------------------------------------------- //
	void FormatReader::skipWhiteSpaces() {
		dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->skipWhiteSpaces();
	}

	void FormatReader::setFail(bool fail) {
		dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->setFail(fail);
	}

	unsigned int FormatReader::getBufPos() {
		return dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->getBufPos();
	}

	void FormatReader::setBufPos(unsigned int bufPos) {
		dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->setBufPos(bufPos);
	}

	wchar_t* FormatReader::getBuf() {
		return dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->getBuf();
	}

	unsigned int FormatReader::bufLeft() {
		return dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->bufLeft();
	}

	FormatReader::FormatReader(BufferedReader reader) {
		this->setImpl(new detail::FormatReaderImpl(reader));
	}

	FormatReader::operator void* () const {
		return dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->operator void* ();
	}

	bool FormatReader::operator! () const {
		return dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->operator! ();
	}

	bool FormatReader::fail() const {
		return dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->fail();
	}

	bool FormatReader::eof() const {
		return dynamic_cast<detail::FormatReaderImpl*>(this->getImpl())->eof();
	}


	namespace detail {
// -------------------------------------------------------------------------- //
// FormatWriterImpl
// -------------------------------------------------------------------------- //
		class FormatWriterImpl: public BufferedWriterInterface {
		private:
			Writer writer;
			wostringstream stream;

		public:
			FormatWriterImpl(Writer writer): writer(writer) {
				return;
			}

			wostringstream* getCurrentStream() {
				return &this->stream;
			}

			void newLine() {
				if(dynamic_cast<BufferedWriterInterface*>(detail::StreamManager::getImpl(writer)) != NULL)
					dynamic_cast<BufferedWriterInterface*>(detail::StreamManager::getImpl(writer))->newLine();
				else
					dynamic_cast<BufferedWriterInterface*>(this)->write(getDefaultLineSeparator());
			}

			void write(const wchar_t* buf, unsigned int len) {
				this->writer.write(buf, len);
			}

			void flush() {
				this->writer.flush();
			}

			void close() {
				this->writer.close();
			}
		};

	} // namespace detail



// -------------------------------------------------------------------------- //
// FormatWriter
// -------------------------------------------------------------------------- //
	FormatWriter::FormatWriter(Writer writer) {
		this->setImpl(new detail::FormatWriterImpl(writer));
	}

	std::wostringstream* FormatWriter::getCurrentStream() {
		return dynamic_cast<detail::FormatWriterImpl*>(this->getImpl())->getCurrentStream();
	}

	void FormatWriter::newLine() {
		return dynamic_cast<detail::FormatWriterImpl*>(this->getImpl())->newLine();
	}


} // namespace arx


