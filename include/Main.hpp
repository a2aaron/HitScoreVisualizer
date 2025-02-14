#pragma once

#include "GlobalNamespace/CutScoreBuffer.hpp"
#include "GlobalNamespace/FlyingScoreEffect.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"

constexpr auto logger = Paper::ConstLoggerContext(MOD_ID);

std::string ConfigsPath();

void LoadCurrentConfig();
void Judge(
    GlobalNamespace::CutScoreBuffer* cutScoreBuffer,
    GlobalNamespace::FlyingScoreEffect* flyingScoreEffect,
    GlobalNamespace::NoteCutInfo const& noteCutInfo
);
