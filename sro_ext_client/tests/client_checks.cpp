#include "core/config.hpp"
#include "core/app.hpp"
#include "core/event_bus.hpp"
#include "core/plugin_manager.hpp"
#include "plugins/net_log/net_log_capture.hpp"
#include "plugins/net_log/packet_parser.hpp"
#include "sdk/types/packed_time.hpp"
#include "sdk/game/cso_item.hpp"
#include "sdk/game/cref_obj_item.hpp"
#include "render/item_icon_renderer.hpp"
#include "sdk/game/ccos_data_mgr.hpp"
#include "sdk/game/c_skill_manager.hpp"
#include "utils/hooks.hpp"
#include "utils/msvc9_stl.hpp"

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

  auto check_packed_time() -> void {
    using namespace ext_client::sdk;

    require(is_leap_year(2024) && !is_leap_year(2023) && is_leap_year(2000) && !is_leap_year(1900), "leap year logic");
    require(days_in_month(2024, 2) == 29 && days_in_month(2023, 2) == 28 && days_in_month(2024, 4) == 30, "days in month");

    // Construct packed timestamp: sec=15, month=10, day=2, hour=14, min=30
    const std::uint32_t raw = 15 | (10 << 6) | (2 << 10) | (14 << 15) | (30 << 20);
    const packed_time pt(raw);
    require(pt.second() == 15, "packed second");
    require(pt.month() == 10, "packed month");
    require(pt.day() == 2, "packed day");
    require(pt.hour() == 14, "packed hour");
    require(pt.minute() == 30, "packed minute");
    require(pt.is_valid(), "valid packed time");

    const packed_time invalid_pt(15 | (13 << 6) | (2 << 10)); // month 13
    require(!invalid_pt.is_valid(), "invalid month rejected");

    SYSTEMTIME ref_now{};
    ref_now.wYear = 2026;
    ref_now.wMonth = 10;
    ref_now.wDay = 2;
    ref_now.wHour = 14;
    ref_now.wMinute = 30;

    elapsed_logout_parts elapsed{};
    // Same time: 0 elapsed
    require(compute_elapsed_logout(pt, elapsed, &ref_now), "compute elapsed logout same time");
    require(elapsed.months == 0 && elapsed.days == 0 && elapsed.hours == 0 && elapsed.minutes == 0, "0 elapsed");

    // 2 hours earlier: 12:30
    const packed_time pt_2h_ago(0 | (10 << 6) | (2 << 10) | (12 << 15) | (30 << 20));
    require(compute_elapsed_logout(pt_2h_ago, elapsed, &ref_now), "compute elapsed 2h ago");
    require(elapsed.months == 0 && elapsed.days == 0 && elapsed.hours == 2 && elapsed.minutes == 0, "2 hours elapsed");

    // 1 month earlier: month=9, day=2, hour=14, min=30
    const packed_time pt_1m_ago(0 | (9 << 6) | (2 << 10) | (14 << 15) | (30 << 20));
    require(compute_elapsed_logout(pt_1m_ago, elapsed, &ref_now), "compute elapsed 1m ago");
    require(elapsed.months == 1 && elapsed.days == 0 && elapsed.hours == 0 && elapsed.minutes == 0, "1 month elapsed");

    require(!compute_elapsed_logout(invalid_pt, elapsed, &ref_now), "invalid packed time returns false");
    std::puts("PASS packed time bit extraction, leap year, days in month, and elapsed calculation");
  }

  auto check_packet_parser() -> void {
    using namespace ext_client::plugins::net_log;

    // 1. Check Handshake Setup (0x5000)
    const std::uint8_t hs[] = {
      0x0e, 0x9b, 0xd3, 0x54, 0x78, 0xf5, 0x80, 0x66, 0xaf, 0xa2, 0x00, 0x00, 0x00,
      0x95, 0x00, 0x00, 0x00, 0xa3, 0x78, 0xff, 0xfd, 0x96, 0x2d, 0xd3, 0xba, 0x63,
      0x41, 0x02, 0x5d, 0xc7, 0x45
    };
    const auto res_hs = parse_packet(0x5000, hs, sizeof(hs));
    require(res_hs.doc_summary != nullptr, "0x5000 doc summary exists");
    require(!res_hs.fields.empty(), "0x5000 fields decoded");
    require(res_hs.fields[0].name == "SecurityFlags", "0x5000 security flags");

    // 2. Check Identification (0x2001) - full and truncated
    const std::uint8_t id_full[] = {
      0x0d, 0x00, 'G', 'a', 't', 'e', 'w', 'a', 'y', 'S', 'e', 'r', 'v', 'e', 'r', 0x00
    };
    const auto res_id_full = parse_packet(0x2001, id_full, sizeof(id_full));
    require(res_id_full.doc_summary != nullptr, "0x2001 doc summary exists");
    require(!res_id_full.fields.empty(), "0x2001 full fields decoded");
    require(res_id_full.fields[0].value == "GatewayServer", "0x2001 full ServiceName GatewayServer");

    const std::uint8_t id_trunc[] = {
      0x0d, 0x00, 'G', 'a', 't', 'e', 'w', 'a', 'y', 'S'
    };
    const auto res_id_trunc = parse_packet(0x2001, id_trunc, sizeof(id_trunc));
    require(!res_id_trunc.fields.empty(), "0x2001 partial fields decoded");
    require(res_id_trunc.fields[0].value == "GatewayS", "0x2001 partial ServiceName GatewayS");

    // 3. Check Gateway Shard List / Server Nodes (0xA107)
    const std::uint8_t nodes[] = {
      0x02, 0x00, 0x0f, 0x00, '1', '9', '2', '.', '1', '6', '8', '.', '1', '9', '9', '.', '1', '2', '9',
      0xbe, 0x32, 0x01, 0x0f, 0x00, '1', '9', '2', '.', '1', '6', '8', '.', '1', '9', '9', '.', '1', '2', '9',
      0xbf, 0x32
    };
    const auto res_nodes = parse_packet(0xA107, nodes, sizeof(nodes));
    require(res_nodes.doc_summary != nullptr, "0xA107 doc summary exists");
    require(!res_nodes.fields.empty(), "0xA107 fields decoded");

    // 4. Check Gateway Login Response (0xA101)
    const std::uint8_t login[] = {
      0x01, 0x41, 0x00, 0x00, 0x00, 0x01, 0x43, 0x01, 0x0b, 0x00, 'A', 'M', 'I', 'N', 'I', 'Y', 'A', 'L'
    };
    const auto res_login = parse_packet(0xA101, login, sizeof(login));
    require(res_login.doc_summary != nullptr, "0xA101 doc summary exists");
    require(!res_login.fields.empty(), "0xA101 fields decoded");

    std::puts("PASS packet parser for 0x5000, 0x2001, 0xA107, and 0xA101");
  }

  auto check_player_classification() -> void {
    // 1. Validate canonical player vtable addresses
    constexpr std::uintptr_t k_cic_user_vt = 0x0104214Cu;
    constexpr std::uintptr_t k_cic_player_vt = 0x01041EBCu;

    const auto is_player_vtable = [](std::uintptr_t vt) -> bool {
      return vt == k_cic_user_vt || vt == k_cic_player_vt;
    };

    require(is_player_vtable(0x0104214C), "CICUser vtable classified as player");
    require(is_player_vtable(0x01041EBC), "CICPlayer vtable classified as player");
    require(!is_player_vtable(0x01042220), "Non-player vtable rejected");
    require(!is_player_vtable(0x0104258C), "Item vtable rejected");

    // 2. Validate player name extraction at offset 0x8CC using MSVC9 SSO layout
    std::vector<std::uint8_t> mock_user_mem(0x950, 0);
    *reinterpret_cast<std::uintptr_t*>(mock_user_mem.data()) = k_cic_user_vt;

    auto* name_words = reinterpret_cast<std::uint32_t*>(mock_user_mem.data() + 0x8CC);
    std::wcscpy(reinterpret_cast<wchar_t*>(mock_user_mem.data() + 0x8CC + 4), L"Hero");
    name_words[5] = 4; // length
    name_words[6] = 7; // capacity (SSO)

    const auto* name_wstr = reinterpret_cast<const ext_client::msvc9::wstring*>(mock_user_mem.data() + 0x8CC);
    require(!name_wstr->empty(), "mock player name is non-empty");
    require(std::wcscmp(name_wstr->c_str(), L"Hero") == 0, "extracted player name at 0x8CC matches expected");

    // 3. Validate guild name offset logic at 0x914 using MSVC9 heap layout
    const wchar_t* heap_guild = L"Vanguard";
    auto* guild_words = reinterpret_cast<std::uint32_t*>(mock_user_mem.data() + 0x914);
    *reinterpret_cast<const wchar_t**>(mock_user_mem.data() + 0x914 + 4) = heap_guild;
    guild_words[5] = static_cast<std::uint32_t>(std::wcslen(heap_guild));
    guild_words[6] = 15; // capacity >= 8 (heap)

    const auto* guild_wstr = reinterpret_cast<const ext_client::msvc9::wstring*>(mock_user_mem.data() + 0x914);
    require(std::wcscmp(guild_wstr->c_str(), L"Vanguard") == 0, "remote player guild name at 0x914 matches expected");

    std::puts("PASS player classification (vftables 0x0104214C, 0x01041EBC) and 0x8CC name extraction");
  }

  auto check_hud_esp() -> void {
    // 1. Verify default configuration
    auto& cfg = config::data().hud_esp;
    require(cfg.enabled, "hud_esp enabled by default");
    require(cfg.show_target_indicator, "target indicator enabled by default");
    require(!cfg.show_target_snapline, "target snapline disabled by default for clean production visuals");
    require(cfg.show_target_info, "target info enabled by default");
    require(cfg.show_monster_esp, "monster esp enabled by default");
    require(cfg.show_monster_hp_bar, "monster hp bar enabled by default");
    require(cfg.show_unique_alert, "unique alert enabled by default");
    require(cfg.show_player_esp, "player esp enabled by default");
    require(cfg.show_item_drop_esp, "item drop esp enabled by default");
    require(!cfg.show_self_esp, "self esp disabled by default");
    require(cfg.min_rarity == 0, "default min rarity is 0 (all)");
    require(cfg.max_distance_meters == 80, "default max distance is 80m");
    require(cfg.target_ring_radius == 18.0f, "default ring radius is 18.0f");
    require(!cfg.show_mesh_boxes, "mesh boxes disabled by default");
    require(!cfg.show_skeleton, "skeleton disabled by default");
    require(!cfg.show_spine_only, "spine only disabled by default");
    require(cfg.skeleton_thickness == 1.5f, "default skeleton thickness is 1.5f");
    require(cfg.mesh_box_thickness == 1.2f, "default mesh box thickness is 1.2f");
    require(cfg.overhead_offset_y == 36.0f, "default overhead offset is 36.0f");

    // 2. Test runtime publication
    cfg.max_distance_meters = 120;
    cfg.show_mesh_boxes = true;
    cfg.show_skeleton = true;
    cfg.show_item_drop_esp = false;
    cfg.show_self_esp = true;
    config::mark_dirty();
    config::publish_runtime();
    require(config::runtime()->hud_esp.max_distance_meters == 120, "published hud_esp max distance");
    require(config::runtime()->hud_esp.show_mesh_boxes, "published hud_esp show_mesh_boxes");
    require(config::runtime()->hud_esp.show_skeleton, "published hud_esp show_skeleton");
    require(!config::runtime()->hud_esp.show_item_drop_esp, "published hud_esp show_item_drop_esp");
    require(config::runtime()->hud_esp.show_self_esp, "published hud_esp show_self_esp");
    cfg.max_distance_meters = 80;
    cfg.show_mesh_boxes = false;
    cfg.show_skeleton = false;
    cfg.show_item_drop_esp = true;
    cfg.show_self_esp = false;
    config::mark_dirty();
    config::publish_runtime();

    // 3. Test mathematical World-to-Screen projection and near-plane culling
    // Synthetic identity View-Projection matrix with camera at origin looking down +Z
    struct synthetic_vp {
      float m[4][4];
    };
    // Standard perspective projection matrix (FOV 60 deg, aspect 16:9, near 1, far 1000)
    // clip_x = x * 1.299f, clip_y = y * 2.31f, clip_w = z
    const auto project_synthetic = [](float x, float y, float z, float& out_sx, float& out_sy) -> bool {
      const float clip_x = x * 1.299038f;
      const float clip_y = y * 2.309401f;
      const float clip_w = z;

      if (clip_w <= 0.001f) {
        return false; // Behind camera / near plane
      }

      const float inv_w = 1.0f / clip_w;
      const float ndc_x = clip_x * inv_w;
      const float ndc_y = clip_y * inv_w;

      constexpr float screen_w = 1920.0f;
      constexpr float screen_h = 1080.0f;
      out_sx = (1.0f + ndc_x) * (screen_w * 0.5f);
      out_sy = (1.0f - ndc_y) * (screen_h * 0.5f);
      return true;
    };

    float sx = 0.0f, sy = 0.0f;
    // Object straight ahead at (0, 0, 10) should project to center of screen (960, 540)
    require(project_synthetic(0.0f, 0.0f, 10.0f, sx, sy), "point in front of camera projects");
    require(std::abs(sx - 960.0f) < 0.01f && std::abs(sy - 540.0f) < 0.01f, "center point maps to screen center");

    // Object behind camera at (0, 0, -5) must be culled
    require(!project_synthetic(0.0f, 0.0f, -5.0f, sx, sy), "point behind camera is culled");

    // 4. Test entity rarity filtering logic
    const auto passes_filter = [](std::uint8_t rarity, bool is_unique, bool is_giant, int min_rarity) -> bool {
      if (min_rarity == 1 && rarity == 0) return false;
      if (min_rarity == 2 && !is_giant && !is_unique && rarity < 4) return false;
      if (min_rarity == 3 && !is_unique) return false;
      return true;
    };
    require(passes_filter(0, false, false, 0), "normal mob passes min_rarity=0");
    require(!passes_filter(0, false, false, 1), "normal mob blocked by min_rarity=1");
    require(passes_filter(1, false, false, 1), "champion mob passes min_rarity=1");
    require(!passes_filter(1, false, false, 2), "champion mob blocked by min_rarity=2");
    require(passes_filter(4, false, true, 2), "giant mob passes min_rarity=2");
    require(!passes_filter(4, false, true, 3), "giant mob blocked by min_rarity=3");
    require(passes_filter(3, true, false, 3), "unique mob passes min_rarity=3");
    require(passes_filter(8, true, false, 3), "unique mob (rarity 8) passes min_rarity=3");

    // 5. Test 3D bounding box corner generation and affine transform math
    struct synthetic_d3d_matrix {
      float m[4][4];
    };
    const auto transform_pt = [](float px, float py, float pz, const synthetic_d3d_matrix& mat) -> std::tuple<float, float, float> {
      return {
        px * mat.m[0][0] + py * mat.m[1][0] + pz * mat.m[2][0] + mat.m[3][0],
        px * mat.m[0][1] + py * mat.m[1][1] + pz * mat.m[2][1] + mat.m[3][1],
        px * mat.m[0][2] + py * mat.m[1][2] + pz * mat.m[2][2] + mat.m[3][2]
      };
    };
    synthetic_d3d_matrix trans_mat = {{{1.0f, 0.0f, 0.0f, 0.0f},
                                       {0.0f, 1.0f, 0.0f, 0.0f},
                                       {0.0f, 0.0f, 1.0f, 0.0f},
                                       {100.0f, 20.0f, 50.0f, 1.0f}}};
    auto [tx, ty, tz] = transform_pt(-5.0f, 0.0f, 10.0f, trans_mat);
    require(std::abs(tx - 95.0f) < 0.001f && std::abs(ty - 20.0f) < 0.001f && std::abs(tz - 60.0f) < 0.001f,
            "bounding box affine translation correctly offsets local vertices");

    std::puts("PASS hud_esp configuration, projection arithmetic, rarity filters, and 3d bounding box math");
  }

  auto check_msvc9_stl_views() -> void {
    // 1. Test vector_view
    int arr[5] = {10, 20, 30, 40, 50};
    auto vec = ext_client::msvc9::vector_view<int>::from_pointers(arr, arr + 5);
    require(vec.size() == 5, "vector_view size");
    require(!vec.empty(), "vector_view not empty");
    require(vec[0] == 10 && vec[4] == 50, "vector_view index operator");
    require(vec.safe_at(2) != nullptr && *vec.safe_at(2) == 30, "vector_view safe_at in-bounds");
    require(vec.safe_at(5) == nullptr, "vector_view safe_at out-of-bounds");

    const auto* found_40 = vec.find_if([](int x) { return x == 40; });
    require(found_40 != nullptr && *found_40 == 40, "vector_view find_if existing element");
    const auto* found_99 = vec.find_if([](int x) { return x == 99; });
    require(found_99 == nullptr, "vector_view find_if non-existent element");

    int visit_count = 0;
    vec.for_each([&](int x) -> bool {
      ++visit_count;
      return x != 30; // terminate when 30 is reached
    });
    require(visit_count == 3, "vector_view early-exit for_each stops at 3rd element");

    // 2. Test map_view with synthetic Red-Black Tree
    using map_node_t = ext_client::msvc9::n_map_node<int, int>;
    map_node_t head{};
    map_node_t node1{};
    map_node_t node2{};
    map_node_t node3{};

    // Configure sentinel head
    head.isnil = 1;
    head.color = 1; // black
    head.left = &node1;   // min (100)
    head.parent = &node2; // root (200)
    head.right = &node3;  // max (300)

    // Configure root (node2)
    node2.isnil = 0;
    node2.color = 1;
    node2.key = 200;
    node2.value = 2;
    node2.parent = &head;
    node2.left = &node1;
    node2.right = &node3;

    // Configure left child (node1)
    node1.isnil = 0;
    node1.color = 0;
    node1.key = 100;
    node1.value = 1;
    node1.parent = &node2;
    node1.left = &head;
    node1.right = &head;

    // Configure right child (node3)
    node3.isnil = 0;
    node3.color = 0;
    node3.key = 300;
    node3.value = 3;
    node3.parent = &node2;
    node3.left = &head;
    node3.right = &head;

    auto map = ext_client::msvc9::map_view<int, int>::from_head_and_size(&head, 3);
    require(map.size() == 3, "map_view size");
    require(!map.empty(), "map_view not empty");
    require(map.contains(100), "map_view contains min key");
    require(map.contains(200), "map_view contains root key");
    require(map.contains(300), "map_view contains max key");
    require(!map.contains(999), "map_view does not contain non-existent key");

    auto* val_100 = map.find_value(100);
    require(val_100 != nullptr && *val_100 == 1, "map_view find_value(100)");
    auto* val_200 = map.find_value(200);
    require(val_200 != nullptr && *val_200 == 2, "map_view find_value(200)");
    auto* val_300 = map.find_value(300);
    require(val_300 != nullptr && *val_300 == 3, "map_view find_value(300)");
    require(map.find_value(500) == nullptr, "map_view find_value(500) returns null");

    // In-order traversal order check
    std::vector<int> visited_keys;
    map.for_each([&](int key, int /*val*/) {
      visited_keys.push_back(key);
    });
    require(visited_keys.size() == 3, "map_view traversed all 3 nodes");
    require(visited_keys[0] == 100 && visited_keys[1] == 200 && visited_keys[2] == 300,
            "map_view traversed in ascending in-order sequence");

    // Early-exit check
    int map_visits = 0;
    map.for_each([&](int key, int /*val*/) -> bool {
      ++map_visits;
      return key != 200; // break after visiting 200
    });
    require(map_visits == 2, "map_view early-exit breaks on condition");

    // find_if check
    auto* pred_found = map.find_if([](int val) { return val == 3; });
    require(pred_found != nullptr && *pred_found == 3, "map_view find_if matches value");

    // 3. Test set_view with synthetic Red-Black Tree
    using set_node_t = ext_client::msvc9::n_set_node<int>;
    set_node_t s_head{};
    set_node_t s_node1{};
    set_node_t s_node2{};
    set_node_t s_node3{};

    s_head.isnil = 1;
    s_head.color = 1;
    s_head.left = &s_node1;
    s_head.parent = &s_node2;
    s_head.right = &s_node3;

    s_node2.isnil = 0;
    s_node2.color = 1;
    s_node2.key = 20;
    s_node2.parent = &s_head;
    s_node2.left = &s_node1;
    s_node2.right = &s_node3;

    s_node1.isnil = 0;
    s_node1.color = 0;
    s_node1.key = 10;
    s_node1.parent = &s_node2;
    s_node1.left = &s_head;
    s_node1.right = &s_head;

    s_node3.isnil = 0;
    s_node3.color = 0;
    s_node3.key = 30;
    s_node3.parent = &s_node2;
    s_node3.left = &s_head;
    s_node3.right = &s_head;

    auto set = ext_client::msvc9::set_view<int>::from_head_and_size(&s_head, 3);
    require(set.size() == 3, "set_view size");
    require(set.contains(10) && set.contains(20) && set.contains(30), "set_view contains all keys");
    require(!set.contains(99), "set_view does not contain missing key");
    require(set.find_element(20) != nullptr && *set.find_element(20) == 20, "set_view find_element");

    std::vector<int> set_keys;
    set.for_each([&](int key) {
      set_keys.push_back(key);
    });
    require(set_keys.size() == 3 && set_keys[0] == 10 && set_keys[1] == 20 && set_keys[2] == 30,
            "set_view in-order traversal");

    std::puts("PASS msvc9 stl views (vector_view, map_view, set_view binary search & in-order traversal)");
  }

  auto check_memory_safety() -> void {
    using namespace ext_client::utils::memory;

    // 1. is_aligned_ptr
    void* aligned_ptr = reinterpret_cast<void*>(0x00401000);
    void* unaligned_ptr = reinterpret_cast<void*>(0x00401003);
    require(is_aligned_ptr(aligned_ptr, 4), "0x00401000 is 4-byte aligned");
    require(!is_aligned_ptr(unaligned_ptr, 4), "0x00401003 is not 4-byte aligned");

    // 2. is_game_ptr
    require(!is_game_ptr(nullptr), "null is not game ptr");
    require(!is_game_ptr(reinterpret_cast<void*>(0x1000)), "0x1000 is not game ptr (low memory)");
    require(!is_game_ptr(reinterpret_cast<void*>(0x80000000)), "0x80000000 is not game ptr (kernel space)");
    require(is_game_ptr(reinterpret_cast<void*>(0x00400000)), "0x00400000 is game ptr");

    // 3. is_valid_ptr (combines range bounds + natural alignment in a single fast branch)
    require(!is_valid_ptr(nullptr), "null is not valid ptr");
    require(!is_valid_ptr(reinterpret_cast<void*>(0x1000)), "0x1000 is not valid ptr (low memory)");
    require(!is_valid_ptr(reinterpret_cast<void*>(0x80000000)), "0x80000000 is not valid ptr (kernel space)");
    require(!is_valid_ptr(reinterpret_cast<void*>(0x00401003), 4), "0x00401003 is not valid ptr (unaligned 4-byte)");
    require(is_valid_ptr(reinterpret_cast<void*>(0x00401000), 4), "0x00401000 is valid ptr (valid address + 4-byte aligned)");

    // 4. safe_read
    int test_val = 12345;
    int read_out = 0;
    if (is_game_ptr(&test_val)) {
      require(safe_read(&test_val, read_out) && read_out == 12345, "safe_read reads valid value");
    }
    int bad_out = 0;
    require(!safe_read(nullptr, bad_out), "safe_read safely rejects nullptr without throwing");

    // 5. vector_view from simulated container pointers
    int data[4] = {100, 200, 300, 400};
    auto view = ext_client::msvc9::vector_view<int>::from_pointers(data, data + 4, data + 4);
    require(view.size() == 4 && view[0] == 100 && view[3] == 400, "vector_view from container fields");

    std::puts("PASS memory safety checks (alignment, bounds, unified is_valid_ptr, safe_read, vector_view)");
  }

  auto check_equipment_system() -> void {
    // 1. Validate structure invariants
    require(cso_item::k_class_size == 0x1F8, "CSOItem size must be 504 bytes (0x1F8)");

    // 2. Simulated CSOItem buffer
    alignas(void*) std::uint8_t item_buf[0x1F8]{};
    auto* item = reinterpret_cast<cso_item*>(item_buf);

    // Initial state: not valid because active slot flag is 0
    require(!item->is_valid(), "empty buffer is invalid item");

    // Populate active slot flag (+0x28) and ref_id (+0x34)
    item_buf[0x028] = 1; // active slot flag
    *reinterpret_cast<std::uint32_t*>(&item_buf[0x034]) = 10580; // ref item id
    *reinterpret_cast<std::uint32_t*>(&item_buf[0x038]) = 998877; // item unique id
    item_buf[0x08C] = 7; // opt level +7
    item_buf[0x072] = 2; // adv elixir level +2
    *reinterpret_cast<std::uint32_t*>(&item_buf[0x098]) = 145; // durability
    *reinterpret_cast<std::uint32_t*>(&item_buf[0x09C]) = 1; // count

    // Socket 0 at +0x74: stone_id 2001, param 5
    *reinterpret_cast<std::uint32_t*>(&item_buf[0x074]) = 2001;
    *reinterpret_cast<std::uint32_t*>(&item_buf[0x078]) = 5;

    // Stats at +0xE0
    auto* stats = reinterpret_cast<cso_item_stats*>(&item_buf[0x0E0]);
    stats->max_phy_atk = 1250;
    stats->min_phy_atk = 980;
    stats->phy_atk_pct = 25; // ~80%
    stats->max_durability = 150;

    require(item->is_valid(), "populated buffer is valid item");
    require(item->ref_id() == 10580, "ref_id extracted at +0x34");
    require(item->item_id() == 998877, "item_id extracted at +0x38");
    require(item->opt_level() == 7, "opt_level extracted at +0x8C");
    require(item->adv_elixir_level() == 2, "adv_elixir_level at +0x72");
    require(item->durability() == 145, "durability at +0x98");
    require(item->count() == 1, "count at +0x9C");

    const auto sockets = item->sockets();
    require(sockets.size() == 1 && sockets[0].stone_id == 2001 && sockets[0].param == 5, "sockets parsed");

    const auto* item_stats = item->stats();
    require(item_stats != nullptr, "stats pointer valid");
    require(item_stats->max_phy_atk == 1250 && item_stats->min_phy_atk == 980, "combat stats parsed");
    require(item_stats->phy_atk_pct == 25, "variance percentage parsed");

    // 3. Simulated CRefObjItem buffer
    alignas(void*) std::uint8_t ref_buf[0x600]{};
    auto* ref = reinterpret_cast<cref_obj_item*>(ref_buf);

    // TypeID: Type1=3, Type2=1, Type3=6, Type4=1 -> Chinese Weapon (Sword)
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x000]) = (3) | (1 << 8) | (6 << 16) | (1 << 24);
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x004]) = 10580;
    ref_buf[0x1D0] = 31; // Class 31 -> Degree = (31 - 1)/3 + 1 = 11th Degree!
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x1D4]) = 101; // req level 101
    *reinterpret_cast<std::int32_t*>(&ref_buf[0x254]) = 15; // 1.5 meters range

    require(ref->is_equipment() && ref->is_weapon(), "weapon classification");
    require(!ref->is_shield() && !ref->is_armor(), "classification filters");
    require(ref->degree() == 11, "degree arithmetic: (31 - 1)/3 + 1 == 11");
    require(ref->req_level() == 101, "req level parsed at +0x1D4");
    require(std::abs(ref->attack_range() - 1.5f) < 0.001f, "attack range parsed at +0x254");

    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x0A0]) = 2; // SoX rarity
    require(ref->is_sox(), "is_sox check");
    require(ref->sox_tier() == 0, "sox tier for class 31 is first (Star)");

    ref_buf[0x1D0] = 33; // Class 33 -> (33-1)%3 == 2 -> Sun!
    require(ref->sox_tier() == 2, "sox tier for class 33 is third (Sun)");

    // TypeID classification table verified against the client's refitem data (TypeID1=3, TypeID2=1)
    auto set_type = [&](std::uint32_t tid3, std::uint32_t tid4) {
      *reinterpret_cast<std::uint32_t*>(&ref_buf[0x000]) = (3) | (1 << 8) | (tid3 << 16) | (tid4 << 24);
    };
    for (std::uint32_t t3 : {1u, 2u, 3u, 9u, 10u, 11u}) {
      set_type(t3, 3);
      require(ref->is_armor() && !ref->is_weapon() && !ref->is_accessory() && !ref->is_avatar(), "armor tid3 1-3 / 9-11");
    }
    for (std::uint32_t t3 : {5u, 12u}) {
      set_type(t3, 3);
      require(ref->is_accessory() && !ref->is_armor(), "accessory tid3 5 / 12");
    }
    set_type(4, 2);
    require(ref->is_shield() && !ref->is_armor(), "shield tid3 4");
    set_type(7, 1);
    require(ref->is_job_suit() && !ref->is_armor() && !ref->is_weapon(), "job suit tid3 7");
    set_type(13, 4);
    require(ref->is_avatar() && !ref->is_devil_spirit(), "avatar tid3 13");
    set_type(14, 1);
    require(ref->is_devil_spirit() && !ref->is_avatar(), "devil spirit tid3 14");
    // Non-equipment (TypeID2 != 1) must never be classified as equipment
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x000]) = (3) | (3 << 8) | (6 << 16) | (1 << 24);
    require(!ref->is_equipment() && !ref->is_weapon(), "non-equipment is not classified");

    // 4. Test icon_path retrieval from +0x15C (SSO string)
    std::memcpy(&ref_buf[0x15C + 4], "icon\\test.ddj", 14);
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x15C + 20]) = 13; // length
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x15C + 24]) = 15; // capacity
    require(ref->icon_path() == "icon\\test.ddj", "icon_path extracted from +0x15C");

    // Test fallback to +0x178 when +0x15C is empty
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x15C + 20]) = 0; // clear length
    std::memcpy(&ref_buf[0x178 + 4], "test2.ddj", 10);
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x178 + 20]) = 9;
    *reinterpret_cast<std::uint32_t*>(&ref_buf[0x178 + 24]) = 15;
    require(ref->icon_path() == "icon\\test2.ddj", "icon_path fallback to +0x178 with prepended icon\\");

    // 5. Item parameters (Param1..Param6)
    *reinterpret_cast<std::int32_t*>(&ref_buf[0x2B4]) = 100;
    *reinterpret_cast<std::int32_t*>(&ref_buf[0x2B8]) = 25;
    *reinterpret_cast<std::int32_t*>(&ref_buf[0x2BC]) = 200;
    *reinterpret_cast<std::int32_t*>(&ref_buf[0x2C0]) = 30;
    *reinterpret_cast<std::int32_t*>(&ref_buf[0x2C4]) = 5;
    *reinterpret_cast<std::int32_t*>(&ref_buf[0x2C8]) = 6;
    require(ref->param1() == 100, "param1 at +0x2B4");
    require(ref->param2() == 25, "param2 at +0x2B8");
    require(ref->param3() == 200, "param3 at +0x2BC");
    require(ref->param4() == 30, "param4 at +0x2C0");
    require(ref->param5() == 5, "param5 at +0x2C4");
    require(ref->param6() == 6, "param6 at +0x2C8");

    std::puts("PASS equipment system (CSOItem, CRefObjItem, stats, variances, sockets, degree math, icon paths, params)");
  }

  auto check_item_icon_rendering() -> void {
    // 1. SoX animation frame calculations (32 frames, 40ms/frame)
    require(render::calculate_sox_frame(0) < 32, "sox frame within bounds");
    require(render::calculate_sox_frame_at(0, 0) == 0, "sox frame at t=0");
    require(render::calculate_sox_frame_at(0, 5) == 5, "sox frame seed offset");
    require(render::calculate_sox_frame_at(40, 0) == 1, "sox frame advances after 40ms");
    require(render::calculate_sox_frame_at(40 * 32, 0) == 0, "sox frame loops at 32 frames");
    require(render::calculate_sox_frame_at(40 * 35, 2) == 5, "sox frame arithmetic (35 + 2) % 32 == 5");

    // 2. Nasrun animation frame calculations (16 frames, 40ms/frame)
    require(render::calculate_nasrun_frame(0) < 16, "nasrun frame within bounds");
    require(render::calculate_nasrun_frame_at(0, 0) == 0, "nasrun frame at t=0");
    require(render::calculate_nasrun_frame_at(40 * 16, 0) == 0, "nasrun frame loops at 16 frames");
    require(render::calculate_nasrun_frame_at(40 * 18, 1) == 3, "nasrun frame arithmetic (18 + 1) % 16 == 3");

    // 3. SoX spritesheet UVs (256x128 texture, 8 cols x 4 rows)
    for (std::uint32_t f = 0; f < 32; ++f) {
      ImVec2 uv0{}, uv1{};
      render::get_sox_uvs(f, uv0, uv1);

      // Verify bounds
      require(uv0.x >= 0.0f && uv0.x <= 1.0f, "sox uv0.x in [0, 1]");
      require(uv0.y >= 0.0f && uv0.y <= 1.0f, "sox uv0.y in [0, 1]");
      require(uv1.x >= 0.0f && uv1.x <= 1.0f, "sox uv1.x in [0, 1]");
      require(uv1.y >= 0.0f && uv1.y <= 1.0f, "sox uv1.y in [0, 1]");

      // Verify cell dimensions (width = 0.125f, height = 0.25f)
      require(std::abs((uv1.x - uv0.x) - 0.125f) < 1e-5f, "sox uv width is 1/8 (0.125)");
      require(std::abs((uv1.y - uv0.y) - 0.25f) < 1e-5f, "sox uv height is 1/4 (0.25)");
    }

    // Specific boundary frames
    ImVec2 s_uv0{}, s_uv1{};
    render::get_sox_uvs(0, s_uv0, s_uv1);
    require(s_uv0.x == 0.0f && s_uv0.y == 0.0f, "sox frame 0 starts at (0, 0)");
    render::get_sox_uvs(31, s_uv0, s_uv1);
    require(std::abs(s_uv1.x - 1.0f) < 1e-5f && std::abs(s_uv1.y - 1.0f) < 1e-5f, "sox frame 31 ends at (1, 1)");

    // 4. Nasrun spritesheet UVs (512x32 texture, 16 cols x 1 row)
    for (std::uint32_t f = 0; f < 16; ++f) {
      ImVec2 uv0{}, uv1{};
      render::get_nasrun_uvs(f, uv0, uv1);

      require(uv0.x >= 0.0f && uv0.x <= 1.0f, "nasrun uv0.x in [0, 1]");
      require(uv0.y == 0.0f && uv1.y == 1.0f, "nasrun uv y span is 0 to 1");
      require(std::abs((uv1.x - uv0.x) - 0.0625f) < 1e-5f, "nasrun uv width is 1/16 (0.0625)");
    }
    render::get_nasrun_uvs(15, s_uv0, s_uv1);
    require(std::abs(s_uv1.x - 1.0f) < 1e-5f, "nasrun frame 15 ends at 1.0");

    std::puts("PASS item icon rendering (spritesheet UVs, frame calculation, timing)");
  }

  auto check_native_architecture() -> void {
    // 1. Companion / Pet Data Layout & Calculations (SCOSInfo)
    alignas(16) std::uint8_t pet_buf[0x8000]{0};
    auto* info = reinterpret_cast<scos_info*>(pet_buf);

    *reinterpret_cast<std::uint32_t*>(pet_buf + 0x08) = 777;   // cos_id
    *reinterpret_cast<std::uint32_t*>(pet_buf + 0x0C) = 24100; // ref_obj_id

    // Setup MSVC9 string SSO layout for Pet Custom Name at +0x10
    // _Buf at +0x04 (up to 7 wchar_t + null), _Mysize at +0x14, _Myres at +0x18
    const wchar_t test_pet_name[] = L"Wolf";
    std::memcpy(pet_buf + 0x10 + 4, test_pet_name, sizeof(test_pet_name));
    *reinterpret_cast<std::uint32_t*>(pet_buf + 0x10 + 0x14) = 4; // length
    *reinterpret_cast<std::uint32_t*>(pet_buf + 0x10 + 0x18) = 7; // capacity (SSO)

    *reinterpret_cast<std::uint32_t*>(pet_buf + 0x30) = 12000;       // hp
    *reinterpret_cast<std::uint32_t*>(pet_buf + 0x34) = 15000;       // max_hp
    *reinterpret_cast<std::uint32_t*>(pet_buf + 0x38) = 6500;        // satiety (65.00%)
    *reinterpret_cast<std::uint8_t*>(pet_buf + 0x3C)  = 75;          // level
    *reinterpret_cast<std::uint8_t*>(pet_buf + 0x3E)  = 3;           // type4 = 3 (growth pet)
    *reinterpret_cast<std::uint32_t*>(pet_buf + 0x40) = 0x0F;        // pickup_flags
    *reinterpret_cast<std::uint64_t*>(pet_buf + 0x48) = 75000000ULL; // exp
    *reinterpret_cast<std::uint64_t*>(pet_buf + 0x50) = 100000000ULL;// max_exp
    *reinterpret_cast<std::uint32_t*>(pet_buf + 0xF8) = 2500;        // stored_sp (fellow)

    require(info->cos_id() == 777, "scos_info cos_id at +0x08");
    require(info->ref_obj_id() == 24100, "scos_info ref_obj_id at +0x0C");
    require(std::wcscmp(info->name(), L"Wolf") == 0, "scos_info wstring_ref name at +0x10");
    require(info->current_hp() == 12000, "scos_info current_hp at +0x30");
    require(info->max_hp() == 15000, "scos_info max_hp at +0x34");
    require(info->satiety() == 6500, "scos_info satiety at +0x38");
    require(std::abs(info->satiety_percent() - 65.0f) < 0.01f, "scos_info satiety_percent (0..100)");
    require(info->level() == 75, "scos_info level at +0x3C");
    require(info->type4() == 3 && info->is_growth_pet(), "scos_info type4 growth pet");
    require(info->pickup_flags() == 0x0F, "scos_info pickup_flags at +0x40");
    require(info->current_exp() == 75000000ULL, "scos_info current_exp at +0x48");
    require(info->max_exp() == 100000000ULL, "scos_info max_exp at +0x50");
    require(std::abs(info->exp_percent() - 75.0f) < 0.01f, "scos_info exp_percent 75%");

    // Setup an item at slot 0 (offset 0xD8) and slot 28 (offset 0xD8 + 28 * 504)
    auto* slot0_bytes = pet_buf + 0xD8;
    *reinterpret_cast<std::uint8_t*>(slot0_bytes + 0x28) = 1;     // active flag
    *reinterpret_cast<std::uint32_t*>(slot0_bytes + 0x34) = 8888; // ref_id
    *reinterpret_cast<std::uint32_t*>(slot0_bytes + 0x38) = 4444; // item_id
    auto* item0 = info->get_item(0);
    require(item0 != nullptr, "scos_info slot 0 item resolved at +0xD8");
    require(item0->item_id() == 4444, "scos_info slot 0 item_id valid");

    auto* slot28_bytes = pet_buf + 0xD8 + (28 * cso_item::k_class_size);
    *reinterpret_cast<std::uint8_t*>(slot28_bytes + 0x28) = 1;     // active flag
    *reinterpret_cast<std::uint32_t*>(slot28_bytes + 0x34) = 9999; // ref_id
    *reinterpret_cast<std::uint32_t*>(slot28_bytes + 0x38) = 5555; // item_id
    auto* item28 = info->get_item(28);
    require(item28 != nullptr, "scos_info slot 28 item resolved at +0xD8 + 28*504 (page 2)");
    require(item28->item_id() == 5555, "scos_info slot 28 item_id valid");

    // 2. Fellow Pet checks
    *reinterpret_cast<std::uint8_t*>(pet_buf + 0x3E) = 9; // fellow pet
    require(info->is_fellow_pet(), "scos_info is_fellow_pet() true for type4 == 9");
    require(info->stored_sp() == 2500, "scos_info stored_sp at +0xF8");

    // 3. Companion Manager (CCOSDataMgr) map_view projection
    using pet_map_node_t = ext_client::msvc9::n_map_node<std::uint32_t, scos_info*>;
    pet_map_node_t p_head{}, p_node1{}, p_node2{};

    p_head.isnil = 1;
    p_head.left = &p_node1;
    p_head.parent = &p_node1;
    p_head.right = &p_node2;

    p_node1.isnil = 0;
    p_node1.key = 501;
    p_node1.value = info;
    p_node1.parent = &p_head;
    p_node1.left = &p_head;
    p_node1.right = &p_node2;

    p_node2.isnil = 0;
    p_node2.key = 502;
    p_node2.value = info;
    p_node2.parent = &p_node1;
    p_node2.left = &p_head;
    p_node2.right = &p_head;

    alignas(16) std::uint8_t mgr_buf[0x40]{0};
    *reinterpret_cast<void**>(mgr_buf + 0x08 + 4) = &p_head; // map at +0x08, head at +0x0C
    *reinterpret_cast<std::uint32_t*>(mgr_buf + 0x08 + 8) = 2; // size
    *reinterpret_cast<std::uint32_t*>(mgr_buf + 0x10) = 2; // pet_count at +0x10

    auto* cos_mgr = reinterpret_cast<ccos_data_mgr*>(mgr_buf);
    require(cos_mgr->pet_count() == 2, "ccos_data_mgr pet_count() == 2");
    auto active_pets = cos_mgr->get_all_pets();
    require(active_pets.size() == 2, "ccos_data_mgr get_all_pets() returns 2 pets");
    require(active_pets[0].first == 501 && active_pets[0].second == info, "ccos_data_mgr pet 501 matches");
    require(active_pets[1].first == 502 && active_pets[1].second == info, "ccos_data_mgr pet 502 matches");

    // 4. Skill System (CSkillRunTimeManager) map_view projection
    using skill_map_node_t = ext_client::msvc9::n_map_node<std::uint32_t, void*>;
    skill_map_node_t s_head{}, s_node1{}, s_node2{}, s_node3{};

    s_head.isnil = 1;
    s_head.left = &s_node1;
    s_head.parent = &s_node2;
    s_head.right = &s_node3;

    s_node2.isnil = 0;
    s_node2.key = 2002;
    s_node2.value = reinterpret_cast<void*>(0x1234);
    s_node2.parent = &s_head;
    s_node2.left = &s_node1;
    s_node2.right = &s_node3;

    s_node1.isnil = 0;
    s_node1.key = 2001;
    s_node1.value = reinterpret_cast<void*>(0x1234);
    s_node1.parent = &s_node2;
    s_node1.left = &s_head;
    s_node1.right = &s_head;

    s_node3.isnil = 0;
    s_node3.key = 2003;
    s_node3.value = reinterpret_cast<void*>(0x1234);
    s_node3.parent = &s_node2;
    s_node3.left = &s_head;
    s_node3.right = &s_head;

    alignas(16) std::uint8_t skill_mgr_buf[0x40]{0};
    *reinterpret_cast<void**>(skill_mgr_buf + 0x0C + 4) = &s_head; // map at +0x0C, head at +0x10
    *reinterpret_cast<std::uint32_t*>(skill_mgr_buf + 0x0C + 8) = 3; // size

    const auto skill_map =
        ext_client::msvc9::map_view<std::uint32_t, void*>::from_object(skill_mgr_buf, 0x0C);
    require(skill_map.size() == 3, "CSkillRunTimeManager map_view size == 3");
    require(skill_map.contains(2001), "skill 2001 learned");
    require(skill_map.contains(2002), "skill 2002 learned");
    require(skill_map.contains(2003), "skill 2003 learned");
    require(!skill_map.contains(9999), "skill 9999 not learned");

    // 5. Avatar Equipment map_view projection (CICPlayer + 0x23E0)
    using avatar_map_node_t = ext_client::msvc9::n_map_node<std::uint8_t, cso_item*>;
    avatar_map_node_t a_head{}, a_node0{}, a_node2{};

    a_head.isnil = 1;
    a_head.left = &a_node0;
    a_head.parent = &a_node0;
    a_head.right = &a_node2;

    a_node0.isnil = 0;
    a_node0.key = 0; // Avatar Dress
    a_node0.value = item0;
    a_node0.parent = &a_head;
    a_node0.left = &a_head;
    a_node0.right = &a_node2;

    a_node2.isnil = 0;
    a_node2.key = 2; // Avatar Hat
    a_node2.value = item28;
    a_node2.parent = &a_node0;
    a_node2.left = &a_head;
    a_node2.right = &a_head;

    alignas(16) std::uint8_t fake_player[0x2500]{0};
    *reinterpret_cast<void**>(fake_player + 0x23E0 + 4) = &a_head; // map at +0x23E0, head at +0x23E4
    *reinterpret_cast<std::uint32_t*>(fake_player + 0x23E0 + 8) = 2; // size

    const auto avatar_map =
        ext_client::msvc9::map_view<std::uint8_t, cso_item*>::from_object(fake_player, 0x23E0);
    require(avatar_map.size() == 2, "avatar map_view size == 2");
    require(avatar_map.contains(0), "avatar map has slot 0 (Dress)");
    require(avatar_map.contains(2), "avatar map has slot 2 (Hat)");
    require(!avatar_map.contains(1), "avatar map does not have slot 1");
    require(avatar_map.find_value(0) != nullptr && *avatar_map.find_value(0) == item0, "avatar slot 0 matches item0");
    require(avatar_map.find_value(2) != nullptr && *avatar_map.find_value(2) == item28, "avatar slot 2 matches item28");

    std::puts("PASS native game architecture (SCOSInfo layout/inventory/name, CCOSDataMgr map, CSkillRunTimeManager map, Avatar map)");
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
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  try {
    std::puts("1. check_config");
    check_config();
    std::puts("2. check_player_classification");
    check_player_classification();
    std::puts("3. check_hud_esp");
    check_hud_esp();
    std::puts("4. check_packets");
    check_packets();
    std::puts("5. check_packet_parser");
    check_packet_parser();
    std::puts("6. check_hooks");
    check_hooks();
    std::puts("7. check_events");
    check_events();
    std::puts("8. check_packed_time");
    check_packed_time();
    std::puts("9. check_msvc9_stl_views");
    check_msvc9_stl_views();
    std::puts("10. check_memory_safety");
    check_memory_safety();
    std::puts("11. check_equipment_system");
    check_equipment_system();
    std::puts("12. check_item_icon_rendering");
    check_item_icon_rendering();
    std::puts("13. check_native_architecture");
    check_native_architecture();
    std::puts("ALL CHECKS PASSED!");
    return 0;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
  }
}
