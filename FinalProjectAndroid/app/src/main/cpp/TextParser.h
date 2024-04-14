#pragma once

#include <jni.h>
#include <string>
#include <vector>

typedef int Pitch; // 1 - 8 for corresponding note in a major scale, -1 for rest

class TextParser {
public:
    TextParser();
    ~TextParser();

    std::vector<Pitch> parse(std::string input);
};
