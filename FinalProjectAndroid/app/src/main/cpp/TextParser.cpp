//
// Created by joels on 4/14/2024.
//

#include "TextParser.h"

TextParser::TextParser() {}
TextParser::~TextParser() {}

std::vector<Pitch> TextParser::parse(std::string input) {
    const size_t numNotes = input.size();
    std::vector<Pitch> sequence(numNotes);

    for (char c : input) {
        if (c == 'x') {
            sequence.push_back(-1);
        } else {
            const int num = static_cast<int>(c) - static_cast<int>('0');
            if (num < 1 || num > 8) {
                sequence.push_back(num);
            } else {
                // ERROR
                return std::vector<Pitch>();
            }
        }
    }

    return sequence;
}
