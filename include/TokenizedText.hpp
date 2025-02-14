#pragma once

#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Create a setNAME method that reads from the name##Tokens field and sets all tokens listed.
#define SET_TOKEN(name) \
void set_##name(std::string_view name) { \
    invalidate_text(); \
    for (int idx : name##Tokens) \
        tokens[idx] = name; \
} \

class TokenizedText {
   public:
    TokenizedText() = default;
    bool operator==(TokenizedText const&) const = default;

    TokenizedText(std::string str) {
        original = str;
        // Parse the string into tokens, converting the string back is easy.
        std::stringstream nonPercentStr;
        int i = 0;
        bool isPercent = false;
        for (char const current : str) {
            if (isPercent) {
                std::string buffer;
                if (current == 'n')
                    tokens.emplace_back("\n");
                else if (current == '%')
                    tokens.emplace_back("%");
                else {
                    switch (current) {
                        case 'b':
                            beforeCutTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 'c':
                            accuracyTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 'a':
                            afterCutTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 't':
                            timeDependencyTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 'B':
                            beforeCutSegmentTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 'C':
                            accuracySegmentTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 'A':
                            afterCutSegmentTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 'T':
                            timeDependencySegmentTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 's':
                            scoreTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 'p':
                            percentTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        case 'd':
                            directionTokens.push_back(i);
                            tokens.emplace_back("");
                            break;
                        default:
                            // keep % when it doesn't correspond to a key
                            auto str = std::string("%") + current;
                            tokens.push_back(str);
                            isPercent = false;
                            i++;
                            continue;
                    }
                }
                isPercent = false;
                i++;
                continue;
            } else if (current == '%') {
                tokens.emplace_back(nonPercentStr.str());
                nonPercentStr.str(std::string());
                isPercent = true;
                i++;
            } else
                nonPercentStr.put(current);
        }
        if (nonPercentStr.str().size() != 0)
            tokens.emplace_back(nonPercentStr.str());
    }

    std::string Raw() { return original; }
    // Get the token-joined string from creation of this
    std::string Join() {
        if (!textValid) {
            textValid = true;
            text = std::string();
            for (auto const& piece : tokens)
                text += piece;
        }
        return text;
    }

    void invalidate_text() { textValid = false; }

    bool is_text_valid() { return textValid; }

    SET_TOKEN(beforeCut)
    SET_TOKEN(accuracy)
    SET_TOKEN(afterCut)
    SET_TOKEN(timeDependency)
    SET_TOKEN(percent)
    SET_TOKEN(beforeCutSegment)
    SET_TOKEN(accuracySegment)
    SET_TOKEN(afterCutSegment)
    SET_TOKEN(timeDependencySegment)
    SET_TOKEN(score)
    SET_TOKEN(direction)

    std::string original;
    std::vector<std::string> tokens;

   private:
    // Is cached text valid? Should be invalidated on tokens change
    bool textValid = false;
    // Cached text, should be invalidated on tokens change
    std::string text;

    std::vector<int> beforeCutTokens;
    std::vector<int> accuracyTokens;
    std::vector<int> afterCutTokens;
    std::vector<int> timeDependencyTokens;
    std::vector<int> percentTokens;
    std::vector<int> beforeCutSegmentTokens;
    std::vector<int> accuracySegmentTokens;
    std::vector<int> afterCutSegmentTokens;
    std::vector<int> timeDependencySegmentTokens;
    std::vector<int> scoreTokens;
    std::vector<int> directionTokens;
};

#undef SET_TOKEN
