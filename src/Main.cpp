#include "Main.hpp"

#include "Config.hpp"
#include "GlobalNamespace/BeatmapObjectExecutionRating.hpp"
#include "GlobalNamespace/FlyingScoreEffect.hpp"
#include "GlobalNamespace/FlyingScoreSpawner.hpp"
#include "GlobalNamespace/IReadonlyCutScoreBuffer.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "Settings.hpp"
#include "TMPro/TextMeshPro.hpp"
#include "UnityEngine/AnimationCurve.hpp"
#include "UnityEngine/SpriteRenderer.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "bsml/shared/BSML.hpp"
#include "custom-types/shared/register.hpp"
#include "json/DefaultConfig.hpp"

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0};

std::string ConfigsPath() {
    static std::string path = getDataDir(modInfo);
    return path;
}

static void SetDefaultConfig() {
    getGlobalConfig().SelectedConfig.SetValue("");
    getGlobalConfig().CurrentConfig = defaultConfig;
}

void LoadCurrentConfig() {
    std::string selected = getGlobalConfig().SelectedConfig.GetValue();
    if (!selected.empty() && !fileexists(selected)) {
        logger.warn("Could not find selected config! Using the default");
        SetDefaultConfig();
        return;
    }
    try {
        ReadFromFile(selected, getGlobalConfig().CurrentConfig);
    } catch (std::exception const& err) {
        logger.error("Could not load config file {}: {}", selected, err.what());
        SetDefaultConfig();
    }
}

// used for fixed position
GlobalNamespace::FlyingScoreEffect* currentEffect = nullptr;
// used for updating ratings
std::unordered_map<GlobalNamespace::CutScoreBuffer*, GlobalNamespace::FlyingScoreEffect*> swingRatingMap = {};

static bool SkipJudge(GlobalNamespace::NoteCutInfo const& cutInfo) {
    using ScoringType = GlobalNamespace::NoteData::ScoringType;

    auto cutType = cutInfo.noteData->scoringType;
    if (cutType == ScoringType::ChainHead || cutType == ScoringType::ChainHeadArcTail)
        return !getGlobalConfig().CurrentConfig.HasChainHead;
    if (cutType == ScoringType::ChainLink || cutType == ScoringType::ChainLinkArcHead)
        return !getGlobalConfig().CurrentConfig.HasChainLink;
    return false;
}

MAKE_HOOK_MATCH(
    FlyingScoreEffect_InitAndPresent,
    &GlobalNamespace::FlyingScoreEffect::InitAndPresent,
    void,
    GlobalNamespace::FlyingScoreEffect* self,
    GlobalNamespace::IReadonlyCutScoreBuffer* cutScoreBuffer,
    float duration,
    UnityEngine::Vector3 targetPos,
    UnityEngine::Color color
) {
    bool enabled = getGlobalConfig().ModEnabled.GetValue();

    if (enabled) {
        auto& config = getGlobalConfig().CurrentConfig;
        if (config.FixedPos) {
            targetPos = config.FixedPos.value();
            self->transform->position = targetPos;
            if (!getGlobalConfig().HideUntilDone.GetValue()) {
                if (currentEffect)
                    currentEffect->gameObject->active = false;
                currentEffect = self;
            }
        } else if (config.PosOffset)
            targetPos = UnityEngine::Vector3::op_Addition(targetPos, *config.PosOffset);
    }
    FlyingScoreEffect_InitAndPresent(self, cutScoreBuffer, duration, targetPos, color);

    if (enabled) {
        if (cutScoreBuffer == nullptr) {
            logger.error("CutScoreBuffer is null!");
            return;
        }
        auto cast = il2cpp_utils::try_cast<GlobalNamespace::CutScoreBuffer>(cutScoreBuffer).value_or(nullptr);
        if (cast == nullptr) {
            logger.error("CutScoreBuffer is not GlobalNamespace::CutScoreBuffer!");
            return;
        }
        if (SkipJudge(cast->noteCutInfo))
            return;

        if (!cast->isFinished)
            swingRatingMap.insert({cast, self});

        self->_maxCutDistanceScoreIndicator->enabled = false;
        self->_text->richText = true;
        self->_text->enableWordWrapping = false;
        self->_text->overflowMode = TMPro::TextOverflowModes::Overflow;

        Judge(cast, self, cast->noteCutInfo);
    }
}

MAKE_HOOK_MATCH(
    CutScoreBuffer_HandleSaberSwingRatingCounterDidChange,
    &GlobalNamespace::CutScoreBuffer::HandleSaberSwingRatingCounterDidChange,
    void,
    GlobalNamespace::CutScoreBuffer* self,
    GlobalNamespace::ISaberSwingRatingCounter* swingRatingCounter,
    float rating
) {
    CutScoreBuffer_HandleSaberSwingRatingCounterDidChange(self, swingRatingCounter, rating);

    if (getGlobalConfig().ModEnabled.GetValue()) {
        if (SkipJudge(self->noteCutInfo))
            return;

        auto itr = swingRatingMap.find(self);
        if (itr == swingRatingMap.end())
            return;
        auto flyingScoreEffect = itr->second;

        Judge(self, flyingScoreEffect, self->noteCutInfo);
    }
}

MAKE_HOOK_MATCH(
    CutScoreBuffer_HandleSaberSwingRatingCounterDidFinish,
    &GlobalNamespace::CutScoreBuffer::HandleSaberSwingRatingCounterDidFinish,
    void,
    GlobalNamespace::CutScoreBuffer* self,
    GlobalNamespace::ISaberSwingRatingCounter* swingRatingCounter
) {
    CutScoreBuffer_HandleSaberSwingRatingCounterDidFinish(self, swingRatingCounter);

    if (getGlobalConfig().ModEnabled.GetValue()) {
        if (SkipJudge(self->noteCutInfo))
            return;

        auto itr = swingRatingMap.find(self);
        if (itr == swingRatingMap.end())
            return;
        auto flyingScoreEffect = itr->second;
        swingRatingMap.erase(itr);

        Judge(self, flyingScoreEffect, self->noteCutInfo);

        if (getGlobalConfig().CurrentConfig.FixedPos && getGlobalConfig().HideUntilDone.GetValue()) {
            if (currentEffect)
                currentEffect->gameObject->active = false;
            currentEffect = flyingScoreEffect;
        }
    }
}

MAKE_HOOK_MATCH(
    FlyingScoreSpawner_HandleFlyingObjectEffectDidFinish,
    &GlobalNamespace::FlyingScoreSpawner::HandleFlyingObjectEffectDidFinish,
    void,
    GlobalNamespace::FlyingScoreSpawner* self,
    GlobalNamespace::FlyingObjectEffect* effect
) {
    if (currentEffect == (GlobalNamespace::FlyingScoreEffect*) effect) {
        currentEffect->gameObject->active = false;
        currentEffect = nullptr;
    }
    FlyingScoreSpawner_HandleFlyingObjectEffectDidFinish(self, effect);
}

MAKE_HOOK_MATCH(
    FlyingScoreEffect_ManualUpdate, &GlobalNamespace::FlyingScoreEffect::ManualUpdate, void, GlobalNamespace::FlyingScoreEffect* self, float t
) {
    FlyingScoreEffect_ManualUpdate(self, t);

    if (getGlobalConfig().ModEnabled.GetValue()) {
        self->_color.a = self->_fadeAnimationCurve->Evaluate(t);
        self->_text->color = self->_color;
    }
}

extern "C" void setup(CModInfo* info) {
    *info = modInfo.to_c();

    Paper::Logger::RegisterFileContextId(MOD_ID);

    getGlobalConfig().Init(modInfo);

    if (!direxists(ConfigsPath()))
        mkpath(ConfigsPath());

    LoadCurrentConfig();

    logger.info("Completed setup!");
}

extern "C" void late_load() {
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    BSML::Init();

    BSML::Register::RegisterSettingsMenu<HSV::SettingsViewController*>("Hit Score Visualizer");
    BSML::Register::RegisterMainMenu<HSV::SettingsViewController*>("Hit Score Visualizer", "Hit Score Visualizer", "");

    logger.info("Installing hooks...");
    INSTALL_HOOK(logger, FlyingScoreEffect_InitAndPresent);
    INSTALL_HOOK(logger, CutScoreBuffer_HandleSaberSwingRatingCounterDidChange);
    INSTALL_HOOK(logger, CutScoreBuffer_HandleSaberSwingRatingCounterDidFinish);
    INSTALL_HOOK(logger, FlyingScoreSpawner_HandleFlyingObjectEffectDidFinish);
    INSTALL_HOOK(logger, FlyingScoreEffect_ManualUpdate);
    logger.info("Installed all hooks!");
}
