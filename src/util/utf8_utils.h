#pragma once 
#include <string>


std::string safeUtf8Delta(const std::string& full, const std::string& accumulated) {
    if (accumulated.size() >= full.size()) return "";

    const std::string& new_part = full.substr(accumulated.size());
    size_t i = 0, last_good_pos = 0;

    while (i < new_part.size()) {
        unsigned char c = static_cast<unsigned char>(new_part[i]);
        size_t char_len = 1;

        if ((c & 0x80) == 0x00) char_len = 1;
        else if ((c & 0xE0) == 0xC0) char_len = 2;
        else if ((c & 0xF0) == 0xE0) char_len = 3;
        else if ((c & 0xF8) == 0xF0) char_len = 4;
        else break;

        if (i + char_len > new_part.size()) break;

        if (char_len == 3 &&
            static_cast<unsigned char>(new_part[i])     == 0xEF &&
            static_cast<unsigned char>(new_part[i + 1]) == 0xBF &&
            static_cast<unsigned char>(new_part[i + 2]) == 0xBD) {
            break;
        }

        last_good_pos = i + char_len;
        i += char_len;
    }

    return new_part.substr(0, last_good_pos);
}
