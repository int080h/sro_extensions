#pragma once
#include "nav/NavMeshSession.h"
#include "formats/NavMeshFormat.h"
#include <string>
#include <vector>

namespace sro::nav {

class NavMeshEditorService {
public:
    struct ValidationIssue {
        std::string message;
        bool isError = true;
    };

    struct ValidationResult {
        bool ok = true;
        std::vector<ValidationIssue> issues;
    };

    explicit NavMeshEditorService(NavMeshSession& session) : m_session(session) {}

    NavMeshSession& Session() { return m_session; }
    const NavMeshSession& Session() const { return m_session; }

    ValidationResult ValidateNavMesh(const formats::NavMesh& nav) const;
    static std::vector<int> FindPathBFS(const formats::NavMesh& nav, int startCell, int endCell);

    bool SaveAllDirty() { return m_session.SaveAllDirty(); }
    bool HasDirtyFiles() const { return m_session.HasDirtyFiles(); }

    formats::NavMesh* GetTerrainNavMesh(int rx, int ry) { return m_session.GetTerrainNavMesh(rx, ry); }
    formats::AiNavData* GetAiNavData(uint16_t regionId) { return m_session.GetAiNavData(regionId); }

    bool LoadTerrainNavMesh(int rx, int ry) { return m_session.LoadTerrainNavMesh(rx, ry); }
    bool LoadAiNavData(uint16_t regionId) { return m_session.LoadAiNavData(regionId); }
    bool SaveTerrainNavMesh(int rx, int ry) { return m_session.SaveTerrainNavMesh(rx, ry); }
    bool SaveAiNavData(uint16_t regionId) { return m_session.SaveAiNavData(regionId); }

    void MarkTerrainDirty(int rx, int ry) { m_session.MarkTerrainDirty(rx, ry); }
    void MarkAiNavDirty(uint16_t regionId) { m_session.MarkAiNavDirty(regionId); }

private:
    NavMeshSession& m_session;
};

} // namespace sro::nav
