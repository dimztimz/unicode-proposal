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

For each operation there should be at least 3 functions (or function templates).

1. One that takes a range and index and returns new index.
2. One that takes pair of iterators and returns updated iterator.
3. One that takes a range and returns range or iterator.

Let the bikeshedding begin. I will now show some of the possible function
signatures for decoding one code point from UTF-8 string and encoding it to
UTF-8.

```cpp
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>

using namespace std;

// In the bellow functions consider Str and Size as template type parameters
using Str = string;
using Size = Str::size_type;
struct Err {};

/* ========================= RANGE + INDEX ========================= */
// This function has no out parameters, it returns tuple of
// (next index - one past the decoded CP, the CP, and possible error value).
// Precondition: s[i] must be valid, i.e. 0 <= i < size(s).
auto u8_next(const Str& s, Size i) -> tuple<Size, char32_t, Err>;

// Similar to the above but error is signaled by special value of the returned
// CP. Unicode code point needs 21 bits and char32_t has 32 so there enough
// space to signal all possible types of errors.
auto u8_next2(const Str& s, Size i)
    -> tuple<Size, char32_t>; // error is above 0x0010FFFF
auto u8_next3(const Str& s, Size i)
    -> tuple<Size, int32_t>; // use negative int for error as in ICU

// Question: Should we use struct instead of tuple? I prefer struct.
// For brevity I will write tuple here.

// These functions do not return, instead they use out parameters. The index is
// updated in place, it is in-out parameter.
void u8_next3(const Str& s, /*inout*/ Size& i, /*out*/ char32_t& CP,
              /*out*/ Err& e);
void u8_next4(const Str& s, /*inout*/ Size& i,
              /*out*/ char32_t& CP); // Error part of CP

// Maybe use mixture of return value and out parameters?
auto u8_next5(const Str& s, /*inout*/ Size& i) -> char32_t;
auto u8_next6(const Str& s, Size i, /*out*/ char32_t& CP) -> Size;

/* ========================= PAIR OF ITERATORS ========================= */
using Iter = Str::const_iterator;

auto u8_next(Iter first, Iter last) -> tuple<Iter, char32_t, Err>;
auto u8_next2(Iter first, Iter last)
    -> tuple<Iter, char32_t>; // Error part of CP, above 0x0010FFFF
auto u8_next3(Iter first, Iter last)
    -> tuple<Iter, int32_t>; // Error part of CP, negative number

// Mixed out-parameters and return value.
auto u8_next4(Iter first, Iter last, /*out*/ char32_t& CP) -> Iter;
auto u8_next5(/*inout*/ Iter& first, Iter last) -> char32_t;

// All out-parameters.
void u8_next6(/*inout*/ Iter& first, Iter last, /*out*/ char32_t& CP);

/* ========================= RANGES ========================= */
auto u8_next(const Str& s)
    -> tuple<ranges::borrowed_iterator_t<Str>, char32_t, Err>;
auto u8_next2(const Str& s)
    -> tuple<ranges::borrowed_iterator_t<Str>, char32_t>; // Error part of CP

// Mixed out-parameters and return value.
template <class Range>
auto u8_next3(/*inout*/ Range& rng) -> char32_t; // modifies rng.begin()
// Some of the concepts machinery should be applied here, e.g. borrowed_range.

// All out-parameters.
template <class Range>
void u8_next4(/*inout*/ Range& rng, /*out*/ char32_t& CP);

/* ====================== DECODING VALID UTF-8 ====================== */
// These functions receive only valid UTF-8 sequences. Therefore they do not
// need to know the size of the whole range and should be able to receive range
// or iterator. Obviously, there is no need to signal error.

using RangeOrIter = Str::const_iterator;
using Size2 = RangeOrIter::difference_type;

// The variant with range + index now can be iterator + index.
auto valid_u8_next(const RangeOrIter& rng, Size2 i) -> tuple<Size2, char32_t>;

// The variant with pair of iterators now is single iterator.
auto valid_u8_next(Iter it) -> tuple<Iter, char32_t>;
auto valid_u8_next(Iter it, /*out*/ char32_t& CP) -> Iter;
auto valid_u8_next(/*inout*/ Iter& it) -> char32_t;

/* ====================== ENCODING TO UTF-8 ====================== */

auto encode_u8(char32_t CP, Iter out)
    -> Iter; // writes to out, can not check for space
auto encode_u8(char32_t CP, Iter out, Iter last)
    -> Iter; // writes to out, can check for space

auto encode_u8(char32_t CP, Str& s, Size i)
    -> Size; // starts writing at s[i]. Checks for space;
```

Here, I would like to introduce a small abstraction, the idea of struct that
keeps one code point, but encoded in code units. Then, some algorithms
are much simpler to implement and are more efficient. See the code:

```cpp
#include <string_view>

using namespace std;

struct encoded_cp_u8 {
	char cp[4]; // Must be efficient at ABI level. Whole struct must be
	            // passed via registers.

	auto data() const -> const char* { return cp; }
	auto size() const -> size_t; // Calculate size from first byte.

	operator string_view() const { return {data(), size()}; }
};

struct encoded_cp_u16 {
	char16_t cp[2];

	auto data() const -> const char16_t* { return cp; }
	auto size() const -> size_t; // Calculate size from first code unit.

	operator u16string_view() const { return {data(), size()}; }
};

auto encode_u8(char32_t CP) -> encoded_cp_u8;

auto find_code_point(string& s, char32_t cp) -> size_t
{
	auto ecp = encode_u8(cp);
	return s.find(ecp);
}
```

So far this is enough. I would like to get some feedback. I'm interested which
function signatures are better.