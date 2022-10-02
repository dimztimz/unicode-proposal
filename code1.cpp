#include <ranges>
#include <string>
#include <string_view>
#include <tuple>

using namespace std;

// In the bellow functions consider Str and Size as template type parameters
using Str = string;
using Index = Str::size_type;
struct Err {};

/* ========================= RANGE + INDEX ========================= */
// This function has no out parameters, it returns tuple of
// (next index - one past the decoded CP, the CP, and possible error value).
// Precondition: s[i] must be valid, i.e. 0 <= i < size(s).
auto u8_next(const Str& s, Index i) -> tuple<Index, char32_t, Err>;

// Similar to the above but error is signaled by special value of the returned
// CP. Unicode code point needs 21 bits and char32_t has 32 so there enough
// space to signal all possible types of errors.
auto u8_next2(const Str& s, Index i)
    -> tuple<Index, char32_t>; // error is above 0x0010FFFF
auto u8_next3(const Str& s, Index i)
    -> tuple<Index, int32_t>; // use negative int for error as in ICU

// Question: Should we use struct instead of tuple? I prefer struct.
// For brevity I will write tuple here.

// These functions do not return, instead they use out parameters. The index is
// updated in place, it is in-out parameter.
void u8_next3(const Str& s, /*inout*/ Index& i, /*out*/ char32_t& CP,
              /*out*/ Err& e);
void u8_next4(const Str& s, /*inout*/ Index& i,
              /*out*/ char32_t& CP); // Error part of CP

// Maybe use mixture of return value and out parameters?
auto u8_next5(const Str& s, /*inout*/ Index& i) -> char32_t;
auto u8_next6(const Str& s, Index i, /*out*/ char32_t& CP) -> Index;

/* ======================RANGE + INDEX ALTERNATIVES ===================== */

// return the size of the CP as uint8_t because it is value between 1 and 4
using CPSize = uint8_t;

auto u8_next7(const Str& s, Index i) -> tuple<char32_t, CPSize>;

// return both the updated index and the size of the CP
auto u8_next8(const Str& s, Index i) -> tuple<Index, char32_t, uint8_t>;

/* ========================= PAIR OF ITERATORS ========================= */
using Iter = Str::const_iterator;

auto u8_next(Iter first, Iter last) -> tuple<Iter, char32_t, Err>;
auto u8_next2(Iter first, Iter last)
    -> tuple<Iter, char32_t>; // Error part of CP, above 0x0010FFFF
auto u8_next3(Iter first, Iter last)
    -> tuple<Iter, int32_t>; // Error part of CP, negative number

auto u8_next4(Iter first, Iter last)
    -> tuple<Iter, char32_t, CPSize>; // Maybe return CP size here too?

// Mixed out-parameters and return value.
auto u8_next5(Iter first, Iter last, /*out*/ char32_t& CP) -> Iter;
auto u8_next6(/*inout*/ Iter& first, Iter last) -> char32_t;

// All out-parameters.
void u8_next7(/*inout*/ Iter& first, Iter last, /*out*/ char32_t& CP);

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
using Index2 = RangeOrIter::difference_type;

// The variant with range + index now can be iterator + index.
auto valid_u8_next(const RangeOrIter& rng, Index2 i) -> tuple<Index2, char32_t>;

// The variant with pair of iterators now is single iterator.
auto valid_u8_next(Iter it) -> tuple<Iter, char32_t>;
auto valid_u8_next(Iter it, /*out*/ char32_t& CP) -> Iter;
auto valid_u8_next(/*inout*/ Iter& it) -> char32_t;

/* ====================== ENCODING TO UTF-8 ====================== */

auto encode_u8(char32_t CP, Iter out)
    -> Iter; // writes to out, can not check for space
auto encode_u8(char32_t CP, Iter out, Iter last)
    -> Iter; // writes to out, can check for space

auto encode_u8(char32_t CP, Str& s, Index i)
    -> Index; // starts writing at s[i]. Checks for space;