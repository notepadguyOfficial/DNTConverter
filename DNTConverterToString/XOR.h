#pragma once

#include <string>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <utility>

#define BEGIN_NAMESPACE(x) namespace x {
#define END_NAMESPACE }

BEGIN_NAMESPACE(XorCompileTime)

// Replace structured constexpr seed calculation (C++14-compatible)
constexpr int compile_time_seed() {
	return __TIME__[7] - '0' +
		(__TIME__[6] - '0') * 10 +
		(__TIME__[4] - '0') * 60 +
		(__TIME__[3] - '0') * 600 +
		(__TIME__[1] - '0') * 3600 +
		(__TIME__[0] - '0') * 36000;
}

constexpr int seed = compile_time_seed();

// RandomGenerator for C++14
template <int N>
struct RandomGenerator {
private:
	static constexpr unsigned a = 16807; // 7^5
	static constexpr unsigned m = 2147483647; // 2^31 - 1
	static constexpr unsigned s = RandomGenerator<N - 1>::value;

	static constexpr unsigned lo = a * (s & 0xFFFF);
	static constexpr unsigned hi = a * (s >> 16);
	static constexpr unsigned lo2 = lo + ((hi & 0x7FFF) << 16);
	static constexpr unsigned hi2 = hi >> 15;
	static constexpr unsigned lo3 = lo2 + hi;

public:
	static constexpr unsigned max = m;
	static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
};

template <>
struct RandomGenerator<0> {
	static constexpr unsigned value = seed;
};

template <int N, int M>
struct RandomInt {
	static constexpr auto value = RandomGenerator<N + 1>::value % M;
};

template <int N>
struct RandomChar {
	static constexpr char value = static_cast<char>(1 + RandomInt<N, 0x7F - 1>::value);
};

// C++14 index sequence replacement
template <std::size_t... Is>
struct index_sequence {};

template <std::size_t N, std::size_t... Is>
struct make_index_sequence : make_index_sequence<N - 1, N - 1, Is...> {};

template <std::size_t... Is>
struct make_index_sequence<0, Is...> {
	using type = index_sequence<Is...>;
};

template <std::size_t N>
using make_index_sequence_t = typename make_index_sequence<N>::type;

// XorString
template <size_t N, int K, typename Char>
struct XorString {
private:
	const char _key;
	std::array<Char, N + 1> _encrypted;

	constexpr Char enc(Char c) const {
		return c ^ _key;
	}

	Char dec(Char c) const {
		return c ^ _key;
	}

public:
	template <size_t... Is>
	constexpr XorString(const Char* str, index_sequence<Is...>)
		: _key(RandomChar<K>::value), _encrypted{ enc(str[Is])... } {
	}

	constexpr auto decrypt() {
		for (size_t i = 0; i < N; ++i) {
			_encrypted[i] = dec(_encrypted[i]);
		}
		_encrypted[N] = '\0';
		return _encrypted.data();
	}
};

// Variadic printf functions (unchanged)
inline void w_printf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

inline void w_printf_s(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
}

inline void w_sprintf(char* buf, size_t buf_size, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, buf_size, fmt, args);
	va_end(args);
}

inline int w_sprintf_ret(char* buf, size_t buf_size, const char* fmt, ...) {
	int ret;
	va_list args;
	va_start(args, fmt);
	ret = vsnprintf(buf, buf_size, fmt, args);
	va_end(args);
	return ret;
}

inline void w_sprintf_s(char* buf, size_t buf_size, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buf, buf_size, fmt, args);
	va_end(args);
}

inline int w_sprintf_s_ret(char* buf, size_t buf_size, const char* fmt, ...) {
	int ret;
	va_list args;
	va_start(args, fmt);
	ret = vsprintf_s(buf, buf_size, fmt, args);
	va_end(args);
	return ret;
}

#define XorStr(s) ([]{ \
    constexpr XorCompileTime::XorString<sizeof(s) / sizeof(char) - 1, __COUNTER__, char> expr(s, make_index_sequence<sizeof(s) / sizeof(char) - 1>()); \
    return expr; \
}()).decrypt()

#define XorStrW(s) ([]{ \
    constexpr XorCompileTime::XorString<sizeof(s) / sizeof(wchar_t) - 1, __COUNTER__, wchar_t> expr(s, make_index_sequence<sizeof(s) / sizeof(wchar_t) - 1>()); \
    return expr; \
}()).decrypt()

END_NAMESPACE
