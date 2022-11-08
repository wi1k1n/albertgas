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

long Util::angle2steps(const float& angle) {
    return angle * MOTOR_STEPS_PER_REVOLUTION / 360.f;
}

bool Util::stringIsFloat(const String& s) {
    auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
    bool pointCaptured = false;
    for (auto it = s.begin(); it != s.end(); it++) {
        bool isPoint = *it == '.';
        if (!isDigit(*it) && (!isPoint || pointCaptured))
            return false;
        pointCaptured |= isPoint;
    }
    return true;
}