#pragma once

#include <jni.h>
#include <string>
#include <vector>

// 1 - 8 for corresponding note in a major scale, -1 for rest
// Converted to semitones where 0 is root and 12 is an octave up
typedef int Pitch;

class TextParser {
public:
    TextParser();
    ~TextParser();

    std::vector<Pitch> parse(std::string input);
};
