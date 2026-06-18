#include "rendering/D3dx9ShaderAssembler.h"

#include <fstream>
#include <string>

D3dx9ShaderAssembler& D3dx9ShaderAssembler::Instance() {
    static D3dx9ShaderAssembler inst;
    return inst;
}

D3dx9ShaderAssembler::D3dx9ShaderAssembler() {
    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::wstring exeDir = modulePath;
    const size_t slash = exeDir.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        exeDir.resize(slash);
    }

    const std::wstring searchDirs[] = {
        exeDir,
        exeDir + L"/..",
        L".",
    };
    const wchar_t* names[] = {
        L"d3dx9_43.dll",
        L"d3dx9_42.dll",
        L"d3dx9_41.dll",
    };

    for (const auto& dir : searchDirs) {
        for (const wchar_t* name : names) {
            const std::wstring path = dir + L"\\" + name;
            m_module = LoadLibraryW(path.c_str());
            if (m_module) break;
        }
        if (m_module) break;
    }

    if (!m_module) {
        for (const wchar_t* name : names) {
            m_module = LoadLibraryW(name);
            if (m_module) break;
        }
    }
    if (!m_module) return;

    m_assemble = reinterpret_cast<AssembleShaderFn>(GetProcAddress(m_module, "D3DXAssembleShader"));
    m_available = m_assemble != nullptr;
}

D3dx9ShaderAssembler::~D3dx9ShaderAssembler() {
    if (m_module) FreeLibrary(m_module);
}

bool D3dx9ShaderAssembler::AssembleFile(const std::wstring& path, std::vector<DWORD>& outBytecode,
                                        std::string& outError) {
    outBytecode.clear();
    outError.clear();
    if (!m_available) {
        outError = "d3dx9_43.dll not available";
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        outError = "failed to open shader file";
        return false;
    }
    const std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (source.empty()) {
        outError = "shader file is empty";
        return false;
    }

    ID3DXBuffer* shader = nullptr;
    ID3DXBuffer* errors = nullptr;
    const HRESULT hr = m_assemble(source.data(), static_cast<UINT>(source.size()), nullptr, nullptr,
                                  0, &shader, &errors);
    if (FAILED(hr) || !shader) {
        if (errors) {
            outError.assign(static_cast<const char*>(errors->GetBufferPointer()),
                            errors->GetBufferSize());
            errors->Release();
        } else {
            outError = "D3DXAssembleShader failed (hr=" + std::to_string(hr) + ")";
        }
        if (shader) shader->Release();
        return false;
    }

    const DWORD* bytes = static_cast<const DWORD*>(shader->GetBufferPointer());
    const size_t dwordCount = shader->GetBufferSize() / sizeof(DWORD);
    outBytecode.assign(bytes, bytes + dwordCount);
    shader->Release();
    return !outBytecode.empty();
}
