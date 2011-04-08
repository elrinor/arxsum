#ifndef __ARX_CONVERTER_H__
#define __ARX_CONVERTER_H__

#include "config.h"
#include "shared_ptr.h"
#include <string>

namespace arx {

	namespace detail {
		class ToUTF16ConverterImpl;
		class FromUTF16ConverterImpl;
	}

	class ToUTF16Converter {
	private:
		shared_ptr<detail::ToUTF16ConverterImpl> impl;
	public:
		/**
		 * @brief Creates a new converter from a system default codepage.
		 */
		ToUTF16Converter(); 

		/**
		 * @brief Creates a new converter from a codepage identified by encoding parameter
		 * @param encoding the name of the codepage to convert from
		 */
		ToUTF16Converter(const char* encoding);

		/**
		 * @brief Converts the given string src to UTF-16. Skips unconvertible characters (like UTF-8 invalid multibyte sequences).
		 * @param src the string to convert
		 * @returns the UTF-16 encoded representation of src
		 */
		std::wstring convert(const std::string& src) const;

		/**
		 * @brief Converts the given string src to UTF-16. Skips unconvertible characters (like UTF-8 invalid multibyte sequences), and leaves an unconverted trail in src.
		 * 
		 * Modifies src in a following way:
		 * <ul>
		 * <li> if src ends with an incomplete multibyte sequence, then this sequence is stored in src </li>
		 * <li> else, src is set to empty string </li>
		 * </ul>
		 * @param src the string to convert
		 * @returns the UTF-16 encoded representation of src
		 */
		std::wstring convertSome(std::string& src) const;
	};

	class FromUTF16Converter {
	private:
		shared_ptr<detail::FromUTF16ConverterImpl> impl;
	public:
		/**
		 * @brief Creates a new converter to a system default codepage.
		 */
		FromUTF16Converter();

		/**
		 * @brief Creates a new converter to a codepage identified by encoding parameter
		 * @param encoding the name of the codepage to convert to
		 */
		FromUTF16Converter(const char* encoding);

		/**
		 * @brief Converts the given string src from UTF-16 to specified encoding.
		 * @param src the string to convert
		 * @returns the converted representation of src
		 */
		std::string convert(const std::wstring& src) const;
	};

} // namespace arx


#endif

