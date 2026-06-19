#pragma once

// Shutdown tiers:
// - Graceful (F7 / menu unload): full teardown + DLL unload via FreeLibraryAndExitThread
// - Game exit (PostQuitMessage): full teardown + TerminateProcess
// - Watchdog (shutdown_guard): teardown then hard kill after a short delay
// - Fast kill (ExitProcess / CPSQuit intercept): no teardown; breaks deadlocks during game exit

namespace ext_client::hooks::cps_quit {

  auto install() -> bool;
  auto uninstall() -> void;

} // namespace ext_client::hooks::cps_quit
