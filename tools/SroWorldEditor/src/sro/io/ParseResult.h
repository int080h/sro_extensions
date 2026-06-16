#pragma once
#include <string>

namespace sro {

struct ParseResult {
    bool ok = false;
    std::string error;
    std::wstring path;

    static ParseResult Success(const std::wstring& p = {}) {
        ParseResult r;
        r.ok = true;
        r.path = p;
        return r;
    }

    static ParseResult Fail(const std::string& err, const std::wstring& p = {}) {
        ParseResult r;
        r.ok = false;
        r.error = err;
        r.path = p;
        return r;
    }
};

} // namespace sro
