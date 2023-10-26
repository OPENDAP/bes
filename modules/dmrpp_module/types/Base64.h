// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Dan Holloway <dholloway@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// This class based on implementation described at http://vorbrodt.blog/2019/03/23/base64-encoding
// and http://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64.

#include <stdexcept>
#include <string>
#include <vector>

namespace base64 {

#define BASE64_ENCODE_LOOKUP  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
constexpr const auto BASE64_PAD_CHARACTER = '=';

class Base64 {

public:
    static std::string encode(const u_int8_t *cursor, int32_t size) {
        char encodeLookup[] = BASE64_ENCODE_LOOKUP;
        char padCharacter = BASE64_PAD_CHARACTER;
        std::string encodedString;
        encodedString.reserve(((size / 3) + (size % 3 > 0)) * 4);
        u_int64_t temp;

        for (int32_t idx = 0; idx < size / 3; idx++) {
            temp = (*cursor++) << 16;
            temp += (*cursor++) << 8;
            temp += (*cursor++);
            encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
            encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
            encodedString.append(1, encodeLookup[(temp & 0x00000FC0) >> 6]);
            encodedString.append(1, encodeLookup[(temp & 0x0000003F)]);
        }

        switch (size % 3) {
            case 1:
                temp = (*cursor++) << 16;
                encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
                encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
                encodedString.append(2, padCharacter);
                break;
            case 2:
                temp = (*cursor++) << 16;
                temp += (*cursor++) << 8;
                encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
                encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
                encodedString.append(1, encodeLookup[(temp & 0x00000FC0) >> 6]);
                encodedString.append(1, padCharacter);
                break;
        }
        return encodedString;
    }

    static std::vector<u_int8_t> decode(const std::string &input) {
        char padCharacter = BASE64_PAD_CHARACTER;

        if (input.size() % 4) //Sanity check
            throw std::runtime_error("Non-Valid base64!");

        size_t padding = 0;
        if (!input.empty()) {
            if (input[input.size() - 1] == padCharacter)
                padding++;
            if (input[input.size() - 2] == padCharacter)
                padding++;
        }

        std::vector<u_int8_t> decodedBytes;
        decodedBytes.reserve(((input.size() / 4) * 3) - padding);

        u_int32_t temp = 0;

        std::string::const_iterator cursor = input.begin();
        while (cursor < input.end()) {
            for (size_t quantumPosition = 0; quantumPosition < 4; quantumPosition++) {
                temp <<= 6;
                if (*cursor >= 0x41 && *cursor <= 0x5A) // This area will need tweaking if
                    temp |= *cursor - 0x41;             // you are using an alternate alphabet
                else if (*cursor >= 0x61 && *cursor <= 0x7A)
                    temp |= *cursor - 0x47;
                else if (*cursor >= 0x30 && *cursor <= 0x39)
                    temp |= *cursor + 0x04;
                else if (*cursor == 0x2B)
                    temp |= 0x3E; //change to 0x2D for URL alphabet
                else if (*cursor == 0x2F)
                    temp |= 0x3F; //change to 0x5F for URL alphabet
                else if (*cursor == padCharacter) //pad
                {
                    switch (input.end() - cursor) {
                        case 1: //One pad character
                            decodedBytes.push_back((temp >> 16) & 0x000000FF);
                            decodedBytes.push_back((temp >> 8) & 0x000000FF);
                            return decodedBytes;
                        case 2: //Two pad characters
                            decodedBytes.push_back((temp >> 10) & 0x000000FF);
                            return decodedBytes;
                        default:
                            throw std::runtime_error("Invalid Padding in Base 64!");
                    }
                }
                else
                    throw std::runtime_error("Non-Valid Character in Base 64!");

                cursor++;
            }
            decodedBytes.push_back((temp >> 16) & 0x000000FF);
            decodedBytes.push_back((temp >> 8) & 0x000000FF);
            decodedBytes.push_back((temp) & 0x000000FF);
        }

        return decodedBytes;
    }
};

}   // namespace base64
