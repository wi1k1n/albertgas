#ifndef UTIL_H__
#define UTIL_H__

#include <Arduino.h>
#include <vector>
#include <set>
#include <functional>

#include "config.h"

class Util {
    static void _tokenize(String txt, char separator, std::function<void(const String& token)> cb, bool includeEmpty = false);
public:
    static void tokenizeUnique(String txt, char separator, std::set<String>& dst, bool includeEmpty = false);
    static void tokenizeNonUnique(String txt, char separator, std::vector<String>& dst, bool includeEmpty = false);
    
    static long angle2steps(const float& angle);
    static bool stringIsFloat(const String& s);
};

#endif // UTIL_H__