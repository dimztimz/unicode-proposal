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

/* =========== OPERATION: U8_NEXT =============== */

constexpr char32_t cp_error = -1;

auto u8_advance_i(const Str& s, Index& i) -> char32_t
{
	char32_t cp;
	auto sz = size(s);
	U8_NEXT(s, i, sz, cp);
	return cp;
}
auto u8_next_i(const Str& s, Index i) -> std::pair<Index, char32_t>
{
	auto cp = u8_advance_i(s, i);
	return {i, cp};
}
auto u8_advance_it(Iter& first, Iter last) -> char32_t
{
	auto i = Iter::difference_type();
	auto sz = distance(first, last);
	char32_t cp;
	U8_NEXT(first, i, sz, cp);
	advance(first, i);
	return cp;
}
auto u8_next_it(Iter first, Iter last) -> std::pair<Iter, char32_t>
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
		char32_t cp;
		std::tie(i, cp) = u8_next_i(s, i);
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
		char32_t cp;
		std::tie(j, cp) = u8_next_i(s, i);
		auto cp_size = j - i;
		// process cp
	}
}

auto find_cp_faster(const string& s, char32_t cp) -> size_t
{
	char enc_cp[4];
	auto [it, ok] = encode_u8(cp, enc_cp);
	if (!ok)
		return s.npos;
	auto sv = string_view(enc_cp, it - enc_cp);
	return s.find(sv);
}

auto find_cp_slower(const string& s, char32_t cp) -> size_t
{
	for (size_t i = 0, j = 0; i != size(s); i = j) {
		auto [jj, dec_cp] = u8_next_i(s, i);
		j = jj;
		if (dec_cp == cp_error)
			continue;
		else if (dec_cp == cp)
			return i;
	}
	return s.npos;
}
