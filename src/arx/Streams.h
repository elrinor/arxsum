#ifndef __ARX_STREAMS_H__
#define __ARX_STREAMS_H__

#include "config.h"
#include "shared_ptr.h"
#include <string>
#include <sstream>

#ifndef EOF
#  define EOF (-1)
#endif

namespace arx {
	namespace detail {
		class StreamManager;

		class InputStreamInterface;
		class OutputStreamInterface;
	}

// -------------------------------------------------------------------------- //
// InputStream
// -------------------------------------------------------------------------- //
	class InputStream {
	private:
		friend class detail::StreamManager;
		shared_ptr<detail::InputStreamInterface> impl;

	protected:
		/**
		 * @brief Returns the shared_ptr pointer to the implementation class.
		 * @returns the shared_ptr pointer to the implementation class
		 */
		detail::InputStreamInterface* getImpl() const;
		
		/**
		 * @brief Sets the shared_ptr pointer to the implementation class.
		 * @param impl the shared_ptr pointer to the new implementation object.
		 */
		void setImpl(detail::InputStreamInterface* impl);

		/**
		 * Protected constructor.
		 */
		InputStream();

	public:
		/**
		 * @brief Reads one byte from the stream. 
		 * @returns the next byte of data, or -1 (EOF) if the end of stream was reached.
		 */
		int read();

		/**
		 * @brief Reads up to len bytes and stores them into the buffer buf.
		 * @param buf the buffer to read bytes to
		 * @param len the length of the buffer
		 * @returns the total number of bytes read, or -1 (EOF) if the end of stream was reached
		 */
		int read(unsigned char* buf, unsigned int len);

		/**
		 * @brief Skips up to n bytes. 
		 * The actual number of bytes skipped can be less than n for a variety of reasons, for example because of reaching the end of file.
		 * @param n the number of bytes to be skipped
		 * @returns the number of actually skipped bytes. The negative value indicates that no bytes were skipped.
		 */
		long long skip(long long n);

		/**
		* @brief Closes the stream and releases all the system resources associated with this stream. No more reading will be possible.
		*/
		void close();
	};


// -------------------------------------------------------------------------- //
// OutputStream
// -------------------------------------------------------------------------- //
	class OutputStream {
	private:
		friend class detail::StreamManager;
		shared_ptr<detail::OutputStreamInterface> impl;

	protected:
		/**
		 * @brief Returns the shared_ptr pointer to the implementation class.
		 * @returns the shared_ptr pointer to the implementation class
		 */
		detail::OutputStreamInterface* getImpl() const;
		
		/**
		 * @brief Sets the shared_ptr pointer to the implementation class.
		 * @param impl the shared_ptr pointer to the new implementation object.
		 */
		void setImpl(detail::OutputStreamInterface* impl);

		/**
		 * Protected constructor.
		 */
		OutputStream();

	public:
		/**
		 * @brief Writes one byte to the stream. The byte to be written is the eight low-order bits of argument byte.
		 * @param byte the byte to be written
		 */
		void write(int byte);

		/**
		 * @brief Writes len bytes from the buffer buf to the stream.
		 * @param buf the data buffer
		 * @param len length of the data buffer
		 */
		void write(const unsigned char* buf, unsigned int len);

		/**
		 * @brief Flushes the stream and forces any output bytes to be written out.
		 */
		void flush();

		/**
		 * @brief Closes the stream and releases all the system resources associated with this stream. No more writing will be possible.
		 */
		void close();
	};

	enum FileStreamOpenMode {
		OM_NONE = 0, 
		OM_UNBUFFERED = 1, /*< This flag will result in much faster I/O when dealing with large files. When using this flag, be sure to provide page-aligned pointers to read and write methods.*/
		OM_SEQUENTIAL_SCAN = 2 /*< Used is used as a hint to OS. Does nothing on Linux.*/
	};

// -------------------------------------------------------------------------- //
// FileInputStream
// -------------------------------------------------------------------------- //
	class FileInputStream: public InputStream {
	protected:
		FileInputStream();
	public:
		/**
		 * @brief Creates a FileInputStream by opening a file named fileName in the file system.
		 * @param fileName the name of the file in the file system
		 * @param openMode file open mode. See {@link arx::FileStreamOpenMode}.
		 */
		explicit FileInputStream(std::string fileName, FileStreamOpenMode openMode = OM_NONE);

		/**
		* @brief Creates a FileInputStream by opening a file named fileName in the file system.
		* @param fileName the name of the file in the file system
		* @param openMode file open mode. See {@link arx::FileStreamOpenMode}.
		*/
		explicit FileInputStream(std::wstring fileName, FileStreamOpenMode openMode = OM_NONE);
	};

// -------------------------------------------------------------------------- //
// FileOutputStream
// -------------------------------------------------------------------------- //
	class FileOutputStream: public OutputStream {
	protected:
		FileOutputStream();
	public:
		/**
		* @brief Creates a FileOutputStream by opening a file named fileName in the file system.
		* @param fileName the name of the file in the file system
		* @param openMode file open mode. See {@link arx::FileStreamOpenMode}.
		*/
		explicit FileOutputStream(std::string fileName, FileStreamOpenMode openMode = OM_NONE);

		/**
		* @brief Creates a FileOutputStream by opening a file named fileName in the file system.
		* @param fileName the name of the file in the file system
		* @param openMode file open mode. See {@link arx::FileStreamOpenMode}.
		*/
		explicit FileOutputStream(std::wstring fileName, FileStreamOpenMode openMode = OM_NONE);
	};

// -------------------------------------------------------------------------- //
// ConsoleOutputStream
// -------------------------------------------------------------------------- //
	class ConsoleOutputStream: public FileOutputStream{
	public:
		ConsoleOutputStream();
	};

// -------------------------------------------------------------------------- //
// ConsoleInputStream
// -------------------------------------------------------------------------- //
	class ConsoleInputStream: public FileInputStream{
	public:
		ConsoleInputStream();
	};

	namespace detail {
		class ReaderInterface;
		class WriterInterface;
	}

// -------------------------------------------------------------------------- //
// Reader 
// -------------------------------------------------------------------------- //
	class Reader {
	private:
		friend class detail::StreamManager;
		shared_ptr<detail::ReaderInterface> impl;

	protected:
		/**
		 * @brief Returns the shared_ptr pointer to the implementation class.
		 * @returns the shared_ptr pointer to the implementation class
		 */
		detail::ReaderInterface* getImpl() const;

		/**
		 * @brief Sets the shared_ptr pointer to the implementation class.
		 * @param impl the shared_ptr pointer to the new implementation object.
		 */
		void setImpl(detail::ReaderInterface* impl);

		/**
		 * Protected constructor.
		 */
		Reader();

	public:
		/**
		 * @brief Reads one character from the stream. 
		 * @returns the next character, or -1 (EOF) if the end of stream was reached.
		 */
		int read();

		/**
		 * @brief Reads up to len characters and stores them into the buffer buf.
		 * @param buf the buffer to read characters to
		 * @param len the length of the buffer
		 * @returns the total number of characters read, or -1 (EOF) if the end of stream was reached
		 */
		int read(wchar_t* buf, unsigned int len);

		/**
		 * @brief Skips up to n characters. 
		 * 
		 * The actual number of characters skipped can be less than n for a variety of reasons, for example because of reaching the end of file.
		 * @param n the number of characters to be skipped
		 * @returns the number of actually skipped characters. The negative value indicates that no characters were skipped.
		 */
		long long skip(long long n);

		/**
		 * @brief Closes the Reader and releases all the system resources associated with it. No more reading will be possible.
		 */
		void close();
	};

// -------------------------------------------------------------------------- //
// Writer 
// -------------------------------------------------------------------------- //
	class Writer {
	private:
		friend class detail::StreamManager;
		shared_ptr<detail::WriterInterface> impl;

	protected:
		/**
		* @brief Returns the shared_ptr pointer to the implementation class.
		* @returns the shared_ptr pointer to the implementation class
		*/
		detail::WriterInterface* getImpl() const;

		/**
		* @brief Sets the shared_ptr pointer to the implementation class.
		* @param impl the shared_ptr pointer to the new implementation object.
		*/
		void setImpl(detail::WriterInterface* impl);

		/**
		* Protected constructor.
		*/
		Writer();

	public:
		/**
		* @brief Writes one character.
		* @param character the character to be written
		*/
		void write(wchar_t character);

		/**
		* @brief Writes len characters from the buffer buf.
		* @param buf the character buffer
		* @param len length of the character buffer
		*/
		void write(const wchar_t* buf, unsigned int len);

		/**
		* @brief Flushes the Writer and forces any output characters to be written out.
		*/
		void flush();

		/**
		* @brief Closes the Writer and releases all the system resources associated with it. No more writing will be possible.
		*/
		void close();
	};

// -------------------------------------------------------------------------- //
// InputStreamReader
// -------------------------------------------------------------------------- //
	class InputStreamReader: public Reader {
	public:
		explicit InputStreamReader(InputStream stream, const char* encoding = "DEFAULT");
	};

// -------------------------------------------------------------------------- //
// OutputStreamWriter
// -------------------------------------------------------------------------- //
	class OutputStreamWriter: public Writer {
	public:
		explicit OutputStreamWriter(OutputStream stream, const char* encoding = "DEFAULT");
	};

// -------------------------------------------------------------------------- //
// BufferedReader
// -------------------------------------------------------------------------- //
	class BufferedReader: public Reader {
	protected:
		/**
		* Protected constructor.
		*/
		BufferedReader();

	public:
		/**
		 * @brief Creates a new BufferedReader, that reads from given Reader.
		 * @param reader the Reader to read from
		 */
		explicit BufferedReader(Reader reader);

		/**
		* @brief Creates a new BufferedReader, that reads from given Reader, with a buffer of size bufSize.
		* @param reader the Reader to read from
		* @param bufSize the size of the character buffer
		*/
		BufferedReader(Reader reader, unsigned int bufSize);

		/**
		 * @brief Reads one line of input. 
		 * @param outString the string to store the characters at
		 * @param withLineSeparator append the line separator to outString?
		 * @returns the number of characters read, or -1 (EOF) if the end of underlying Reader was reached
		 */
		int readLine(std::wstring& outString, bool withLineSeparator = false);
	};

// -------------------------------------------------------------------------- //
// BufferedWriter
// -------------------------------------------------------------------------- //
	class BufferedWriter: public Writer {
	protected:
		/**
		* Protected constructor.
		*/
		BufferedWriter();

	public:
		/**
		 * @brief Creates a new BufferedWriter that writes to a given Writer.
		 * @param writer the Writer to write to
		 */
		explicit BufferedWriter(Writer writer);

		/**
		 * @brief Creates a new BufferedWriter that writes to a given Writer, with a buffer of size bufSize.
		 * @param writer the Writer to write to
		 * @param bufSize the size of the character buffer
		 */
		BufferedWriter(Writer writer, unsigned int bufSize);

		/**
		 * @brief Creates a new BufferedWriter that writes to a given Writer, with the given line separator of size bufSize.
		 * @param writer the Writer to write to
		 * @param lineSeparator the line separator to use
		 */
		BufferedWriter(Writer writer, const std::wstring& lineSeparator);

		/**
		 * @brief Creates a new BufferedWriter that writes to a given Writer, with the given line separator and a buffer of size bufSize.
		 * @param writer the Writer to write to
		 * @param bufSize the size of the character buffer
		 * @param lineSeparator the line separator to use
		 */
		BufferedWriter(Writer writer, unsigned int bufSize, const std::wstring& lineSeparator);

		/**
		 * @brief Writes the given string s. 
		 * @param s the string to be written out
		 */
		void write(const std::wstring& s);

		/**
		 * @brief Writes a line separator. 
		 * 
		 * The line separator string is not necessarily a single newline ('\n') character. 
		 */
		void newLine();
	};

// -------------------------------------------------------------------------- //
// FormatReader
// -------------------------------------------------------------------------- //
	class FormatReader: public BufferedReader {
	private:
		void skipWhiteSpaces();
		void setFail(bool fail);
		unsigned int getBufPos();
		void setBufPos(unsigned int bufPos);
		wchar_t* getBuf();
		unsigned int bufLeft();

	public:
		explicit FormatReader(BufferedReader reader);

		/**
		 * @brief Indicates if the stream is still good.
		 * @returns a null pointer if fail(), non-null pointer otherwise
		 */
		operator void* () const;

		/**
		 * @brief Indicates if the stream is not bad.
		 * @returns fail()
		 */
		bool operator! () const;

		/**
		 * Indicated a failure to extract a valid field from an underlying data source.
		 * @returns true if the last field extraction operation failed
		 */
		bool fail() const;

		/**
		 * Indicates an end of file.
		 * @returns true if the end of file was reached
		 */
		bool eof() const;

		/**
		 * Generic read operator. Reads anything that you can read using stl stream.
		 * @param value value to read from the stream 
		 * @returns *this, so that chaining is possible
		 */
		template<class T>
		FormatReader& operator>> (T& value) {
			skipWhiteSpaces();
			if(this->fail() || this->eof())
				return *this;
			std::wstring s(getBuf(), getBufPos() + bufLeft());
			std::wistringstream stream(s);
			stream.seekg(getBufPos());
			stream >> value;
			if(stream.fail())
				this->setFail(true);
			else
				setBufPos(stream.tellg());
			return *this;
		}
	};

// -------------------------------------------------------------------------- //
// FormatWriter
// -------------------------------------------------------------------------- //
	class FormatWriter: public BufferedWriter {
	private:
		std::wostringstream* getCurrentStream();

	public:
		explicit FormatWriter(Writer writer);
		
		template<class T>
		FormatWriter& operator<< (const T& value) {
			std::wostringstream* stream = getCurrentStream();
			if(stream->str().size() > 0)
				stream->str(L"");
			*stream << value;
			if(stream->str().size() > 0)
				write(stream->str());
			return *this;
		}


		/*
		typedef std::wostringstream& (*ManipulatorFunc) (std::wostringstream&);

		FormatWriter& operator<< (std::wostringstream& (*func) (std::wostringstream&)) {
			std::wostringstream* stream = getCurrentStream();
			if(stream->str().size() > 0)
				stream->str(L"");
			*stream << func;
			if(stream->str().size() > 0)
				write(stream->str());
			return *this;
		}
		*/


		void newLine();
	};

}

#endif

