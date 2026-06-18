#pragma once

#include <d3d9.h>
#include <windows.h>

#include <string>
#include <vector>

// Minimal D3DX shader assembler bindings via d3dx9_43.dll (no legacy DXSDK headers required).
class D3dx9ShaderAssembler {
public:
    static D3dx9ShaderAssembler& Instance();

    bool Available() const { return m_available; }

    bool AssembleFile(const std::wstring& path, std::vector<DWORD>& outBytecode,
                      std::string& outError);

private:
    D3dx9ShaderAssembler();
    ~D3dx9ShaderAssembler();

    struct ID3DXBuffer : IUnknown {
        virtual LPVOID STDMETHODCALLTYPE GetBufferPointer() = 0;
        virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0;
    };

    using AssembleShaderFn = HRESULT(WINAPI*)(LPCSTR, UINT, const DWORD*, void*, DWORD,
                                              ID3DXBuffer**, ID3DXBuffer**);

    HMODULE m_module = nullptr;
    AssembleShaderFn m_assemble = nullptr;
    bool m_available = false;
};
