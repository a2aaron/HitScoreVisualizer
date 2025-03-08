#include "Settings.hpp"

#include "Config.hpp"
#include "HMUI/Touchable.hpp"
#include "Main.hpp"
#include "UnityEngine/Resources.hpp"
#include "bsml/shared/BSML-Lite.hpp"

DEFINE_TYPE(HSV, CustomList);
DEFINE_TYPE(HSV, SettingsViewController);

using namespace HSV;

void CustomList::ctor() {
    INVOKE_CTOR();
    expandCell = false;
    reuseIdentifier = "HSVConfigListTableCell";
    cellSize = 8;
    tableView = nullptr;
}

HMUI::TableCell* CustomList::CellForIdx(HMUI::TableView* tableView, int idx) {
    auto tableCell = (GlobalNamespace::SimpleTextTableCell*) tableView->DequeueReusableCellForIdentifier(reuseIdentifier).unsafePtr();
    if (!tableCell) {
        auto simpleTextTableCellInstance = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::SimpleTextTableCell*>()->First([](auto x) {
            return x->name == std::string("SimpleTextTableCell");
        });
        tableCell = Instantiate(simpleTextTableCellInstance);
        tableCell->reuseIdentifier = reuseIdentifier;

        tableCell->_text->richText = true;
        tableCell->_text->enableWordWrapping = false;
        BSML::Lite::AddHoverHint(tableCell, "");
    }

    tableCell->text = data[idx];
    if (failures.contains(idx)) {
        tableCell->GetComponent<HMUI::HoverHint*>()->text = failures[idx];
        tableCell->interactable = false;
    } else {
        tableCell->GetComponent<HMUI::HoverHint*>()->text = "";
        tableCell->interactable = true;
    }
    tableCell->gameObject->active = true;
    return tableCell;
}

float CustomList::CellSize() {
    return cellSize;
}

int CustomList::NumberOfCells() {
    return data.size();
}

int SettingsViewController::selectedIdx = -1;
std::vector<std::string> SettingsViewController::fullConfigPaths = {};

void SettingsViewController::ConfigSelected(int idx) {
    selectedIdx = idx;
    getGlobalConfig().SelectedConfig.SetValue(fullConfigPaths[idx]);
    LoadCurrentConfig();
    selectedConfig->text = "Current Config: " + configList->data[idx];
}

void SettingsViewController::RefreshConfigList() {
    auto& failureMap = configList->failures;
    auto& data = configList->data;
    failureMap.clear();
    data = {"Default"};
    fullConfigPaths = {""};
    if (getGlobalConfig().SelectedConfig.GetValue() == "")
        selectedIdx = 0;

    Config config;
    for (auto& entry : std::filesystem::recursive_directory_iterator(ConfigsPath())) {
        std::string displayPath = entry.path().stem().string();
        std::string fullPath = entry.path().string();
        // test loading the config
        try {
            ReadFromFile(fullPath, config);
        } catch (std::exception const& err) {
            logger.error("Could not load config file {}: {}", fullPath, err.what());
            data.emplace_back(fmt::format("<color=red>{}", displayPath));
            fullConfigPaths.emplace_back(fullPath);
            failureMap.insert({data.size() - 1, fmt::format("Error loading config: {}", err.what())});
            continue;
        }
        data.emplace_back(displayPath);
        fullConfigPaths.emplace_back(fullPath);
        if (getGlobalConfig().SelectedConfig.GetValue() == fullPath)
            selectedIdx = data.size() - 1;
    }
    configList->tableView->ReloadData();
    if (selectedIdx >= 0) {
        configList->tableView->SelectCellWithIdx(selectedIdx, false);
        configList->tableView->ScrollToCellWithIdx(selectedIdx, HMUI::TableView::ScrollPositionType::Beginning, false);
    }
}

void SettingsViewController::RefreshUI() {
    RefreshConfigList();
    selectedConfig->text = "Current Config: " + configList->data[selectedIdx];
    enabledToggle->toggle->isOn = getGlobalConfig().ModEnabled.GetValue();
    hideToggle->toggle->isOn = getGlobalConfig().HideUntilDone.GetValue();
}

void SettingsViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if (firstActivation) {
        gameObject->AddComponent<HMUI::Touchable*>();
        auto container = BSML::Lite::CreateVerticalLayoutGroup(transform);

        auto textLayout = BSML::Lite::CreateVerticalLayoutGroup(container);
        textLayout->childForceExpandHeight = false;
        textLayout->childForceExpandWidth = false;
        textLayout->childControlHeight = true;
        textLayout->spacing = 1.5;

        enabledToggle = BSML::Lite::CreateToggle(textLayout, "Mod Enabled", getGlobalConfig().ModEnabled.GetValue(), [](bool enabled) {
            getGlobalConfig().ModEnabled.SetValue(enabled);
        });
        BSML::Lite::AddHoverHint(enabledToggle, "Toggles whether the mod is active or not");

        hideToggle =
            BSML::Lite::CreateToggle(textLayout, "Hide Until Calculation Finishes", getGlobalConfig().HideUntilDone.GetValue(), [](bool enabled) {
                getGlobalConfig().HideUntilDone.SetValue(enabled);
            });
        BSML::Lite::AddHoverHint(enabledToggle, "With this enabled, the hit scores will not be displayed until the score has been finalized");

        selectedConfig = BSML::Lite::CreateText(textLayout, "");

        configList = BSML::Lite::CreateScrollableCustomSourceList<CustomList*>(container, {50, 50}, [this](int idx) { ConfigSelected(idx); });
    }
    RefreshUI();
}
