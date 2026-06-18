#pragma once

#include "index/TextDataCatalog.h"

#include "index/ObjectIndex.h"

#include "Parsers/BsrParser.h"

#include "rendering/MeshRenderer.h"

#include <string>

#include <vector>



struct ObjectInspectNode {

    std::string label;

    std::string value;

    std::vector<ObjectInspectNode> children;

};



struct ObjectInspectReport {

    std::string title;

    std::string primaryModelPath;

    std::string modelPathSource;

    std::vector<ObjectInspectNode> sections;

    BsrResource bsr;

    bool bsrLoaded = false;

    int missingTextureCount = 0;

};



class ObjectInspector {

public:

    static ObjectInspectReport BuildFromGameObject(const sro::GameObjectRef& ref, MeshRenderer* mr,

                                                   const sro::TextDataCatalog* textData = nullptr);

    static ObjectInspectReport BuildFromBsrPath(const std::string& bsrPath, MeshRenderer* mr,

                                                const std::wstring& clientPath);

    static ObjectInspectReport BuildFromObjId(uint32_t objId, const sro::ObjectIndex& objectIndex, MeshRenderer* mr,

                                              const std::wstring& clientPath);

};

