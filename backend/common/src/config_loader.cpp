#include "swift/config_loader.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifndef _WIN32
#include <unistd.h>  // environ
#endif

namespace swift {

namespace {

void Trim(std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    auto end = s.find_last_not_of(" \t\r\n");
    s = s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
}

}  // namespace

std::string KeyValueConfig::ToLowerKey(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

void KeyValueConfig::LoadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        return;
    std::string line;
    while (std::getline(f, line)) {
        Trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        Trim(key);
        Trim(value);
        if (key.empty())
            continue;
        map_[ToLowerKey(key)] = value;
    }
}

void KeyValueConfig::ApplyEnvOverrides(const std::string& env_prefix) {
    std::string prefix_upper = env_prefix;
    std::transform(prefix_upper.begin(), prefix_upper.end(), prefix_upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    size_t plen = prefix_upper.size();

#ifndef _WIN32
    for (char** p = environ; *p != nullptr; ++p) {
        std::string env(*p);
        size_t eq = env.find('=');
        if (eq == std::string::npos)
            continue;
        std::string name = env.substr(0, eq);
        std::string value = env.substr(eq + 1);
        std::string name_upper = name;
        std::transform(name_upper.begin(), name_upper.end(), name_upper.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        if (name_upper.size() <= plen || name_upper.compare(0, plen, prefix_upper) != 0)
            continue;
        std::string key = name_upper.substr(plen);
        map_[ToLowerKey(key)] = value;
    }
#endif
}

void KeyValueConfig::Load(const std::string& path, const std::string& env_prefix) {
    LoadFile(path);
    ApplyEnvOverrides(env_prefix);
}

bool KeyValueConfig::Has(const std::string& key) const {
    return map_.find(ToLowerKey(key)) != map_.end();
}

std::string KeyValueConfig::Get(const std::string& key, const std::string& default_val) const {
    auto it = map_.find(ToLowerKey(key));
    if (it == map_.end())
        return default_val;
    return it->second;
}

int KeyValueConfig::GetInt(const std::string& key, int default_val) const {
    std::string v = Get(key, "");
    if (v.empty())
        return default_val;
    try {
        return std::stoi(v);
    } catch (...) {
        return default_val;
    }
}

int64_t KeyValueConfig::GetInt64(const std::string& key, int64_t default_val) const {
    std::string v = Get(key, "");
    if (v.empty())
        return default_val;
    try {
        return std::stoll(v);
    } catch (...) {
        return default_val;
    }
}

bool KeyValueConfig::GetBool(const std::string& key, bool default_val) const {
    std::string v = Get(key, "");
    if (v.empty())
        return default_val;
    std::string lower = ToLowerKey(v);
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on")
        return true;
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off")
        return false;
    return default_val;
}

KeyValueConfig LoadKeyValueConfig(const std::string& path, const std::string& env_prefix) {
    KeyValueConfig cfg;
    cfg.Load(path, env_prefix);
    return cfg;
}

}  // namespace swift
