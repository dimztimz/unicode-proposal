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