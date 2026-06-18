#include "EfpParser.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <set>
#include <unordered_set>

namespace {

bool IsPrintablePathChar(unsigned char c) {
    return c >= 32 && c < 127;
}

std::string Lower(std::string s) {
    std::replace(s.begin(), s.end(), '\\', '/');
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool HasSuffix(const std::string& s, const char* suffix) {
    const size_t len = std::strlen(suffix);
    return s.size() >= len && s.compare(s.size() - len, len, suffix) == 0;
}

bool LooksLikeAssetPath(const std::string& s, const char* suffix) {
    if (!HasSuffix(Lower(s), suffix)) return false;
    if (s.size() < 5 || s.size() > 260) return false;
    for (unsigned char c : s) {
        if (!IsPrintablePathChar(c)) return false;
    }
    return true;
}

void PushUnique(std::vector<std::string>& out, std::unordered_set<std::string>& seen, const std::string& path) {
    const std::string key = Lower(path);
    if (seen.insert(key).second) {
        out.push_back(path);
    }
}

std::vector<std::string> ExtractAsciiStrings(const std::vector<unsigned char>& data) {
    std::vector<std::string> strings;
    std::string cur;
    for (unsigned char c : data) {
        if (IsPrintablePathChar(c)) {
            cur.push_back(static_cast<char>(c));
        } else {
            if (cur.size() >= 4) strings.push_back(cur);
            cur.clear();
        }
    }
    if (cur.size() >= 4) strings.push_back(cur);
    return strings;
}

std::vector<std::string> ExtractLengthPrefixedStrings(const std::vector<unsigned char>& data) {
    std::vector<std::string> strings;
    for (size_t i = 0; i + sizeof(uint32_t) <= data.size(); ++i) {
        uint32_t len = 0;
        std::memcpy(&len, data.data() + i, sizeof(len));
        if (len < 3 || len > 260 || i + sizeof(uint32_t) + len > data.size()) continue;

        const unsigned char* start = data.data() + i + sizeof(uint32_t);
        bool printable = true;
        for (uint32_t j = 0; j < len; ++j) {
            if (!IsPrintablePathChar(start[j])) {
                printable = false;
                break;
            }
        }
        if (!printable) continue;

        strings.emplace_back(reinterpret_cast<const char*>(start), len);
    }
    return strings;
}

void MergeUniqueStrings(std::vector<std::string>& out, const std::vector<std::string>& extra) {
    std::unordered_set<std::string> seen;
    for (const std::string& s : out) {
        seen.insert(Lower(s));
    }
    for (const std::string& s : extra) {
        const std::string key = Lower(s);
        if (seen.insert(key).second) {
            out.push_back(s);
        }
    }
}

bool IsTexturePath(const std::string& s) {
    return LooksLikeAssetPath(s, ".ddj") || LooksLikeAssetPath(s, ".dds");
}

bool IsMeshPath(const std::string& s) {
    return LooksLikeAssetPath(s, ".bms");
}

bool IsRenderPlate(const std::string& s) {
    return Lower(s) == "renderplate";
}

bool IsRenderMesh(const std::string& s) {
    return Lower(s) == "rendermesh";
}

bool IsViewModeToken(const std::string& s) {
    const std::string lower = Lower(s);
    return lower.find("viewbillboard") != std::string::npos ||
           lower.find("viewybillboard") != std::string::npos ||
           lower.find("viewvbillboard") != std::string::npos ||
           lower == "viewnone";
}

std::string NormalizeViewMode(const std::string& s) {
    const std::string lower = Lower(s);
    if (lower.find("viewybillboard") != std::string::npos) return "ViewYBillboard";
    if (lower.find("viewvbillboard") != std::string::npos) return "ViewVBillboard";
    if (lower.find("viewbillboard") != std::string::npos) return "ViewBillboard";
    if (lower == "viewnone") return "ViewNone";
    return {};
}

std::string FindNearbyViewMode(const std::vector<std::string>& strings, size_t index, size_t radius) {
    const size_t begin = (index > radius) ? index - radius : 0;
    const size_t end = (std::min)(strings.size(), index + radius + 1);
    for (size_t j = begin; j < end; ++j) {
        if (IsViewModeToken(strings[j])) {
            return NormalizeViewMode(strings[j]);
        }
    }
    return {};
}

std::string FindNearbyShape(const std::vector<std::string>& strings, size_t index, size_t radius) {
    const size_t begin = (index > radius) ? index - radius : 0;
    const size_t end = (std::min)(strings.size(), index + radius + 1);
    for (size_t j = begin; j < end; ++j) {
        if (IsRenderMesh(strings[j])) return "RenderMesh";
        if (IsRenderPlate(strings[j])) return "RenderPlate";
    }
    return {};
}

void PushRenderItem(std::vector<EfpRenderItem>& out, std::set<std::string>& seen, EfpRenderItem item) {
    if (item.Shape.empty() || item.TexturePaths.empty()) return;
    std::string key = Lower(item.Shape) + "|" + Lower(item.MeshPath) + "|" + Lower(item.ViewMode);
    for (const std::string& tex : item.TexturePaths) {
        key += "|" + Lower(tex);
    }
    if (seen.insert(key).second) {
        out.push_back(std::move(item));
    }
}

std::string TextureFamilyKey(const std::string& texPath) {
    std::string family = Lower(texPath);
    const size_t slash = family.find_last_of('/');
    if (slash != std::string::npos) {
        family = family.substr(slash + 1);
    }
    while (!family.empty() && std::isdigit(static_cast<unsigned char>(family.back()))) {
        family.pop_back();
    }
    while (!family.empty() && (family.back() == '_' || family.back() == '-')) {
        family.pop_back();
    }
    return family;
}

void CollapseDuplicatePlateItems(std::vector<EfpRenderItem>& items) {
    std::vector<EfpRenderItem> collapsed;
    collapsed.reserve(items.size());
    std::set<std::string> seenFamilies;
    for (EfpRenderItem& item : items) {
        if (item.TexturePaths.empty()) continue;
        const std::string family = TextureFamilyKey(item.TexturePaths.front());
        if (!seenFamilies.insert(family).second) continue;
        collapsed.push_back(std::move(item));
    }
    items = std::move(collapsed);
}

struct ParsedSlideBlock {
    size_t Offset = 0;
    FrameTextureSlideData Slide;
};

struct ParsedEmitBlock {
    size_t Offset = 0;
    EfpEmitterDef Emitter;
};

bool ReadLengthPrefixedString(const std::vector<unsigned char>& data, size_t offset, std::string& out) {
    if (offset + 4 > data.size()) return false;
    uint32_t len = 0;
    std::memcpy(&len, data.data() + offset, sizeof(len));
    if (len > 260 || offset + 4 + len > data.size()) return false;
    out.assign(reinterpret_cast<const char*>(data.data() + offset + 4), len);
    return true;
}

bool TryParseTextureSlide(const std::vector<unsigned char>& data, size_t cmdOffset,
                          FrameTextureSlideData& outSlide) {
    std::string cmd;
    if (!ReadLengthPrefixedString(data, cmdOffset - 4, cmd)) return false;
    if (Lower(cmd) != "textureslide") return false;

    const size_t paramOffset = cmdOffset + cmd.size();
    if (paramOffset + 14 + 16 > data.size()) return false;

    uint8_t byte0 = 0;
    uint8_t byte1 = 0;
    float start = 0.0f;
    float end = 0.0f;
    float speed = 0.0f;
    std::memcpy(&byte0, data.data() + paramOffset, 1);
    std::memcpy(&byte1, data.data() + paramOffset + 1, 1);
    std::memcpy(&start, data.data() + paramOffset + 2, sizeof(float));
    std::memcpy(&end, data.data() + paramOffset + 6, sizeof(float));
    std::memcpy(&speed, data.data() + paramOffset + 10, sizeof(float));

    const size_t leftOffset = paramOffset + 14;
    if (leftOffset + 16 > data.size()) return false;

    Vector3 left{};
    uint32_t keyCount = 0;
    std::memcpy(&left.x, data.data() + leftOffset, sizeof(float));
    std::memcpy(&left.y, data.data() + leftOffset + 4, sizeof(float));
    std::memcpy(&left.z, data.data() + leftOffset + 8, sizeof(float));
    std::memcpy(&keyCount, data.data() + leftOffset + 12, sizeof(uint32_t));

    if (keyCount > 256) return false;
    const size_t keysOffset = leftOffset + 16;
    if (keysOffset + keyCount * 16 > data.size()) return false;

    std::vector<Vector4> keys;
    keys.reserve(keyCount);
    for (uint32_t i = 0; i < keyCount; ++i) {
        Vector4 key{};
        const size_t ko = keysOffset + i * 16;
        std::memcpy(&key.x, data.data() + ko, sizeof(float));
        std::memcpy(&key.y, data.data() + ko + 4, sizeof(float));
        std::memcpy(&key.z, data.data() + ko + 8, sizeof(float));
        std::memcpy(&key.w, data.data() + ko + 12, sizeof(float));
        keys.push_back(key);
    }

    outSlide.Left = left;
    outSlide.Keys = std::move(keys);
    outSlide.Start = start;
    outSlide.End = end;
    outSlide.Speed = speed;
    outSlide.Byte0 = byte0;
    outSlide.Byte1 = byte1;
    outSlide.Valid = left.x > 0.0f && left.y > 0.0f;
    return outSlide.Valid;
}

bool TryParseStaticEmit(const std::vector<unsigned char>& data, size_t cmdOffset,
                        EfpEmitterDef& outEmit) {
    std::string cmd;
    if (!ReadLengthPrefixedString(data, cmdOffset - 4, cmd)) return false;
    if (Lower(cmd) != "staticemit") return false;

    const size_t paramOffset = cmdOffset + cmd.size() + 4;
    if (paramOffset + 16 > data.size()) return false;

    int minP = 0;
    int maxP = 0;
    int burst = 0;
    float spawn = 0.0f;
    std::memcpy(&minP, data.data() + paramOffset, sizeof(int));
    std::memcpy(&maxP, data.data() + paramOffset + 4, sizeof(int));
    std::memcpy(&burst, data.data() + paramOffset + 8, sizeof(int));
    std::memcpy(&spawn, data.data() + paramOffset + 12, sizeof(float));

    if (minP < 0 || minP > 512 || maxP < 0 || maxP > 512 || burst < 0 || burst > 512) {
        return false;
    }
    if (spawn < 0.0f || spawn > 10000.0f) return false;
    if (minP == 0 && maxP == 0) return false;

    outEmit.MinParticles = minP;
    outEmit.MaxParticles = (std::max)(minP, maxP);
    outEmit.BurstRate = (std::max)(1, burst);
    outEmit.SpawnRate = spawn;
    outEmit.Valid = true;
    return true;
}

std::vector<ParsedSlideBlock> ScanTextureSlides(const std::vector<unsigned char>& data) {
    std::vector<ParsedSlideBlock> blocks;
    const char* token = "TextureSlide";
    const size_t tokenLen = std::strlen(token);
    size_t searchFrom = 0;
    while (searchFrom < data.size()) {
        auto it = std::search(data.begin() + searchFrom, data.end(),
                              reinterpret_cast<const unsigned char*>(token),
                              reinterpret_cast<const unsigned char*>(token) + tokenLen);
        if (it == data.end()) break;
        const size_t cmdOffset = static_cast<size_t>(it - data.begin());
        if (cmdOffset >= 4) {
            FrameTextureSlideData slide;
            if (TryParseTextureSlide(data, cmdOffset, slide) && slide.HasKeys()) {
                blocks.push_back({cmdOffset, std::move(slide)});
            }
        }
        searchFrom = cmdOffset + tokenLen;
    }
    return blocks;
}

std::vector<ParsedEmitBlock> ScanStaticEmits(const std::vector<unsigned char>& data) {
    std::vector<ParsedEmitBlock> blocks;
    const char* token = "StaticEmit";
    const size_t tokenLen = std::strlen(token);
    size_t searchFrom = 0;
    while (searchFrom < data.size()) {
        auto it = std::search(data.begin() + searchFrom, data.end(),
                              reinterpret_cast<const unsigned char*>(token),
                              reinterpret_cast<const unsigned char*>(token) + tokenLen);
        if (it == data.end()) break;
        const size_t cmdOffset = static_cast<size_t>(it - data.begin());
        if (cmdOffset >= 4) {
            EfpEmitterDef emit;
            if (TryParseStaticEmit(data, cmdOffset, emit)) {
                blocks.push_back({cmdOffset, emit});
            }
        }
        searchFrom = cmdOffset + tokenLen;
    }
    return blocks;
}

const FrameTextureSlideData* PickNearestSlide(const std::vector<ParsedSlideBlock>& slides,
                                              const std::string& texturePath,
                                              size_t itemOffset) {
    if (slides.empty() || itemOffset == SIZE_MAX) return nullptr;

    const std::string lower = Lower(texturePath);
    const bool textureLooksAnimated = lower.find("fire") != std::string::npos ||
                                      lower.find("flame") != std::string::npos ||
                                      lower.find("frame") != std::string::npos ||
                                      lower.find("water") != std::string::npos ||
                                      lower.find("papyun") != std::string::npos ||
                                      lower.find("smoke") != std::string::npos ||
                                      lower.find("spark") != std::string::npos;
    if (!textureLooksAnimated) return nullptr;

    const FrameTextureSlideData* best = nullptr;
    size_t bestDist = SIZE_MAX;
    for (const ParsedSlideBlock& block : slides) {
        if (!block.Slide.HasKeys()) continue;
        const size_t dist = (block.Offset > itemOffset) ? block.Offset - itemOffset
                                                        : itemOffset - block.Offset;
        if (dist < bestDist) {
            bestDist = dist;
            best = &block.Slide;
        }
    }

    constexpr size_t kMaxSlideAssociationDistance = 512;
    if (best && bestDist <= kMaxSlideAssociationDistance) {
        return best;
    }

    return nullptr;
}

const EfpEmitterDef* PickNearestEmit(const std::vector<ParsedEmitBlock>& emits, size_t itemOffset) {
    if (emits.empty() || itemOffset == SIZE_MAX) return nullptr;
    const ParsedEmitBlock* best = nullptr;
    size_t bestDist = SIZE_MAX;
    for (const ParsedEmitBlock& block : emits) {
        const size_t dist = (block.Offset > itemOffset) ? block.Offset - itemOffset
                                                        : itemOffset - block.Offset;
        if (dist < bestDist) {
            bestDist = dist;
            best = &block;
        }
    }
    return best ? &best->Emitter : nullptr;
}

EfpEmitterDef DefaultEmitterForItem(const EfpRenderItem& item) {
    EfpEmitterDef emit;
    emit.MinParticles = 6;
    emit.MaxParticles = 24;
    emit.BurstRate = 1;
    emit.SpawnRate = 1.0f;
    emit.Valid = item.TextureSlide || item.SlideData.Valid;
    if (!item.TexturePaths.empty()) {
        const std::string lower = Lower(item.TexturePaths.front());
        if (lower.find("fire") != std::string::npos || lower.find("flame") != std::string::npos) {
            emit.MinParticles = 12;
            emit.MaxParticles = 40;
        }
    }
    return emit;
}

size_t FindBytes(const std::vector<unsigned char>& data, const std::string& needle) {
    if (needle.empty()) return SIZE_MAX;
    auto it = std::search(data.begin(), data.end(), needle.begin(), needle.end());
    if (it == data.end()) return SIZE_MAX;
    return static_cast<size_t>(it - data.begin());
}

size_t FindRenderItemOffset(const std::vector<unsigned char>& data, const EfpRenderItem& item) {
    size_t best = SIZE_MAX;
    auto consider = [&](const std::string& value) {
        const size_t offset = FindBytes(data, value);
        if (offset != SIZE_MAX && offset < best) {
            best = offset;
        }
    };

    if (!item.MeshPath.empty()) {
        consider(item.MeshPath);
    }
    if (!item.TexturePaths.empty()) {
        consider(item.TexturePaths.front());
    }
    return best;
}

std::string ScanLifeCommandNearOffset(const std::vector<unsigned char>& data, size_t approxOffset) {
    auto findNearest = [&](const char* token) -> std::pair<size_t, bool> {
        const size_t len = strlen(token);
        size_t bestDist = SIZE_MAX;
        bool found = false;
        for (size_t i = 0; i + len <= data.size(); ++i) {
            if (memcmp(data.data() + i, token, len) != 0) continue;
            const size_t dist = (i > approxOffset) ? i - approxOffset : approxOffset - i;
            if (dist < bestDist) {
                bestDist = dist;
                found = true;
            }
        }
        return {bestDist, found};
    };

    const auto loopHit = findNearest("NormalTimeLoop");
    const auto extinctHit = findNearest("NormalTimeExtinct");
    if (extinctHit.second && (!loopHit.second || extinctHit.first <= loopHit.first)) {
        return "NormalTimeExtinct";
    }
    if (loopHit.second) {
        return "NormalTimeLoop";
    }
    return "NormalTimeLoop";
}

void AttachBinaryData(EfpResource& out, const std::vector<unsigned char>& data) {
    const std::vector<ParsedSlideBlock> slides = ScanTextureSlides(data);
    const std::vector<ParsedEmitBlock> emits = ScanStaticEmits(data);

    if (!slides.empty()) {
        out.HasTextureSlide = true;
    }

    for (EfpRenderItem& item : out.RenderItems) {
        const size_t approxOffset = FindRenderItemOffset(data, item);
        if (!item.TexturePaths.empty()) {
            const FrameTextureSlideData* slide = PickNearestSlide(slides, item.TexturePaths.front(),
                                                                  approxOffset);
            if (slide) {
                item.SlideData = *slide;
                item.TextureSlide = true;
            }
        }
        const EfpEmitterDef* emit = PickNearestEmit(emits, approxOffset);
        if (emit && emit->Valid) {
            item.Emitter = *emit;
            item.HasEmitter = true;
        }
    }

    out.Objects.clear();
    size_t objectIndex = 0;
    for (const EfpRenderItem& item : out.RenderItems) {
        if (!item.TextureSlide && !item.SlideData.Valid && !item.HasEmitter) continue;
        EfpObjectDef obj;
        obj.Emitter = item.HasEmitter ? item.Emitter : DefaultEmitterForItem(item);
        obj.Render.Shape = item.Shape;
        obj.Render.ViewMode = item.ViewMode.empty() ? "ViewYBillboard" : item.ViewMode;
        obj.Render.MeshPath = item.MeshPath;
        obj.Render.TexturePaths = item.TexturePaths;
        obj.Render.TextureSlide = item.SlideData;
        obj.Render.SrcBlend = item.SrcBlend;
        obj.Render.DstBlend = item.DstBlend;
        obj.LifeCommand = ScanLifeCommandNearOffset(data, objectIndex * 512);
        ++objectIndex;
        out.Objects.push_back(std::move(obj));
    }

    if (out.Objects.empty() && !out.RenderItems.empty()) {
        size_t fallbackIndex = 0;
        for (const EfpRenderItem& item : out.RenderItems) {
            if (item.TexturePaths.empty()) continue;
            EfpObjectDef obj;
            obj.Render.Shape = item.Shape;
            obj.Render.ViewMode = item.ViewMode.empty() ? "ViewBillboard" : item.ViewMode;
            obj.Render.MeshPath = item.MeshPath;
            obj.Render.TexturePaths = item.TexturePaths;
            obj.Render.TextureSlide = item.SlideData;
            obj.Emitter.MinParticles = 4;
            obj.Emitter.MaxParticles = 16;
            obj.Emitter.BurstRate = 1;
            obj.Emitter.SpawnRate = 1.0f;
            obj.Emitter.Valid = item.TextureSlide;
            obj.LifeCommand = ScanLifeCommandNearOffset(data, fallbackIndex * 512);
            ++fallbackIndex;
            if (item.TextureSlide) {
                out.Objects.push_back(std::move(obj));
            }
        }
    }
}

bool ReadHeuristic(const std::vector<unsigned char>& data, EfpResource& out) {
    out.ParsedHeuristic = true;

    std::unordered_set<std::string> seenTextures;
    std::unordered_set<std::string> seenMeshes;

    std::vector<std::string> strings = ExtractLengthPrefixedStrings(data);
    MergeUniqueStrings(strings, ExtractAsciiStrings(data));

    for (const std::string& s : strings) {
        const std::string lower = Lower(s);
        if (lower == "renderplate") out.HasRenderPlate = true;
        if (lower == "rendermesh") out.HasRenderMesh = true;
        if (lower.find("viewbillboard") != std::string::npos ||
            lower.find("viewybillboard") != std::string::npos ||
            lower.find("viewvbillboard") != std::string::npos) {
            out.HasBillboard = true;
        }
        if (lower == "textureslide") out.HasTextureSlide = true;

        if (IsTexturePath(s)) {
            PushUnique(out.TexturePaths, seenTextures, s);
        } else if (IsMeshPath(s)) {
            PushUnique(out.MeshPaths, seenMeshes, s);
        }
    }

    std::set<std::string> seenItems;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (IsMeshPath(strings[i])) {
            const std::string viewMode = FindNearbyViewMode(strings, i, 5);
            const std::string shapeNearby = FindNearbyShape(strings, i, 5);

            EfpRenderItem item;
            item.ViewMode = viewMode.empty() ? "ViewBillboard" : viewMode;
            item.MeshPath = strings[i];

            for (size_t j = i + 1; j < strings.size() && j <= i + 6; ++j) {
                if (IsTexturePath(strings[j])) {
                    item.TexturePaths.push_back(strings[j]);
                    break;
                }
                if (IsMeshPath(strings[j])) break;
            }
            if (item.TexturePaths.empty()) {
                const size_t begin = (i > 6) ? i - 6 : 0;
                for (size_t j = i; j-- > begin;) {
                    if (IsTexturePath(strings[j])) {
                        item.TexturePaths.push_back(strings[j]);
                        break;
                    }
                    if (IsMeshPath(strings[j])) break;
                }
            }

            if (!shapeNearby.empty()) {
                item.Shape = shapeNearby;
            } else if (item.ViewMode == "ViewNone") {
                item.Shape = "RenderMesh";
            } else {
                item.Shape = "RenderPlate";
            }
            PushRenderItem(out.RenderItems, seenItems, item);
        }

        if (IsTexturePath(strings[i])) {
            bool plateNearby = false;
            bool meshNearby = false;
            const size_t begin = (i > 5) ? i - 5 : 0;
            const size_t end = (std::min)(strings.size(), i + 6);
            for (size_t j = begin; j < end; ++j) {
                if (IsRenderPlate(strings[j])) plateNearby = true;
                if (IsRenderMesh(strings[j])) meshNearby = true;
            }
            if (plateNearby || meshNearby) {
                EfpRenderItem item;
                item.Shape = meshNearby ? "RenderMesh" : "RenderPlate";
                item.TexturePaths.push_back(strings[i]);
                item.ViewMode = FindNearbyViewMode(strings, i, 5);
                if (item.ViewMode.empty()) {
                    item.ViewMode = meshNearby ? "ViewNone" : "ViewBillboard";
                }
                PushRenderItem(out.RenderItems, seenItems, std::move(item));
            }
        }
    }

    CollapseDuplicatePlateItems(out.RenderItems);
    return out.HasRenderPlate || out.HasRenderMesh || !out.TexturePaths.empty() ||
           !out.MeshPaths.empty() || !out.RenderItems.empty();
}

} // namespace

bool EfpParser::Read(const std::wstring& path, EfpResource& out) {
    out = {};

    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return false;

    std::vector<unsigned char> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    if (data.size() < 12) return false;

    out.Signature.assign(reinterpret_cast<const char*>(data.data()),
                         reinterpret_cast<const char*>(data.data() + (std::min<size_t>)(12, data.size())));
    if (out.Signature.rfind("JMXVEFF", 0) != 0) return false;

    if (!ReadHeuristic(data, out)) {
        return false;
    }

    out.ParsedHeuristic = false;
    AttachBinaryData(out, data);
    return true;
}
