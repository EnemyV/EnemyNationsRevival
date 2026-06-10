// test_ai_paths.cpp
//
// Source-lint for the AI research-path tables in caigmgr.cpp. These are
// hand-maintained arrays (one per developer: Cbt/Eric/Steve/Keith) and the
// known failure mode is copy-paste drift: every live path shipped for ~30
// years with `range_1` listed twice and `range_2` missing, deferring weapon-
// range tiers 2-3 to the post-path fallback sweep (after jets + telephones).
// The rejected aiDavePath was thrown out for "6 duplicates and one dead end"
// -- duplicates in a research path are always errors (topics research once).
//
// Usage: ai_path_tests.exe <path-to-caigmgr.cpp>
// Exit:  0 all pass, 1 failures, 2 usage/cannot-open (skip)

#define _CRT_SECURE_NO_WARNINGS

#include "microtest.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

// read whole file
std::string ReadAll(const char* path) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return std::string();
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::string s((size_t)(n > 0 ? n : 0), '\0');
    if (n > 0 && std::fread(&s[0], 1, (size_t)n, f) != (size_t)n) s.clear();
    std::fclose(f);
    return s;
}

// extract the CRsrchArray:: tokens of `name`'s initializer
std::vector<std::string> PathTokens(const std::string& src, const char* name) {
    std::vector<std::string> toks;
    std::string decl = std::string(name) + "[CRsrchArray::num_types]";
    size_t at = src.find(decl);
    if (at == std::string::npos) return toks;
    size_t end = src.find("};", at);
    if (end == std::string::npos) return toks;

    const std::string tag = "CRsrchArray::";
    size_t p = src.find(tag, at);
    while (p != std::string::npos && p < end) {
        size_t q = p + tag.size();
        size_t r = q;
        while (r < end && (isalnum((unsigned char)src[r]) || src[r] == '_')) ++r;
        std::string t = src.substr(q, r - q);
        if (t != "num_types") toks.push_back(t);
        p = src.find(tag, r);
    }
    return toks;
}

bool Contains(const std::vector<std::string>& v, const char* t) {
    for (size_t i = 0; i < v.size(); ++i) if (v[i] == t) return true;
    return false;
}

void CheckPath(const std::string& src, const char* name) {
    std::vector<std::string> toks = PathTokens(src, name);
    std::printf("[ai_paths] %s: %d topics\n", name, (int)toks.size());
    CHECK(!toks.empty());

    // research topics are researched once -- a duplicate is always an error
    // (the dupe never fires and whatever it displaced is missing)
    for (size_t a = 0; a < toks.size(); ++a)
        for (size_t b = a + 1; b < toks.size(); ++b) {
            if (toks[a] == toks[b])
                std::printf("[ai_paths] %s duplicate: %s\n", name, toks[a].c_str());
            CHECK(toks[a] != toks[b]);
        }

    // the weapon-upgrade tier chains must be complete (range_2 was the one
    // missing for 30 years); landing_craft must be present or sea invasions
    // can never field landers on that path
    CHECK(Contains(toks, "range_1"));
    CHECK(Contains(toks, "range_2"));
    CHECK(Contains(toks, "range_3"));
    CHECK(Contains(toks, "landing_craft"));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: ai_path_tests <caigmgr.cpp>\n"); return 2; }
    std::string src = ReadAll(argv[1]);
    if (src.empty()) { std::printf("SKIP: cannot read %s\n", argv[1]); return 2; }

    CheckPath(src, "aiCbtPath");
    CheckPath(src, "aiEricPath");
    CheckPath(src, "aiStevePath");
    CheckPath(src, "aiKeithPath");
    // aiDavePath is intentionally not checked: it is dead (commented out),
    // rejected by the original developers for exactly these defects.

    return microtest::Summary();
}
