#include "sro/nav/NavMeshContext.h"

namespace sro::nav {

const NavObjectBinding* NavMeshContext::FindByLocalUid(int16_t localUid) const {
    for (const auto& b : bindings) {
        if (b.nvmObjectIndex >= 0 && nvm &&
            nvm->Objects[static_cast<size_t>(b.nvmObjectIndex)].LocalUID == localUid)
            return &b;
    }
    for (const auto& b : bindings) {
        if (b.placement && b.placement->Object.UID == localUid)
            return &b;
    }
    return nullptr;
}

const NavObjectBinding* NavMeshContext::FindByNvmIndex(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(bindings.size()))
        return nullptr;
    return &bindings[static_cast<size_t>(idx)];
}

const NavObjectBinding* NavMeshContext::FindByPlacementUid(int16_t uid) const {
    for (const auto& b : bindings) {
        if (b.placement && b.placement->Object.UID == uid)
            return &b;
    }
    return nullptr;
}

NavMeshContext BuildNavMeshContext(const sro::formats::NavMesh& nvm,
                                   const std::vector<PlacementVM>& placements,
                                   int centerRx, int centerRy) {
    NavMeshContext ctx;
    ctx.nvm = &nvm;
    ctx.bindings.resize(nvm.Objects.size());

    for (size_t i = 0; i < nvm.Objects.size(); ++i) {
        auto& b = ctx.bindings[i];
        b.nvmObjectIndex = static_cast<int>(i);
        const auto& obj = nvm.Objects[i];
        const int objRx = (obj.RegionID >> 8) & 0xFF;
        const int objRy = obj.RegionID & 0xFF;
        b.foreignRegion = (objRx != centerRx || objRy != centerRy);

        for (const auto& p : placements) {
            if (p.LoadedRx != centerRx || p.LoadedRy != centerRy)
                continue;
            if (p.Object.UID == obj.LocalUID) {
                b.placement = &p;
                b.bms = p.Collision.NavMesh;
                break;
            }
        }
        if (!b.placement)
            b.inNvmOnly = true;
    }

    for (size_t ci = 0; ci < nvm.Cells.size(); ++ci) {
        for (uint16_t oi : nvm.Cells[ci].ObjIndices) {
            if (oi < ctx.bindings.size())
                ctx.bindings[oi].cellQuadIndices.push_back(static_cast<int>(ci));
        }
    }

    for (const auto& p : placements) {
        if (p.LoadedRx != centerRx || p.LoadedRy != centerRy)
            continue;
        bool found = false;
        for (const auto& b : ctx.bindings) {
            if (b.placement == &p) { found = true; break; }
        }
        if (!found) {
            NavObjectBinding b;
            b.nvmObjectIndex = -1;
            b.placement = &p;
            b.bms = p.Collision.NavMesh;
            b.placementOnly = true;
            ctx.bindings.push_back(b);
        }
    }

    return ctx;
}

} // namespace sro::nav
