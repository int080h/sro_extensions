#include "core/core_config.hpp"
#include "core/core_main.hpp"
#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "plugins/net_log/net_log_capture.hpp"
#include "utils/hooks.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>

// These checks use real extension code and synthetic x86 functions; no game is launched.
namespace {
  using namespace ext_client;
  using namespace ext_client::core;
  auto require(bool condition, const char *description) -> void {
    if (!condition)
      throw std::runtime_error(description);
  }
  auto until(const std::atomic<bool> &flag) -> void {
    const auto start = GetTickCount();
    while (!flag.load()) {
      if (GetTickCount() - start > 5000)
        throw std::runtime_error("thread synchronization timed out");
      Sleep(1);
    }
  }

  auto check_config() -> void {
    char original_directory[MAX_PATH]{}, temporary_root[MAX_PATH]{}, test_directory[MAX_PATH]{};
    GetCurrentDirectoryA(MAX_PATH, original_directory);
    GetTempPathA(MAX_PATH, temporary_root);
    require(GetTempFileNameA(temporary_root, "ext", 0, test_directory) != 0, "temporary config directory name");
    DeleteFileA(test_directory);
    require(CreateDirectoryA(test_directory, nullptr) && SetCurrentDirectoryA(test_directory),
            "temporary config directory");
    const auto initial = config::runtime();
    config::data().net.max_entries = 37;
    config::mark_dirty();
    config::publish_runtime();
    const auto published = config::runtime();
    require(published->net.max_entries == 37 && initial->net.max_entries != 37, "settings snapshots remain immutable");
    config::publish_runtime();
    require(config::runtime() == published, "unchanged settings skip publication");
    require(GetFileAttributesA(config::path()) == INVALID_FILE_ATTRIBUTES, "mark dirty does not write a file");
    config::flush_pending();
    require(config::is_dirty(), "autosave waits for debounce");
    Sleep(550);
    config::flush_pending();
    require(!config::is_dirty() && GetFileAttributesA(config::path()) != INVALID_FILE_ATTRIBUTES,
            "debounced autosave commits");
    require(GetFileAttributesA("ext_client.ini.tmp") == INVALID_FILE_ATTRIBUTES,
            "atomic replacement consumes temporary file");

    const auto locked = CreateFileA(config::path(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    require(locked != INVALID_HANDLE_VALUE, "lock config against replacement");
    config::data().net.max_entries = 42;
    config::mark_dirty();
    config::publish_runtime();
    require(!config::save() && config::is_dirty(), "failed replacement preserves pending changes");
    CloseHandle(locked);
    require(config::load() && config::data().net.max_entries == 37,
            "failed replacement preserves previous config contents");

    std::atomic<bool> done{false}, failed{false};
    std::thread reader([&] {
      while (!done.load()) {
        const auto snapshot = config::runtime();
        if (snapshot->net.max_entries < 37 || snapshot->net.max_entries > 99)
          failed.store(true);
      }
    });
    for (int i = 37; i < 100; ++i) {
      std::lock_guard lock(event::dispatch_mutex());
      config::data().net.max_entries = i;
      config::mark_dirty();
      config::publish_runtime();
    }
    done.store(true);
    reader.join();
    require(!failed.load() && published->net.max_entries == 37,
            "concurrent readers retain coherent settings snapshots");
    config::data().net.max_entries = 2048;
    config::mark_dirty();
    config::publish_runtime();
    DeleteFileA("ext_client.ini");
    DeleteFileA("ext_client.ini.tmp");
    SetCurrentDirectoryA(original_directory);
    RemoveDirectoryA(test_directory);
    std::puts("PASS immutable settings, debounce, concurrent publication and failed-save recovery");
  }

  auto check_packets() -> void {
    using namespace plugins::net_log;
    log_entry equal{};
    for (int column = 0; column <= 6; ++column) {
      require(!packet_less(equal, equal, column, true), "ascending equality must be unordered");
      require(!packet_less(equal, equal, column, false), "descending equality must be unordered");
    }
    std::vector<log_entry> sortable(300);
    for (unsigned i = 0; i < sortable.size(); ++i) {
      sortable[i].id = i;
      sortable[i].opcode = static_cast<std::uint16_t>(i % 7);
    }
    std::stable_sort(sortable.begin(), sortable.end(),
                     [](const auto &a, const auto &b) { return packet_less(a, b, 3, false); });
    for (std::size_t i = 1; i < sortable.size(); ++i) {
      require(sortable[i - 1].opcode >= sortable[i].opcode, "descending opcode sort");
      if (sortable[i - 1].opcode == sortable[i].opcode)
        require(sortable[i - 1].id < sortable[i].id, "equal opcodes retain capture order");
    }

    clear_log();
    std::vector<log_entry> snapshot;
    std::uint64_t revision = 0, epoch = 0;
    event::packet_context ctx{};
    const auto append = [&] {
      push_entry(packet_direction::client_to_server, event::packet_layer::stream, 0x1234,
                 std::vector<std::uint8_t>(5000, 0x42), ctx, false, false, "check");
    };
    append();
    require(refresh_log_entries(snapshot, revision, epoch), "first snapshot refresh");
    require(snapshot.size() == 1 && snapshot[0].payload.size() == 4096 && snapshot[0].payload_size == 5000,
            "capture caps stored payload and preserves full size");
    snapshot[0].has_parsed = true;
    const auto retained = snapshot[0].payload.data();
    const auto retained_id = snapshot[0].id;
    require(!refresh_log_entries(snapshot, revision, epoch), "unchanged capture skips refresh");
    append();
    require(refresh_log_entries(snapshot, revision, epoch), "new capture refreshes");
    require(snapshot[0].payload.data() == retained && snapshot[0].has_parsed,
            "refresh retains payload allocation and parser cache");
    for (std::size_t i = 0; i < k_log_ring_capacity; ++i)
      append();
    refresh_log_entries(snapshot, revision, epoch);
    require(snapshot.size() == k_log_ring_capacity && snapshot.front().id > retained_id,
            "ring eviction yields chronological surviving entries");
    auto imported = snapshot;
    imported[0].has_parsed = false;
    snapshot[0].has_parsed = true;
    replace_log_entries(std::move(imported));
    refresh_log_entries(snapshot, revision, epoch);
    require(!snapshot[0].has_parsed, "import invalidates cache even with matching IDs");
    clear_log();
    refresh_log_entries(snapshot, revision, epoch);
    require(snapshot.empty(), "clear invalidates snapshot");
    std::puts("PASS packet sort, capture bounds, cache retention, ring eviction and import");
  }

  using int_fn = int(__cdecl *)(int);
  int_fn gateway = nullptr;
  std::atomic<bool> helper_entered{false}, helper_release{false};
  auto __cdecl blocking_helper(int value) -> int {
    helper_entered.store(true);
    until(helper_release);
    return value + 1;
  }
  auto __cdecl detour(int value) -> int {
    utils::hook_call_scope scope;
    return gateway(value) + 10;
  }
  auto check_hooks() -> void {
    require(utils::hook_lib_init(), "hook initialization");
    auto *code =
        static_cast<unsigned char *>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    require(code != nullptr, "synthetic executable page");
    // mov eax,[esp+4]; add eax,1; ret
    const unsigned char body[] = {0x8B, 0x44, 0x24, 0x04, 0x83, 0xC0, 0x01, 0xC3};
    std::memcpy(code, body, sizeof(body));
    auto target = reinterpret_cast<int_fn>(code);
    require(target(2) == 3, "unhooked synthetic function");
    require(utils::apply_detour(reinterpret_cast<char *>(code), reinterpret_cast<char *>(&detour),
                                reinterpret_cast<char **>(&gateway)),
            "apply synthetic detour");
    require(target(2) == 13 && gateway(2) == 3, "detour and relocated original");
    require(utils::remove_detour(reinterpret_cast<char *>(code)), "disable detour");
    require(target(2) == 3 && gateway(2) == 3, "disabled gateway remains valid");
    require(utils::reapply_detour(reinterpret_cast<char *>(code)) && target(2) == 13, "reapply detour");

    std::atomic<bool> done{false}, failed{false}, ready{false};
    std::thread caller([&] {
      ready.store(true);
      while (!done.load()) {
        const auto value = target(2);
        if (value != 3 && value != 13)
          failed.store(true);
      }
    });
    until(ready);
    for (int i = 0; i < 300; ++i) {
      require(utils::remove_detour(reinterpret_cast<char *>(code)), "concurrent disable");
      require(utils::reapply_detour(reinterpret_cast<char *>(code)), "concurrent enable");
    }
    done.store(true);
    caller.join();
    require(!failed.load(), "thread relocation preserves original/detour results");
    require(utils::remove_detour(reinterpret_cast<char *>(code)), "final disable");

    auto *blocking = code + 64;
    // push [esp+4]; mov eax,helper; call eax; add esp,4; ret
    const unsigned char blocking_body[] = {0xFF, 0x74, 0x24, 0x04, 0xB8, 0,    0,   0,
                                           0,    0xFF, 0xD0, 0x83, 0xC4, 0x04, 0xC3};
    std::memcpy(blocking, blocking_body, sizeof(blocking_body));
    const auto helper = reinterpret_cast<std::uintptr_t>(&blocking_helper);
    std::memcpy(blocking + 5, &helper, sizeof(helper));
    require(utils::apply_detour(reinterpret_cast<char *>(blocking), reinterpret_cast<char *>(&detour),
                                reinterpret_cast<char **>(&gateway)),
            "apply blocking detour");
    int result = 0;
    std::thread in_flight([&] { result = reinterpret_cast<int_fn>(blocking)(2); });
    until(helper_entered);
    require(utils::remove_detour(reinterpret_cast<char *>(blocking)), "unpatch in-flight original");
    require(!utils::wait_for_hook_calls(10), "drain rejects active original call");
    helper_release.store(true);
    in_flight.join();
    require(result == 13 && utils::wait_for_hook_calls(1000), "original returns safely after unpatch");
    require(utils::hook_lib_shutdown(), "drain and free gateways");
    VirtualFree(code, 0, MEM_RELEASE);
    std::puts("PASS hook apply, disable, reapply, concurrent patching and in-flight drain");
  }

  auto check_events() -> void {
    auto &manager = plugin::plugin_manager::get();
    manager.register_plugin("checks", "Checks");
    using handler = event::event_handler<500, event::cb_void>;
    auto &callbacks = handler::instance();
    int first = 0, late = 0;
    callbacks.add(
        [&] {
          ++first;
          if (first == 1)
            callbacks.add([&] { ++late; }, "checks", "late");
        },
        "checks", "first");
    callbacks.trigger();
    require(first == 1 && late == 0, "registration during dispatch takes effect next event");
    callbacks.trigger();
    require(first == 2 && late == 1, "next event observes new callback");
    manager.set_plugin_enabled("checks", false);
    callbacks.trigger();
    require(first == 2 && !callbacks.has_active_listeners(), "disabled owner filters callbacks");
    int shutdown_calls = 0;
    event::event_handler<EVENT_ON_SHUTDOWN>::instance().add([&] { ++shutdown_calls; }, "checks", "cleanup");
    event::event_handler<EVENT_ON_SHUTDOWN>::instance().trigger();
    require(shutdown_calls == 1, "disabled plugins still clean up");
    manager.set_plugin_enabled("checks", true);
    callbacks.trigger_owner("checks");
    require(first == 3 && late == 2, "owner trigger observes enable state");

    using concurrent = event::event_handler<501, event::cb_void>;
    std::atomic<unsigned> calls{0};
    std::atomic<bool> registered{false};
    std::thread registrar([&] {
      for (int i = 0; i < 100; ++i)
        concurrent::instance().add([&] { ++calls; }, "checks", "concurrent");
      registered.store(true);
    });
    while (!registered.load())
      concurrent::instance().trigger();
    registrar.join();
    calls.store(0);
    concurrent::instance().trigger();
    require(calls.load() == 100, "concurrent registration publishes complete callback lists");

    std::atomic<bool> entered{false}, release{false}, stopped{false};
    using blocking = event::event_handler<502, event::cb_void>;
    blocking::instance().add(
        [&] {
          entered.store(true);
          until(release);
        },
        "checks", "blocking");
    std::thread dispatch([] { blocking::instance().trigger(); });
    until(entered);
    std::thread stopper([&] {
      event::stop_dispatch();
      stopped.store(true);
    });
    Sleep(20);
    require(!stopped.load(), "shutdown barrier waits for active callback");
    release.store(true);
    dispatch.join();
    stopper.join();
    callbacks.trigger();
    require(first == 3, "stopped dispatch refuses game callbacks");
    event::event_handler<EVENT_ON_SHUTDOWN>::instance().trigger();
    require(shutdown_calls == 2, "shutdown callbacks survive dispatch stop");
    std::puts("PASS callback snapshots, concurrent registration, owner toggles and shutdown barrier");
  }
} // namespace

namespace ext_client::core {
  auto app_main::get() -> app_main & {
    static app_main instance;
    return instance;
  }
  auto app_main::get_module() const noexcept -> HMODULE {
    return nullptr;
  }
} // namespace ext_client::core
namespace ext_client::utils {
  auto log_msg(const char *, ...) -> void {}
} // namespace ext_client::utils

int main() {
  try {
    check_config();
    check_packets();
    check_hooks();
    check_events();
    return 0;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
  }
}
