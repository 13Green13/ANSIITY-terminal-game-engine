#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdexcept>

// ─── Minimal JSON value extraction ─────────────────────────────────
// Not a full parser — just enough to pull fields from flat/nested JSON
// returned by our Flask server.

namespace json {

// Helper: advance index past a JSON string (handles escaped quotes)
inline void skipString(const std::string& json, size_t& i) {
    // i points at opening '"'
    i++; // skip opening quote
    while (i < json.size()) {
        if (json[i] == '\\') { i += 2; continue; } // skip escaped char
        if (json[i] == '"') { i++; return; }        // closing quote
        i++;
    }
}

// Find a string value for a top-level key: "key": "value"
// Skips matches inside nested objects/arrays and inside string values
inline std::string getString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    int depth = 0;
    for (size_t i = 0; i < json.size(); ) {
        char c = json[i];
        // Skip string literals to avoid matching keys inside values
        if (c == '"') {
            // Check if this is our key at depth 1
            if (depth == 1 && json.compare(i, search.size(), search) == 0) {
                size_t pos = json.find(':', i + search.size());
                if (pos == std::string::npos) return "";
                pos = json.find('"', pos + 1);
                if (pos == std::string::npos) return "";
                // Extract value string (handle escaped quotes)
                size_t start = pos + 1;
                size_t end = start;
                while (end < json.size()) {
                    if (json[end] == '\\') { end += 2; continue; }
                    if (json[end] == '"') break;
                    end++;
                }
                return json.substr(start, end - start);
            }
            skipString(json, i);
            continue;
        }
        if (c == '{' || c == '[') { depth++; i++; continue; }
        if (c == '}' || c == ']') { depth--; i++; continue; }
        i++;
    }
    return "";
}

// Find a numeric value for a top-level key: "key": 123
inline int getInt(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    int depth = 0;
    for (size_t i = 0; i < json.size(); ) {
        char c = json[i];
        if (c == '"') {
            if (depth == 1 && json.compare(i, search.size(), search) == 0) {
                size_t pos = json.find(':', i + search.size());
                if (pos == std::string::npos) return 0;
                pos++;
                while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
                try { return std::stoi(json.substr(pos)); }
                catch (...) { return 0; }
            }
            skipString(json, i);
            continue;
        }
        if (c == '{' || c == '[') { depth++; i++; continue; }
        if (c == '}' || c == ']') { depth--; i++; continue; }
        i++;
    }
    return 0;
}

// Find a float value for a top-level key
inline float getFloat(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    int depth = 0;
    for (size_t i = 0; i < json.size(); ) {
        char c = json[i];
        if (c == '"') {
            if (depth == 1 && json.compare(i, search.size(), search) == 0) {
                size_t pos = json.find(':', i + search.size());
                if (pos == std::string::npos) return 0;
                pos++;
                while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
                try { return std::stof(json.substr(pos)); }
                catch (...) { return 0; }
            }
            skipString(json, i);
            continue;
        }
        if (c == '{' || c == '[') { depth++; i++; continue; }
        if (c == '}' || c == ']') { depth--; i++; continue; }
        i++;
    }
    return 0;
}

// Extract a JSON object {...} starting at or after `start`
inline std::string getObject(const std::string& json, size_t start = 0) {
    if (start == std::string::npos) return "";
    size_t pos = json.find('{', start);
    if (pos == std::string::npos) return "";
    int depth = 0;
    for (size_t i = pos; i < json.size(); i++) {
        if (json[i] == '"') { skipString(json, i); i--; continue; } // i-- because loop does i++
        if (json[i] == '{') depth++;
        else if (json[i] == '}') { depth--; if (depth == 0) return json.substr(pos, i - pos + 1); }
    }
    return "";
}

// Extract a JSON array [...] for a key
inline std::string getArray(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find('[', pos + search.size());
    if (pos == std::string::npos) return "";
    int depth = 0;
    for (size_t i = pos; i < json.size(); i++) {
        if (json[i] == '[') depth++;
        else if (json[i] == ']') { depth--; if (depth == 0) return json.substr(pos, i - pos + 1); }
    }
    return "";
}

// Split an array of objects into individual object strings
inline std::vector<std::string> splitArrayObjects(const std::string& arr) {
    std::vector<std::string> result;
    size_t pos = 0;
    while (true) {
        std::string obj = getObject(arr, pos);
        if (obj.empty()) break;
        result.push_back(obj);
        pos = arr.find(obj, pos) + obj.size();
    }
    return result;
}

// Split an array of integers: [20, 35, 50]
inline std::vector<int> splitArrayInts(const std::string& arr) {
    std::vector<int> result;
    size_t i = 0;
    while (i < arr.size()) {
        if (arr[i] == '-' || (arr[i] >= '0' && arr[i] <= '9')) {
            try { result.push_back(std::stoi(arr.substr(i))); }
            catch (...) { break; }
            while (i < arr.size() && (arr[i] == '-' || (arr[i] >= '0' && arr[i] <= '9'))) i++;
        } else {
            i++;
        }
    }
    return result;
}

} // namespace json
