/******************************************************************//**
 * \file   string.hpp
 * \author Elliot Goodrich
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#ifndef SSO23_STRING_INCLUDED
#define SSO23_STRING_INCLUDED

#include <algorithm>
#include <cstddef>
#include <climits>
#include <cstring>
#include <ostream>
#include <type_traits>

#include <jemalloc/jemalloc.h>

namespace sso23 {

namespace detail {

static std::size_t const high_bit_mask = static_cast<std::size_t>(1) << (sizeof(std::size_t) * CHAR_BIT - 1);
static std::size_t const sec_high_bit_mask = static_cast<std::size_t>(1) << (sizeof(std::size_t) * CHAR_BIT - 2);

template <typename T>
unsigned char* uchar_cast(T* p) {
	return reinterpret_cast<unsigned char*>(p);
}

template <typename T>
unsigned char const* uchar_cast(T const* p) {
	return reinterpret_cast<unsigned char const*>(p);
}

template <typename T>
unsigned char& most_sig_byte(T& obj) {
	return *(reinterpret_cast<unsigned char*>(&obj) + sizeof(obj) - 1);
}

template <int N>
bool lsb(unsigned char byte) {
	return byte & (1u << N);
}

template <int N>
bool msb(unsigned char byte) {
	return byte & (1u << (CHAR_BIT - N - 1));
}

template <int N>
void set_lsb(unsigned char& byte, bool bit) {
	if(bit) {
		byte |= 1u << N;
	} else {
		byte &= ~(1u << N);
	}
}

template <int N>
void set_msb(unsigned char& byte, bool bit) {
	if(bit) {
		byte |= 1u << (CHAR_BIT - N - 1);
	} else {
		byte &= ~(1u << (CHAR_BIT - N - 1));
	}
}

}

template <typename CharT,
          typename Traits = std::char_traits<CharT>>
class basic_string {
	typedef typename std::make_unsigned<CharT>::type UCharT;
public:
	basic_string() noexcept
	: basic_string{"", static_cast<std::size_t>(0)} {
	}

	basic_string(std::size_t size) {
        if(size <= sso_capacity) {
            set_sso_size(0);
        } else {
            m_data.non_sso.ptr = (CharT*) malloc(sizeof(CharT)*(size + 1));
            set_non_sso_data(size, size);
        }
	}

	basic_string(CharT const* string, std::size_t size) {
		if(size <= sso_capacity) {
			Traits::move(m_data.sso.string, string, size);
			Traits::assign(m_data.sso.string[size], static_cast<CharT>(0));
			set_sso_size(size);
		} else {
			m_data.non_sso.ptr = (CharT*) malloc(sizeof(CharT)*(size + 1));
			Traits::move(m_data.non_sso.ptr, string, size);
			Traits::assign(m_data.non_sso.ptr[size], static_cast<CharT>(0));
			set_non_sso_data(size, size);
		}
	}

	basic_string(CharT const* string)
	: basic_string{string, std::strlen(string)} {
	}

	basic_string(const basic_string& string) {
		if(string.sso()) {
			m_data.sso = string.m_data.sso;
		} else {
			new (this) basic_string{string.c_str(), string.size()};
		}
	}

	basic_string(basic_string&& string) noexcept {
		m_data = string.m_data;
		string.set_moved_from();
	}

	basic_string& operator=(basic_string const& other) {
		auto copy = other;
		swap(copy, *this);
		return *this;
	}

	basic_string& operator=(basic_string&& other) {
		this->~basic_string();
		m_data = other.m_data;
		other.set_moved_from();
		return *this;
	}

	~basic_string() {
		if(!sso()) {
			free(m_data.non_sso.ptr);
		}
	}

	CharT const* c_str() const noexcept {
		return sso() ? m_data.sso.string : m_data.non_sso.ptr;
	}

    CharT * str() noexcept {
        return sso() ? m_data.sso.string : m_data.non_sso.ptr;
    }

	std::size_t size() const noexcept {
		if(sso()) {
			return sso_size();
		} else {
			return read_non_sso_data().first;
		}
	}

	std::size_t capacity() const noexcept {
		if(sso()) {
			return sizeof(m_data) - 1;
		} else {
			return read_non_sso_data().second;
		}
	}

	friend void swap(basic_string& lhs, basic_string& rhs) {
	    Data tmpData = lhs.m_data;
	    lhs.m_data = rhs.m_data;
	    rhs.m_data = tmpData;
		//std::swap(lhs.m_data, rhs.m_data);
	}


private:
	void set_moved_from() {
		set_sso_size(0);
	}

	// We are using sso if the last two bits are 0
	bool sso() const noexcept {
		return !detail::lsb<0>(m_data.sso.size) && !detail::lsb<1>(m_data.sso.size);
	}

	// good
	void set_sso_size(unsigned char size) noexcept {
		m_data.sso.size = static_cast<UCharT>(sso_capacity - size) << 2;
	}

	// good
	std::size_t sso_size() const noexcept {
		return sso_capacity - ((m_data.sso.size >> 2) & 63u);
	}

	void set_non_sso_data(std::size_t size, std::size_t capacity) {
		auto& size_hsb = detail::most_sig_byte(size);
		auto const size_high_bit = detail::msb<0>(size_hsb);

		auto& cap_hsb = detail::most_sig_byte(capacity);
		auto const cap_high_bit     = detail::msb<0>(cap_hsb);
		auto const cap_sec_high_bit = detail::msb<1>(cap_hsb);

		detail::set_msb<0>(size_hsb, cap_sec_high_bit);

		cap_hsb <<= 2;
		detail::set_lsb<0>(cap_hsb, cap_high_bit);
		detail::set_lsb<1>(cap_hsb, !size_high_bit);

		m_data.non_sso.size = size;
		m_data.non_sso.capacity = capacity;
	}

	std::pair<std::size_t, std::size_t> read_non_sso_data() const {
		auto size = m_data.non_sso.size;
		auto capacity = m_data.non_sso.capacity;

		auto& size_hsb = detail::most_sig_byte(size);
		auto& cap_hsb  = detail::most_sig_byte(capacity);

		// Remember to negate the high bits
		auto const cap_high_bit     = detail::lsb<0>(cap_hsb);
		auto const size_high_bit    = !detail::lsb<1>(cap_hsb);
		auto const cap_sec_high_bit = detail::msb<0>(size_hsb);

		detail::set_msb<0>(size_hsb, size_high_bit);

		cap_hsb >>= 2;
		detail::set_msb<0>(cap_hsb, cap_high_bit);
		detail::set_msb<1>(cap_hsb, cap_sec_high_bit);

		return std::make_pair(size, capacity);
	}

private:
	union Data {
		struct NonSSO {
			CharT* ptr;
			std::size_t size;
			std::size_t capacity;
		} non_sso/* __attribute((aligned(1)))*/;
		struct SSO {
			CharT string[sizeof(NonSSO) / sizeof(CharT) - 1];
			UCharT size;
		} sso/* __attribute((aligned(1)))*/;
	} m_data __attribute((aligned(8)));

public:
	static std::size_t const sso_capacity = sizeof(typename Data::NonSSO) / sizeof(CharT) - 1;
} __attribute((aligned(1),packed));

template <typename CharT, typename Traits>
bool operator==(const basic_string<CharT, Traits>& lhs, const CharT* rhs) noexcept {
	return !std::strcmp(lhs.c_str(), rhs);
}

template <typename CharT, typename Traits>
bool operator==(const CharT* lhs, const basic_string<CharT, Traits>& rhs) noexcept {
	return rhs == lhs;
}

template <typename CharT, typename Traits>
bool operator==(const basic_string<CharT, Traits>& lhs,
                const basic_string<CharT, Traits>& rhs) noexcept {
	if(lhs.size() != rhs.size()) return false;
	return !std::strcmp(lhs.c_str(), rhs.c_str());
}

template <typename CharT, typename Traits>
std::ostream& operator<<(std::ostream& stream, const basic_string<CharT, Traits>& string) {
	return stream << string.c_str();
}

typedef basic_string<char> string;

}

#endif // SSO32_STRING_INCLUDED
