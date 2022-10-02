#include <string>
#include <string_view>
#include <tuple>
#include <unicode/utf8.h>

using namespace std;

// In the bellow functions consider Str, Index, Iter as template type parameters
using Str = string;
using Index = Str::size_type;
struct Err {};

/* ========================= RANGE + INDEX ========================= */
struct code_point_and_error {
	int32_t a;

	// if error() == true, cp() will be unspecified value outside
	// of the Unicode range, i.e. cp() > 0x0010FFFF.
	char32_t cp() const { return a; }
	bool error() const { return a < 0; }
};
// TODO: maybe make this structure to look like std::optional or std::expected.
// We don't use them because it forces additional bool. ICU uses negative int
// to signal error in U8_NEXT.

auto u8_next1(const Str& s, Index& i) -> code_point_and_error
{
	code_point_and_error cpe;
	auto sz = size(s);
	U8_NEXT(s, i, sz, cpe.a);
	return cpe;
}

auto u8_next2(const Str& s, Index i) -> std::pair<Index, code_point_and_error>
{
	auto cpe = u8_next1(s, i);
	return {i, cpe};
}

// usage

void u8_next_usage(Str& s)
{
	// u8_next1, index is inout parameter
	for (size_t i = 0; i != size(s);) {
		auto cp = u8_next1(s, i);
		// process cp
	}
	for (size_t i = 0; i != size(s);) {
		auto j = i;
		auto cp = u8_next1(s, j);
		auto cp_size = j - i;
		// process cp
		i = j;
	}
	for (size_t j = 0; j != size(s);) {
		auto i = j;
		auto cp = u8_next1(s, j);
		auto cp_size = j - i;
		// process cp
	}
	for (size_t i = 0, j = 0; i != size(s); i = j) {
		auto cp = u8_next1(s, j);
		auto cp_size = j - i;
		// process cp
	}

	// u8_next2, index in return value
	for (size_t i = 0; i != size(s);) {
		code_point_and_error cpe;
		std::tie(i, cpe) = u8_next2(s, i);
		// process cp
	}
	for (size_t i = 0; i != size(s);) {
		auto [j, cp] = u8_next2(s, i);
		auto cp_size = j - i;
		// process cp
		i = j;
	}
	for (size_t i = 0, j = 0; i != size(s); i = j) {
		auto [jj, cp] = u8_next2(s, i);
		j = jj;
		auto cp_size = j - i;
		// process cp
	}
	for (size_t i = 0, j = 0; i != size(s); i = j) {
		code_point_and_error cpe;
		std::tie(j, cpe) = u8_next2(s, i);
		auto cp_size = j - i;
		// process cp
	}
}

auto valid_u8_next1(const Str& s, Index& i) -> char32_t
{
	char32_t c;
	U8_NEXT_UNSAFE(s, i, c);
	return c;
}
auto valid_u8_next2(const Str& s, Index i) -> std::pair<Index, char32_t>
{
	auto c = valid_u8_next1(s, i);
	return {i, c};
}
// valid_u8_next() should work with random access iterators too,
// or anything that is indexable. It does not need information about size.

/* ========================= PAIR OF ITERATORS ========================= */
using Iter = Str::const_iterator;

auto u8_next_it1(Iter& first, Iter last) -> code_point_and_error
{
	auto i = Iter::difference_type();
	auto sz = distance(first, last);
	code_point_and_error cpe;
	U8_NEXT(first, i, sz, cpe.a);
	advance(first, i);
	return cpe;
}

auto u8_next_it2(Iter first, Iter last) -> std::pair<Iter, code_point_and_error>
{
	auto i = Iter::difference_type();
	auto sz = distance(first, last);
	code_point_and_error cpe;
	U8_NEXT(first, i, sz, cpe.a);
	return {first + i, cpe};
}

auto valid_u8_next_it(Iter first) -> std::pair<Iter, char32_t>
{
	auto i = Iter::difference_type();
	char32_t c;
	U8_NEXT_UNSAFE(first, i, c);
	return {first + i, c};
}

/* ====================== ENCODING TO UTF-8 ====================== */

// starts writing at s[i]. Checks for space except for the first element s[i],
// i < size(s) is precondition.
auto encode_u8(char32_t CP, Str& s, Index i) -> std::pair<Index, bool>
{
	auto sz = size(s);
	auto err = false;
	U8_APPEND(s, i, sz, CP, err);
	return {i, !err};
}

using OutIt = Str::iterator;

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
auto encode_u8(char32_t CP, OutIt out) -> std::pair<OutIt, bool>
{
	auto i = OutIt::difference_type();
	auto sz = i + 4;
	auto err = false;
	U8_APPEND(out, i, sz, CP, err);
	return {out + i, !err};
}

auto encode_valid_cp_u8(char32_t CP, Str& s, Index i) -> Index
{
	U8_APPEND_UNSAFE(s, i, CP);
	return i;
}

auto encode_valid_cp_u8(char32_t CP, OutIt out) -> OutIt
{
	auto i = OutIt::difference_type();
	U8_APPEND_UNSAFE(out, i, CP);
	return out + i;
}
