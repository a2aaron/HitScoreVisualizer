#include <random>

#include "Config.hpp"
#include "GlobalNamespace/CutScoreBuffer.hpp"
#include "GlobalNamespace/IReadonlyCutScoreBuffer.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "Main.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "TMPro/TextMeshPro.hpp"
#include "UnityEngine/Mathf.hpp"
#include "metacore/shared/operators.hpp"

using namespace HSV;
using ScoringType = GlobalNamespace::NoteData::ScoringType;

enum class Direction { Up, UpRight, Right, DownRight, Down, DownLeft, Left, UpLeft, None };

static float const angle = sqrt(2) / 2;

static std::map<Direction, UnityEngine::Vector3> const normalsMap = {
    {Direction::DownRight, {angle, -angle, 0}},
    {Direction::Down, {0, -1, 0}},
    {Direction::DownLeft, {-angle, -angle, 0}},
    {Direction::Left, {-1, 0, 0}},
};

static Direction GetWrongDirection(GlobalNamespace::NoteCutInfo const& cutInfo) {
    float best = std::numeric_limits<float>::min();
    Direction ret = Direction::None;
    for (auto& [direction, normal] : normalsMap) {
        float compare = abs(UnityEngine::Vector3::Dot(cutInfo.cutNormal, normal));
        if (compare > best) {
            best = compare;
            ret = direction;
        }
    }
    if (ret == Direction::None)
        return ret;
    if (UnityEngine::Vector3::Dot(normalsMap.at(ret), UnityEngine::Vector3::op_Subtraction(cutInfo.notePosition, cutInfo.cutPoint)) > 0)
        return ret;
    int asInt = (int) ret;
    if (asInt < 4)
        return (Direction) (asInt + 4);
    return (Direction) (asInt - 4);
}

static std::string_view GetDirectionText(Direction wrongDirection) {
    switch (wrongDirection) {
        case Direction::Up:
            return "↑";
        case Direction::UpRight:
            return "↗";
        case Direction::Right:
            return "→";
        case Direction::DownRight:
            return "↘";
        case Direction::Down:
            return "↓";
        case Direction::DownLeft:
            return "↙";
        case Direction::Left:
            return "←";
        case Direction::UpLeft:
            return "↖";
        default:
            return "";
    }
}

static Judgement& GetBestJudgement(std::vector<Judgement>& judgements, int comparison) {
    Judgement* best = nullptr;
    for (auto& judgement : judgements) {
        if (comparison >= judgement.Threshold && (!best || judgement.Threshold > best->Threshold))
            best = &judgement;
    }
    return best ? *best : judgements.back();
}

static std::string GetBestSegmentText(std::vector<Segment>& segments, int comparison) {
    Segment* best = nullptr;
    for (auto& segment : segments) {
        if (comparison >= segment.Threshold && (!best || segment.Threshold > best->Threshold))
            best = &segment;
    }
    return best ? best->Text : "";
}

static std::string GetBestFloatSegmentText(std::vector<FloatSegment>& segments, float comparison) {
    FloatSegment* best = nullptr;
    for (auto& segment : segments) {
        if (comparison >= segment.Threshold && (!best || segment.Threshold > best->Threshold))
            best = &segment;
    }
    return best ? best->Text : "";
}

static std::string TimeDependenceString(float timeDependence) {
    int multiplier = std::pow(10, getGlobalConfig().CurrentConfig.TimeDependenceDecimalOffset);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(getGlobalConfig().CurrentConfig.TimeDependenceDecimalPrecision) << (timeDependence * multiplier);
    return ss.str();
}

static std::string
GetJudgementText(Judgement& judgement, int score, int before, int after, int accuracy, float timeDependence, int maxScore, Direction wrongDirection) {
    auto& text = judgement.Text;

    text.set_beforeCut(std::to_string(before));
    text.set_accuracy(std::to_string(accuracy));
    text.set_afterCut(std::to_string(after));
    text.set_score(std::to_string(score));
    text.set_percent(std::to_string(round(100 * (float) score / maxScore)));
    text.set_timeDependency(TimeDependenceString(timeDependence));
    text.set_beforeCutSegment(GetBestSegmentText(getGlobalConfig().CurrentConfig.BeforeCutAngleSegments, before));
    text.set_accuracySegment(GetBestSegmentText(getGlobalConfig().CurrentConfig.AccuracySegments, accuracy));
    text.set_afterCutSegment(GetBestSegmentText(getGlobalConfig().CurrentConfig.AfterCutAngleSegments, after));
    text.set_timeDependencySegment(GetBestFloatSegmentText(getGlobalConfig().CurrentConfig.TimeDependenceSegments, timeDependence));
    text.set_direction(GetDirectionText(wrongDirection));

    return text.Join();
}

static UnityEngine::Color GetJudgementColor(Judgement& judgement, std::vector<Judgement>& judgements, int score) {
    if (!judgement.Fade || !judgement.Fade.value())
        return judgement.Color.Color;
    // get the lowest judgement with a higher threshold
    Judgement* best = nullptr;
    for (auto& judgement : judgements) {
        if (score < judgement.Threshold && (!best || judgement.Threshold < best->Threshold))
            best = &judgement;
    }
    if (!best)
        return judgement.Color.Color;
    int lowerThreshold = judgement.Threshold;
    int higherThreshold = best->Threshold;
    float lerpDistance = ((float) score - lowerThreshold) / (higherThreshold - lowerThreshold);
    auto lowerColor = judgement.Color.Color;
    auto higherColor = best->Color.Color;
    return UnityEngine::Color(
        lowerColor.r + (higherColor.r - lowerColor.r) * lerpDistance,
        lowerColor.g + (higherColor.g - lowerColor.g) * lerpDistance,
        lowerColor.b + (higherColor.b - lowerColor.b) * lerpDistance,
        lowerColor.a + (higherColor.a - lowerColor.a) * lerpDistance
    );
}

static void UpdateScoreEffect(
    GlobalNamespace::FlyingScoreEffect* flyingScoreEffect,
    int total,
    int before,
    int after,
    int accuracy,
    float timeDependence,
    ScoringType scoringType,
    Direction wrongDirection
) {
    std::string text;
    UnityEngine::Color color;

    int maxScore = GlobalNamespace::ScoreModel::GetNoteScoreDefinition(scoringType)->maxCutScore;

    if (scoringType == ScoringType::ChainLink || scoringType == ScoringType::ChainLinkArcHead) {
        auto&& judgement =
            getGlobalConfig().CurrentConfig.ChainLinkDisplay.value_or(GetBestJudgement(getGlobalConfig().CurrentConfig.Judgements, total));

        text = GetJudgementText(judgement, total, before, after, accuracy, timeDependence, maxScore, wrongDirection);
        color = judgement.Color.Color;
    } else if (scoringType == ScoringType::ChainHead || scoringType == ScoringType::ChainHeadArcTail) {
        auto& judgementVector = getGlobalConfig().CurrentConfig.ChainHeadJudgements;
        auto& judgement = GetBestJudgement(judgementVector, total);

        text = GetJudgementText(judgement, total, before, after, accuracy, timeDependence, maxScore, wrongDirection);
        color = GetJudgementColor(judgement, judgementVector, total);
    } else {
        auto& judgementVector = getGlobalConfig().CurrentConfig.Judgements;
        auto& judgement = GetBestJudgement(judgementVector, total);

        text = GetJudgementText(judgement, total, before, after, accuracy, timeDependence, maxScore, wrongDirection);
        color = GetJudgementColor(judgement, judgementVector, total);
    }

    flyingScoreEffect->_text->text = text;
    flyingScoreEffect->_text->color = color;
    flyingScoreEffect->_color = color;
}

void Judge(
    GlobalNamespace::CutScoreBuffer* cutScoreBuffer,
    GlobalNamespace::FlyingScoreEffect* flyingScoreEffect,
    GlobalNamespace::NoteCutInfo const& noteCutInfo
) {
    if (!cutScoreBuffer) {
        logger.info("CutScoreBuffer is null");
        return;
    }
    if (!flyingScoreEffect || !flyingScoreEffect->_text) {
        logger.info("FlyingScoreEffect is null");
        return;
    }

    if (!cutScoreBuffer->isFinished && getGlobalConfig().HideUntilDone.GetValue()) {
        flyingScoreEffect->_text->text = "";
        return;
    }

    // get scores for each part of the cut
    int before = cutScoreBuffer->beforeCutScore;
    int after = cutScoreBuffer->afterCutScore;
    int accuracy = cutScoreBuffer->centerDistanceCutScore;
    int total = cutScoreBuffer->cutScore;
    float timeDependence = std::abs(noteCutInfo.cutNormal.z);
    Direction wrongDirection = GetWrongDirection(noteCutInfo);

    ScoringType scoringType = noteCutInfo.noteData->scoringType;

    UpdateScoreEffect(flyingScoreEffect, total, before, after, accuracy, timeDependence, scoringType, wrongDirection);
}

static int wrongDirectionsCounter = 0;
static int wrongColorsCounter = 0;
static int bombsCounter = 0;
static int missesCounter = 0;

static std::random_device device;
static std::default_random_engine rng(device());

static int Random(int min, int max) {
    return std::uniform_int_distribution<int>(min, max)(rng);
}

template <class T>
static T const& GetDisplay(std::vector<T> const& displays, int& counter, bool randomize) {
    int idx = randomize ? std::uniform_int_distribution<int>(0, displays.size())(rng) : (counter++ % displays.size());
    return displays[idx];
}

static std::pair<int&, std::vector<BadCutDisplay>&> GetBadCutList(GlobalNamespace::NoteCutInfo const& noteCutInfo) {
    if (noteCutInfo.noteData->colorType == GlobalNamespace::ColorType::None)
        return {bombsCounter, getGlobalConfig().CurrentConfig.Bombs};
    if (!noteCutInfo.saberTypeOK)
        return {wrongColorsCounter, getGlobalConfig().CurrentConfig.WrongColors};
    return {wrongDirectionsCounter, getGlobalConfig().CurrentConfig.WrongDirections};
}

bool SpawnBadCut(GlobalNamespace::FlyingTextSpawner* spawner, GlobalNamespace::NoteCutInfo const& noteCutInfo) {
    auto [counter, vector] = GetBadCutList(noteCutInfo);
    if (vector.empty() || !spawner)
        return false;
    bool randomize = getGlobalConfig().CurrentConfig.RandomizeBadCutDisplays;
    auto const& display = GetDisplay(vector, counter, randomize);
    spawner->_color = display.Color.Color;
    spawner->SpawnText(noteCutInfo.cutPoint, noteCutInfo.worldRotation, noteCutInfo.inverseWorldRotation, display.Text);
    return true;
}

bool SpawnMiss(GlobalNamespace::FlyingTextSpawner* spawner, GlobalNamespace::NoteController* note, float z) {
    auto& config = getGlobalConfig().CurrentConfig;
    if (config.MissDisplays.empty() || !spawner)
        return false;
    auto const& display = GetDisplay(config.MissDisplays, missesCounter, config.RandomizeMissDisplays);
    spawner->_color = display.Color.Color;
    auto position = note->inverseWorldRotation * note->_noteTransform->position;
    position.z = z;
    spawner->SpawnText(position, note->worldRotation, note->inverseWorldRotation, display.Text);
    return true;
}
