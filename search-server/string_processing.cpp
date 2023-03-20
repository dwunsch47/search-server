#include "string_processing.h"
#include <string>

using namespace std;

vector<string_view> SplitIntoWords(string_view str) {
    vector<string_view> result;
    str.remove_prefix(min(str.size(), str.find_first_not_of(' ')));
    
    while (!str.empty()) {
        int64_t space = str.find(' ');
        result.push_back(str.substr(0, space));
        //result.push_back(space == pos_end ? str.substr(pos) : str.substr(pos, space - pos));
        str.remove_prefix(min(str.size(), str.find_first_not_of(" ", space)));
    }

    return result;
}
