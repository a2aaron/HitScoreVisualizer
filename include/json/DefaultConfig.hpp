#pragma once

#include "Config.hpp"

inline HSV::Config const defaultConfig = {
    .Judgements =
        {
            {115, "<size=150%><u>%s</u></size>", {1, 1, 1, 1}},
            {110, "%B<size=120%>%C%s</u></size>%A", {0, 0.5, 1, 1}},
            {105, "%B%C%s</u>%A", {0, 1, 0, 1}},
            {100, "%B%C%s</u>%A", {1, 1, 0, 1}},
            {50, "%B<size=80%>%s</size>%A", {1, 0, 0, 1}, true},
            {0, "%B<size=80%>%s</size>%A", {1, 0, 0, 1}},
        },
    .ChainHeadJudgements =
        {
            {85, "<size=150%><u>%s</u></size>", {1, 1, 1, 1}},
            {80, "%B<size=120%>%C%s</u></size>%A", {0, 0.5, 1, 1}},
            {75, "%B%C%s</u>%A", {0, 1, 0, 1}},
            {70, "%B%C%s</u>%A", {1, 1, 0, 1}},
            {35, "%B<size=80%>%s</size>%A", {1, 0, 0, 1}, true},
            {0, "%B<size=80%>%s</size>%A", {1, 0, 0, 1}},
        },
    .ChainLinkDisplay = {{0, "<alpha=#80><size=80%>%s", {1, 1, 1, 1}}},
    .BeforeCutAngleSegments =
        {
            {70, " + "},
            {0, "<color=#ff4f4f> - </color>"},
        },
    .AccuracySegments =
        {
            {15, "<u>"},
            {0, ""},
        },
    .AfterCutAngleSegments =
        {
            {30, " + "},
            {0, "<color=#ff4f4f> - </color>"},
        },
};
