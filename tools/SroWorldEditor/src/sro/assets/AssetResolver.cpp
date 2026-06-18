#include "assets/AssetResolver.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <filesystem>

namespace sro {

void AssetResolver::ResetForLoad(const std::wstring& clientPath) {
    m_clientPath = clientPath;
    m_pathResolver.SetClientPath(clientPath);
    m_phase = LoadPhase::ObjectIndex;
    m_completedSteps = 0;
    m_textData.BeginIncrementalLoad(clientPath);
    m_stringTable.BeginIncrementalLoad(clientPath);
    RecountTotalSteps();
}

void AssetResolver::RecountTotalSteps() {
    m_totalSteps = 4; // object, tile, string indices (at least 1 step), placements
    m_totalSteps += static_cast<int>(m_stringTable.TotalIndexCount());
    m_totalSteps += static_cast<int>(m_textData.TotalSplitCount());
    if (m_totalSteps < 1) m_totalSteps = 1;
}

void AssetResolver::Initialize(const std::wstring& clientPath) {
    ResetForLoad(clientPath);
    ClientLoadProgress progress;
    while (StepLoad(progress)) {}
}

bool AssetResolver::StepLoad(ClientLoadProgress& progress) {
    if (m_phase == LoadPhase::Done) {
        progress.done = true;
        progress.active = false;
        progress.fraction = 1.f;
        return false;
    }

    progress.active = true;
    progress.failed = false;
    progress.totalSteps = m_totalSteps;
    progress.step = m_completedSteps;

    switch (m_phase) {
    case LoadPhase::ObjectIndex:
        progress.stage = "Loading object.ifo...";
        m_objectIndex.Load(m_clientPath);
        m_phase = LoadPhase::TileIndex;
        break;
    case LoadPhase::TileIndex:
        progress.stage = "Loading tile index...";
        m_tileIndex.Load(m_clientPath);
        m_phase = LoadPhase::StringTable;
        break;
    case LoadPhase::StringTable: {
        std::wstring indexPath;
        if (m_stringTable.LoadNextIndexFile(&indexPath)) {
            progress.stage = "Loading strings: " + ToNarrow(std::filesystem::path(indexPath).filename().wstring());
        } else {
            m_phase = LoadPhase::TextData;
            return StepLoad(progress);
        }
        break;
    }
    case LoadPhase::TextData: {
        std::wstring splitName;
        if (m_textData.LoadNextSplit(&splitName)) {
            progress.stage = "Loading catalog: " + ToNarrow(splitName);
        } else {
            m_textData.FinalizeServiceIds();
            m_phase = LoadPhase::Placements;
            return StepLoad(progress);
        }
        break;
    }
    case LoadPhase::Placements:
        progress.stage = "Loading NPC and teleport data...";
        m_placements.Load(m_clientPath);
        m_phase = LoadPhase::Done;
        break;
    default:
        break;
    }

    ++m_completedSteps;
    progress.step = m_completedSteps;
    progress.fraction = static_cast<float>(m_completedSteps) / static_cast<float>(m_totalSteps);
    if (m_phase == LoadPhase::Done) {
        progress.stage = "Catalog load complete";
        progress.done = true;
        progress.active = false;
        progress.fraction = 1.f;
        Logger::Instance().Info("AssetResolver loaded " + std::to_string(m_textData.Count()) + " game objects");
        return false;
    }
    return true;
}

} // namespace sro
