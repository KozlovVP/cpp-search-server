#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <list>

std::vector<std::string_view> SplitIntoWords(const std::string_view& text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
	using namespace std;
    set<string, less<>> non_empty_strings;
    for (const string_view& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(static_cast<string>(str));
        }
    }
    return non_empty_strings;
}
