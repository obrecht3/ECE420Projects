//
// Created by joels on 4/14/2024.
//

#include "TextParser.h"

TextParser::TextParser() {}
TextParser::~TextParser() {}

std::vector<Pitch> TextParser::parse(std::string input) {
    const size_t numNotes = input.size();
    std::vector<Pitch> sequence;
    sequence.reserve(numNotes);

    for (char c : input) {
        if (c == 'x' || c == 'X') {
            sequence.push_back(-1);
        } else {
            if (c >= '1' && c <= '8') {
                int num = -1;
                switch (c) {
                    case '1':
                        num = 0; // c
                        break;
                    case '2':
                        num = 2; // d
                        break;
                    case '3':
                        num = 4; // e
                        break;
                    case '4':
                        num = 5; // f
                        break;
                    case '5':
                        num = 7; // g
                        break;
                    case '6':
                        num = 9; // a
                        break;
                    case '7':
                        num = 11; // b
                        break;
                    case '8':
                        num = 12; // c +oct
                        break;
                }
                sequence.push_back(num);
            }
        }
    }

    return sequence;
}
