#include <string>
#include <string_view>
#include <tuple>
#include <unicode/utf8.h>

using namespace std;

// In the bellow functions consider Str, Index, Iter as template type parameters
using Str = string;
using Index = Str::size_type;
using Iter = Str::const_iterator;
using OutIt = Str::iterator;

struct code_point_or_error {
	int32_t a;

	// if error() == true, cp() will be unspecified value outside
	// of the Unicode range, i.e. cp() > 0x0010FFFF.
	char32_t cp() const { return a; }
	bool error() const { return a < 0; }
};
// TODO: maybe make this structure to look like std::optional or std::expected.
// We don't use them because it forces additional bool. ICU uses negative int
// to signal error in U8_NEXT.

/* =========== OPERATION: U8_NEXT =============== */
auto u8_advance_i(const Str& s, Index& i) -> code_point_or_error
{
	code_point_or_error cpe;
	auto sz = size(s);
	U8_NEXT(s, i, sz, cpe.a);
	return cpe;
}
auto u8_next_i(const Str& s, Index i) -> std::pair<Index, code_point_or_error>
{
	auto cpe = u8_advance_i(s, i);
	return {i, cpe};
}
auto u8_advance_it(Iter& first, Iter last) -> code_point_or_error
{
	auto i = Iter::difference_type();
	auto sz = distance(first, last);
	code_point_or_error cpe;
	U8_NEXT(first, i, sz, cpe.a);
	advance(first, i);
	return cpe;
}
auto u8_next_it(Iter first, Iter last) -> std::pair<Iter, code_point_or_error>
{
	auto cpe = u8_advance_it(first, last);
	return {first, cpe};
}

/* =========== OPERATION: U8_NEXT_UNSAFE =============== */
auto valid_u8_advance_i(const Str& s, Index& i) -> char32_t
{
	char32_t c;
	U8_NEXT_UNSAFE(s, i, c);
	return c;
}
auto valid_u8_next_i(const Str& s, Index i) -> std::pair<Index, char32_t>
{
	auto c = valid_u8_advance_i(s, i);
	return {i, c};
}
auto valid_u8_advance_it(Iter& first) -> char32_t
{
	auto i = Iter::difference_type();
	char32_t c;
	U8_NEXT_UNSAFE(first, i, c);
	return c;
}
auto valid_u8_next_it(Iter first) -> std::pair<Iter, char32_t>
{
	auto c = valid_u8_advance_it(first);
	return {first, c};
}

/* =========== OPERATION: U8_APPEND =============== */

// starts writing at s[i]. Checks for space except for the first element s[i],
// i < size(s) is precondition.
auto encode_advance_u8(char32_t CP, Str& s, Index& i) -> bool
{
	auto sz = size(s);
	auto err = false;
	U8_APPEND(s, i, sz, CP, err);
	return !err;
}
auto encode_u8(char32_t CP, Str& s, Index i) -> std::pair<Index, bool>
{
	auto ok = encode_advance_u8(CP, s, i);
	return {i, ok};
}

// Writes to out, can check for space except for the first element i.e.
// out < last is precondition.
auto encode_u8(char32_t CP, OutIt out, OutIt last) -> std::pair<OutIt, bool>
{
	auto i = OutIt::difference_type();
	auto sz = distance(out, last);
	auto err = false;
	U8_APPEND(out, i, sz, CP, err);
	return {out + i, !err};
}

// writes to out, can not check for space
template <class OutIt>
auto encode_u8(char32_t CP, OutIt out) -> std::pair<OutIt, bool>
{
	auto i = typename iterator_traits<OutIt>::difference_type();
	auto sz = i + 4;
	auto err = false;
	U8_APPEND(out, i, sz, CP, err);
	return {out + i, !err};
}

class encoded_cp_u8_or_error {
	char a[4];

      public:
	encoded_cp_u8_or_error(char32_t cp = 0)
	{
		auto [it, ok] = encode_u8(cp, a);
		std::fill(it, end(a), '\0');
		if (!ok)
			a[0] = 0xFF;
	}

	auto error() const -> bool { return a[0] & 0xFF == 0xFF; }
	auto size() const -> size_t
	{
		int i = 0;
		U8_FWD_1(a, i, 4);
		return i;
	}
	operator string_view() const { return {a, size()}; }
};

/* =========== OPERATION: U8_APPEND_UNSAFE =============== */
auto encode_valid_cp_u8(char32_t CP, Str& s, Index i) -> Index
{
	U8_APPEND_UNSAFE(s, i, CP);
	return i;
}

template <class OutIt>
auto encode_valid_cp_u8(char32_t CP, OutIt out) -> OutIt
{
	auto i = typename iterator_traits<OutIt>::difference_type();
	U8_APPEND_UNSAFE(out, i, CP);
	return out + i;
}

class encoded_valid_cp_u8 {
	char a[4];

      public:
	encoded_valid_cp_u8(char32_t cp = 0)
	{
		auto it = encode_valid_cp_u8(cp, a);
		std::fill(it, end(a), '\0');
	}

	auto size() const -> size_t
	{
		int i = 0;
		U8_FWD_1_UNSAFE(a, i);
		return i;
	}
	operator string_view() const { return {a, size()}; }
};

/* =========== USAGE EXAMPLES =============== */
void u8_next_usage(Str& s)
{
	// u8_advance_i, index is inout parameter
	for (size_t i = 0; i != size(s);) {
		auto cp = u8_advance_i(s, i);
		// process cp
	}
	for (size_t i = 0; i != size(s);) {
		auto j = i;
		auto cp = u8_advance_i(s, j);
		auto cp_size = j - i;
		// process cp
		i = j;
	}
	for (size_t j = 0; j != size(s);) {
		auto i = j;
		auto cp = u8_advance_i(s, j);
		auto cp_size = j - i;
		// process cp
	}
	for (size_t i = 0, j = 0; i != size(s); i = j) {
		auto cp = u8_advance_i(s, j);
		auto cp_size = j - i;
		// process cp
	}

	// u8_next_i, index in return value
	for (size_t i = 0; i != size(s);) {
		code_point_or_error cpe;
		std::tie(i, cpe) = u8_next_i(s, i);
		// process cp
	}
	for (size_t i = 0; i != size(s);) {
		auto [j, cp] = u8_next_i(s, i);
		auto cp_size = j - i;
		// process cp
		i = j;
	}
	for (size_t i = 0, j = 0; i != size(s); i = j) {
		auto [jj, cp] = u8_next_i(s, i);
		j = jj;
		auto cp_size = j - i;
		// process cp
	}
	for (size_t i = 0, j = 0; i != size(s); i = j) {
		code_point_or_error cpe;
		std::tie(j, cpe) = u8_next_i(s, i);
		auto cp_size = j - i;
		// process cp
	}
}
