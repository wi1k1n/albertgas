#include "config.h"
#include "util.h"

void Util::_tokenize(String txt, char separator, std::function<void(const String& token)> cb, bool includeEmpty) {
    String curToken;
    for (auto it = txt.begin(); it != txt.end(); it++) {
        if (*it == separator) {
            if (includeEmpty && curToken.isEmpty())
                continue;
            cb(curToken);
            curToken = String();
            continue;
        }
        curToken += *it;
    }
    if (!curToken.isEmpty() || includeEmpty) {
        cb(curToken);
    }
}
void Util::tokenizeUnique(String txt, char separator, std::set<String>& dst, bool includeEmpty) {
    _tokenize(txt, separator, [&](const String& token) { dst.insert(token); }, includeEmpty);
}
void Util::tokenizeNonUnique(String txt, char separator, std::vector<String>& dst, bool includeEmpty) {
    _tokenize(txt, separator, [&](const String& token) { dst.push_back(token); }, includeEmpty);
}