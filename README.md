# Proposal for low-level Unicode decoding and encoding

I propose that in the library we should standardize low-level facilities for 
transformations between Unicode encodings. Low-level means that the proposal
generally does not introduce new types. It mostly contains functions and
function templates that only work with existing ranges, strings, iterators and
indexes. It is only for UTF-8, UTF-16 and UTF-32 and nothing else.

My opinion is that this should be done before standardizing high-level
facilities (that introduce new types) like `std::text` or Unicode iterators.

The functions will fall in four categories:

1. **Functions that decode/encode only one code point.**
   ICU has this functionality in [utf8.h] and [utf16.h].
   but it is implemented as macros that are not type-safe.
2. **Functions that take the full input and write to fixed-size range.**
   ICU has lot of such functions. Some examples are in header [ustring.h],
   for example [u_strToUTF8] or [u_strFromUTF8]. If the output  does not fit in
   the given output range they return error and return the size needed for the
   complete output.
3. **Functions that take the full input and write to resizable range.**
   Example in ICU is [icu::UnicodeString::fromUTF8].
4. **Functions that take input in chunks and write in chunks.**
   They keep a state between calls. Examples are [ucnv_fromUnicode] and
   [ucnv_toUnicode]. `std::codecvt` is also such API, but it is hard to use.
   The problem with `codecvt` comes from the return value `partial`. If that is
   returned, one has to do additional complicated checks to see if the input
   has incomplete sequence at the end of the chunk or the output chunk has no
   more space. Additionally, `codevt` may or may not save the few incomplete
   input bytes at the end into the state, the standard makes no guarantees.
   
[utf8.h]: https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/utf8_8h.html
[utf16.h]: https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/utf16_8h.html
[ustring.h]: https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/ustring_8h.html
[u_strToUTF8]: https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/ustring_8h.html#a69430352fe5439927f48b98b209939d7
[u_strFromUTF8]: https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/ustring_8h.html#a706408344cfd478cac2a7413c954aba6
[icu::UnicodeString::fromUTF8]: https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/classicu_1_1UnicodeString.html#a71c230712cdace1eefe4b2497e964788
[ucnv_fromUnicode]: https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/ucnv_8h.html#aa820d3bc3942522eb31bdb5b8ae73727
[ucnv_toUnicode]: https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/ucnv_8h.html#a9451f05be7b1b75832d5ec55b4e6d67f

I will now focus on the first group of functions.

## Functions that decode/encode only one code point

This group contains functions that do the following operations:

- decode the next code point
- decode the previous code point
- move the index/iterator to the next code point without decoding it
- move the index/iterator to the previous code point without decoding it
- adjust index/iterator to point the start of the code point
- encode a code point into some range
- truncate the end of a range (move end iterator) if it contains incomplete
  sequence of code point (incomplete means the sequence seen so far is valid,
  the rest if its bytes maybe are in a different chunk).
  
See ICU header [utf8.h] for all details.

Almost all operations come in two flavors, one that can accept invalid
sequences and another that must have valid sequences. We will consider these
flavors as separate operations.

For each operation there should be at least 2 functions (or function templates).

1. One that takes a string (or more generally, a range) and index and returns
   new index.
2. One that takes pair of iterators and returns updated iterator.

I will now show few examples how such functions can be defined and implemented
using ICU.

```cpp
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
```

Looking at the above functions, I can ask few questions:

1. How should the error be reported in `u8_next`? These are the options I have
   considered:

   1. Use `int32_t` as type for code point in `u8_next` and return negative
      value, exactly the same as ICU.
   2. Use `char32_t` as type for code point in `u8_next` and return unspecified
      high value above the Unicode range, i.e. casting the negative int from ICU
      to the unsigned `char32_t`.
   3. Use separate variable with its own type (some enum like `std::errc`).
   4. Abstract the error in lightweight "sum" type `code_point_or_error` that
      resembles `std::expected` or `std::optional` but that can be implemented
      with just a single `char32_t` or with `int32_t`. `std::expected` and
      `std::optional` must use additional bool internally.
   5. Use single sentinel value for error, signed -1 or unsigned 0xFFFFFFFF.
      I noticed the exact implementation of `U8_NEXT` in ICU and it relies
      on `U8_INTERNAL_NEXT_OR_SUB` and uses exactly -1 to signal error, and not
      any other negative value. The sentinel can be even special type with
      overloaded `operator==`.
   6. Use slightly more type-safe variant of number 5, define an enum as a
      strong typedef for `char32_t`.

      ```cpp
      enum class code_point_or_error: char32_t {
        error = char32_t(-1)
      };
      ```

   For now I decided for the solution number 5, to use sentinel value. It goes
   with the idea of being low-level and not introducing new types.
2. Should the size of the encoded sequence of the code point be returned?
   Probably no. In the examples above I just subtract indexes to get that
   information.
3. Should we use return values or out-parameters? For the code point and error
   definitely use return value, but for the index or iterator both APIs make
   sense. Those parameters are actually in-out not just out. The standard
   library rarely uses out-parameters. Of all the algorithms with iterators,
   only `std::advance()` uses that.
4. Should the functions with string and index be generic or should they work
   only with `string_view`? Probably they should be generic as there are
   multiple character types that can be used for one encoding, e.g. `char` and
   `char8_t` for UTF-8. Then we can ask should they be generic for any range
   or just for `basic_string_view`? My vote is fully generic, as that way they
   will work with plain old arrays with known size too.
