#pragma once

#include "TokenizedText.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector3.hpp"
#include "config-utils/shared/config-utils.hpp"

namespace HSV {
    // why cordl why
    struct EquatableColor : ConfigUtils::Color {
        bool operator==(EquatableColor const& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; };
        EquatableColor(UnityEngine::Color const& rhs) : ConfigUtils::Color(rhs) {}
        EquatableColor() = default;
    };

    struct EquatableVec3 : ConfigUtils::Vector3 {
        bool operator==(EquatableVec3 const& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; };
        EquatableVec3(UnityEngine::Vector3 const& rhs) : ConfigUtils::Vector3(rhs) {}
        EquatableVec3() = default;
    };

    DECLARE_JSON_STRUCT(Judgement) {
       private:
        NAMED_VALUE(std::string, UnprocessedText, "text");
        NAMED_VECTOR(float, UnprocessedColor, "color");

       public:
        DESERIALIZE_FUNCTION(Parsing) {
            Text = TokenizedText(UnprocessedText);
            Color = UnityEngine::Color(UnprocessedColor[0], UnprocessedColor[1], UnprocessedColor[2], UnprocessedColor[3]);
        };

        NAMED_VALUE_DEFAULT(int, Threshold, 0, "threshold");
        NAMED_VALUE_OPTIONAL(bool, Fade, "fade");
        TokenizedText Text;
        EquatableColor Color;

        Judgement(int threshold, std::string text, UnityEngine::Color color, bool fade = false) :
            Threshold(threshold),
            Text(text),
            Color(color),
            Fade(fade) {}
        Judgement() = default;
    };

    DECLARE_JSON_STRUCT(Segment) {
        NAMED_VALUE_DEFAULT(int, Threshold, 0, "threshold");
        NAMED_VALUE(std::string, Text, "text");

        Segment(int threshold, std::string text) : Threshold(threshold), Text(text) {}
        Segment() = default;
    };

    DECLARE_JSON_STRUCT(FloatSegment) {
        NAMED_VALUE_DEFAULT(float, Threshold, 0, "threshold");
        NAMED_VALUE(std::string, Text, "text");

        FloatSegment(int threshold, std::string text) : Threshold(threshold), Text(text) {}
        FloatSegment() = default;
    };

    DECLARE_JSON_STRUCT(Config) {
        NAMED_VECTOR(Judgement, Judgements, "judgments");
        DESERIALIZE_FUNCTION(NeedsJudgements) {
            if (Judgements.size() < 1)
                throw JSONException("no judgements found in config");
        };
        NAMED_VECTOR_DEFAULT(Judgement, ChainHeadJudgements, {}, "chainHeadJudgments");
        DESERIALIZE_FUNCTION(NeedsChainHeads) {
            HasChainHead = ChainHeadJudgements.size() > 0;
        };
        NAMED_VALUE_OPTIONAL(Judgement, ChainLinkDisplay, "chainLinkDisplay");
        DESERIALIZE_FUNCTION(NeedsChainLinks) {
            HasChainLink = ChainLinkDisplay.has_value();
        };
        NAMED_VECTOR_DEFAULT(Segment, BeforeCutAngleSegments, {}, "beforeCutAngleJudgments");
        NAMED_VECTOR_DEFAULT(Segment, AccuracySegments, {}, "accuracyJudgments");
        NAMED_VECTOR_DEFAULT(Segment, AfterCutAngleSegments, {}, "afterCutAngleJudgments");
        NAMED_VECTOR_DEFAULT(FloatSegment, TimeDependenceSegments, {}, "timeDependencyJudgments");
        NAMED_VALUE_OPTIONAL(float, FixedPosX, "fixedPosX");
        NAMED_VALUE_OPTIONAL(float, FixedPosY, "fixedPosY");
        NAMED_VALUE_OPTIONAL(float, FixedPosZ, "fixedPosZ");
        NAMED_VALUE_OPTIONAL(bool, UseFixedPos, "useFixedPos");
        NAMED_VALUE_OPTIONAL(EquatableVec3, UnprocessedFixedPos, "fixedPosition");
        NAMED_VALUE_OPTIONAL(EquatableVec3, UnprocessedPosOffset, "targetPositionOffset");
        DESERIALIZE_FUNCTION(ConvertPositions) {
            if (UseFixedPos.has_value() && UseFixedPos.value())
                FixedPos = UnityEngine::Vector3(FixedPosX.value_or(0), FixedPosY.value_or(0), FixedPosZ.value_or(0));
            else if (UnprocessedFixedPos.has_value())
                FixedPos = UnityEngine::Vector3(UnprocessedFixedPos->x, UnprocessedFixedPos->y, UnprocessedFixedPos->z);
            if (UnprocessedPosOffset)
                PosOffset = UnityEngine::Vector3(UnprocessedPosOffset->x, UnprocessedPosOffset->y, UnprocessedPosOffset->z);
        };
        NAMED_VALUE_DEFAULT(int, TimeDependenceDecimalPrecision, 1, "timeDependencyDecimalPrecision");
        NAMED_VALUE_DEFAULT(int, TimeDependenceDecimalOffset, 2, "timeDependencyDecimalOffset");

        std::optional<EquatableVec3> FixedPos;
        std::optional<EquatableVec3> PosOffset;
        bool HasChainHead;
        bool HasChainLink;
    };
}
