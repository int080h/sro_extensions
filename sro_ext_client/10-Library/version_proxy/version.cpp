#include <windows.h>

#pragma comment(linker, "/export:GetFileVersionInfoA=real_version.GetFileVersionInfoA")
#pragma comment(linker, "/export:GetFileVersionInfoByHandle=real_version.GetFileVersionInfoByHandle")
#pragma comment(linker, "/export:GetFileVersionInfoExA=real_version.GetFileVersionInfoExA")
#pragma comment(linker, "/export:GetFileVersionInfoExW=real_version.GetFileVersionInfoExW")
#pragma comment(linker, "/export:GetFileVersionInfoSizeA=real_version.GetFileVersionInfoSizeA")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExA=real_version.GetFileVersionInfoSizeExA")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExW=real_version.GetFileVersionInfoSizeExW")
#pragma comment(linker, "/export:GetFileVersionInfoSizeW=real_version.GetFileVersionInfoSizeW")
#pragma comment(linker, "/export:GetFileVersionInfoW=real_version.GetFileVersionInfoW")
#pragma comment(linker, "/export:VerFindFileA=real_version.VerFindFileA")
#pragma comment(linker, "/export:VerFindFileW=real_version.VerFindFileW")
#pragma comment(linker, "/export:VerInstallFileA=real_version.VerInstallFileA")
#pragma comment(linker, "/export:VerInstallFileW=real_version.VerInstallFileW")
#pragma comment(linker, "/export:VerLanguageNameA=real_version.VerLanguageNameA")
#pragma comment(linker, "/export:VerLanguageNameW=real_version.VerLanguageNameW")
#pragma comment(linker, "/export:VerQueryValueA=real_version.VerQueryValueA")
#pragma comment(linker, "/export:VerQueryValueW=real_version.VerQueryValueW")

auto WINAPI DllMain(HMODULE module, DWORD reason, LPVOID) -> BOOL {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(module);
    LoadLibraryA("ext_client.dll");
  }
  return TRUE;
}
