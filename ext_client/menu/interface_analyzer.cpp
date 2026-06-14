#include "menu/interface_analyzer.hpp"

#include "config/client_config.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cif_manager.hpp"
#include "sdk/cps_character_select.hpp"
#include "sdk/cps_title.hpp"
#include "sdk/cgwnd.hpp"
#include "utils/process.hpp"
#include "utils/process_manager.hpp"
#include "utils/string.hpp"

#include <imgui.h>

#include <Windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <iomanip>

namespace ext_client::menu::interface_analyzer {
  namespace {

    enum class root_mode : int {
      auto_detect = 0,
      active_process,
      cg_interface,
      cps_title,
      character_select,
      ingame_res_map,
      iface_res_map,
    };

    struct root_ref {
      cgwnd* wnd = nullptr;
      const char* label = "none";
    };

    struct class_summary {
      char type_name[64]{};
      std::size_t count = 0;
    };

    struct snapshot_stats {
      std::size_t total = 0;
      std::size_t visible = 0;
      std::size_t hidden = 0;
      std::size_t static_count = 0;
      std::size_t textured = 0;
      std::size_t res_known = 0;
      std::size_t res_named = 0;
      std::size_t inferred_res = 0;
      std::size_t missing_res = 0;
      std::size_t duplicate_ids = 0;
    };

    struct analyzer_state {
      root_mode root = root_mode::auto_detect;
      bool auto_refresh = false;
      bool visible_only = false;
      bool unresolved_only = false;
      bool selected_only = false;
      int max_depth = 18;
      int probe_max_id = 256;
      char filter[128]{};
      char status[260] = "No snapshot yet.";
      cgwnd* selected = nullptr;
      std::vector<cif_manager::widget_info> rows;
      std::vector<class_summary> classes;
      snapshot_stats stats;
    };

    auto state() -> analyzer_state& {
      static analyzer_state s;
      return s;
    }

    auto append_root(std::vector<root_ref>& roots, cgwnd* wnd, const char* label) -> void {
      if (!wnd || !cif_manager::is_live_widget(wnd)) {
        return;
      }
      for (const auto& existing : roots) {
        if (existing.wnd == wnd) {
          return;
        }
      }
      roots.push_back({wnd, label});
    }

    auto resolve_roots(root_mode mode) -> std::vector<root_ref> {
      std::vector<root_ref> roots;

      auto add_active_process = [&]() {
        append_root(roots, cif_manager::browsable_ui_root(ext_client::utils::process_manager::resolved_active_child()), "active process");
      };

      switch (mode) {
        case root_mode::auto_detect:
          if (cg_interface::is_ready()) {
            append_root(roots, reinterpret_cast<cgwnd*>(cg_interface::get()), "CGInterface");
          }
          if (auto* title = cps_title::current()) {
            append_root(roots, reinterpret_cast<cgwnd*>(title), "CPSTitle");
          }
          if (auto* character_select = cps_character_select::current()) {
            append_root(roots, reinterpret_cast<cgwnd*>(character_select), "CPSCharacterSelect");
          }
          add_active_process();
          break;
        case root_mode::active_process:
          add_active_process();
          break;
        case root_mode::cg_interface:
          if (cg_interface::is_ready()) {
            append_root(roots, reinterpret_cast<cgwnd*>(cg_interface::get()), "CGInterface");
          }
          break;
        case root_mode::cps_title:
          if (auto* title = cps_title::current()) {
            append_root(roots, reinterpret_cast<cgwnd*>(title), "CPSTitle");
          }
          break;
        case root_mode::character_select:
          if (auto* character_select = cps_character_select::current()) {
            append_root(roots, reinterpret_cast<cgwnd*>(character_select), "CPSCharacterSelect");
          }
          break;
        case root_mode::ingame_res_map:
          for (const auto& entry : cif_manager::enumerate_ingame_res_map(512)) {
            append_root(roots, entry.wnd, "IngameResMap");
          }
          break;
        case root_mode::iface_res_map:
          for (const auto& entry : cif_manager::enumerate_iface_res_map(512)) {
            append_root(roots, entry.wnd, "CGInterface res map");
          }
          break;
        default:
          break;
      }

      return roots;
    }

    auto lower_ascii(char c) -> char {
      return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    }

    auto contains_case_insensitive(const char* haystack, const char* needle) -> bool {
      if (!needle || needle[0] == '\0') {
        return true;
      }
      if (!haystack) {
        return false;
      }

      const std::size_t needle_len = std::strlen(needle);
      if (needle_len == 0) {
        return true;
      }

      for (const char* h = haystack; *h != '\0'; ++h) {
        std::size_t i = 0;
        while (i < needle_len && h[i] != '\0' && lower_ascii(h[i]) == lower_ascii(needle[i])) {
          ++i;
        }
        if (i == needle_len) {
          return true;
        }
      }
      return false;
    }

    auto row_matches_filter(const cif_manager::widget_info& row) -> bool {
      const auto& s = state();
      if (s.visible_only && !row.visible) {
        return false;
      }
      if (s.unresolved_only && row.lookup_res_key >= 0) {
        return false;
      }
      if (s.selected_only && row.widget != s.selected) {
        return false;
      }
      if (s.filter[0] == '\0') {
        return true;
      }

      std::string text_utf8 = ::ext_client::utils::string::to_utf8(row.text);

      char haystack[1024]{};
      std::snprintf(haystack,
                    sizeof(haystack),
                    "%p %s %s %s %d %d %d %s",
                    row.widget,
                    row.type_name,
                    row.res_name,
                    row.ddj_path,
                    row.control_id,
                    row.lookup_res_key,
                    cif_manager::unique_id(row.widget),
                    text_utf8.c_str());
      return contains_case_insensitive(haystack, s.filter);
    }

    auto compute_stats() -> void {
      auto& s = state();
      s.stats = snapshot_stats{};
      s.stats.total = s.rows.size();

      std::unordered_map<std::string, std::size_t> class_counts;
      std::unordered_map<int, std::size_t> runtime_ids;

      for (const auto& row : s.rows) {
        if (row.visible) {
          ++s.stats.visible;
        } else {
          ++s.stats.hidden;
        }
        if (cif_manager::is_static(row.widget)) {
          ++s.stats.static_count;
        }
        if (row.ddj_path[0] != '\0') {
          ++s.stats.textured;
        }
        if (row.lookup_res_key >= 0) {
          ++s.stats.res_known;
        } else {
          ++s.stats.missing_res;
        }
        if (row.res_name[0] != '\0') {
          ++s.stats.res_named;
        }
        if (row.lookup_res_inferred) {
          ++s.stats.inferred_res;
        }
        if (row.control_id > 0) {
          ++runtime_ids[row.control_id];
        }
        ++class_counts[row.type_name[0] ? row.type_name : "(unknown)"];
      }

      for (const auto& item : runtime_ids) {
        if (item.second > 1) {
          ++s.stats.duplicate_ids;
        }
      }

      s.classes.clear();
      s.classes.reserve(class_counts.size());
      for (const auto& item : class_counts) {
        class_summary summary{};
        std::strncpy(summary.type_name, item.first.c_str(), sizeof(summary.type_name) - 1);
        summary.count = item.second;
        s.classes.push_back(summary);
      }
      std::sort(s.classes.begin(), s.classes.end(), [](const class_summary& a, const class_summary& b) {
        if (a.count != b.count) {
          return a.count > b.count;
        }
        return std::strcmp(a.type_name, b.type_name) < 0;
      });
    }

    auto capture_snapshot() -> void {
      auto& s = state();
      s.rows.clear();

      const auto roots = resolve_roots(s.root);
      if (roots.empty()) {
        std::snprintf(s.status, sizeof(s.status), "No readable interface roots for selected mode.");
        compute_stats();
        return;
      }

      std::unordered_set<cgwnd*> seen;
      for (const auto& root : roots) {
        auto chunk = cif_manager::walk(root.wnd, s.max_depth, s.probe_max_id);
        for (auto& row : chunk) {
          if (!row.widget || !seen.insert(row.widget).second) {
            continue;
          }
          s.rows.push_back(row);
        }
      }

      compute_stats();
      std::snprintf(s.status, sizeof(s.status), "Captured %zu widgets from %zu root(s).", s.rows.size(), roots.size());
    }

    auto csv_put_escaped(std::ostream& file, std::string_view text) -> void {
      file << '"';
      for (const char c : text) {
        if (c == '"') {
          file << '"';
        }
        file << c;
      }
      file << '"';
    }

    auto export_snapshot() -> void {
      auto& s = state();
      if (s.rows.empty()) {
        std::snprintf(s.status, sizeof(s.status), "Nothing to export. Capture a snapshot first.");
        return;
      }

      char dir[MAX_PATH]{};
      if (!ext_client::utils::process::exe_directory(dir, sizeof(dir))) {
        std::strncpy(dir, ".\\", sizeof(dir) - 1);
      }

      SYSTEMTIME now{};
      GetLocalTime(&now);

      char path[MAX_PATH]{};
      std::snprintf(path,
                    sizeof(path),
                    "%ssro_interface_snapshot_%04u%02u%02u_%02u%02u%02u.csv",
                    dir,
                    now.wYear,
                    now.wMonth,
                    now.wDay,
                    now.wHour,
                    now.wMinute,
                    now.wSecond);

      std::ofstream file(path);
      if (!file.is_open()) {
        std::snprintf(s.status, sizeof(s.status), "Export failed: %s", path);
        return;
      }

      file << "address,vftable,class,kind,depth,control_id,unique_id,visible,x,y,w,h,res_key,res_inferred,res_name,ddj,text,parent\n";

      for (const auto& row : s.rows) {
        std::string text_utf8 = ::ext_client::utils::string::to_utf8(row.text);

        file << row.widget << ",0x" << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << row.vftable << std::dec
             << std::nouppercase << ",";
        csv_put_escaped(file, row.type_name);
        file << "," << static_cast<int>(row.kind) << "," << row.depth << "," << row.control_id << "," << cif_manager::unique_id(row.widget)
             << "," << (row.visible ? 1 : 0) << "," << row.rect_x << "," << row.rect_y << "," << row.rect_w << "," << row.rect_h << ","
             << row.lookup_res_key << "," << (row.lookup_res_inferred ? 1 : 0) << ",";
        csv_put_escaped(file, row.res_name);
        file << ',';
        csv_put_escaped(file, row.ddj_path);
        file << ',';
        csv_put_escaped(file, text_utf8);
        file << "," << (row.widget ? row.widget->parent() : nullptr) << "\n";
      }

      std::snprintf(s.status, sizeof(s.status), "Exported %zu rows to %s", s.rows.size(), path);
    }

    auto draw_metric(const char* label, std::size_t value) -> void {
      ImGui::TextDisabled("%s", label);
      ImGui::SameLine(130.0f);
      ImGui::Text("%zu", value);
    }

    auto draw_toolbar() -> void {
      auto& s = state();

      int root = static_cast<int>(s.root);
      ImGui::SetNextItemWidth(190.0f);
      if (ImGui::Combo("root##ia",
                       &root,
                       "Auto\0Active process\0CGInterface\0CPSTitle\0Character select\0Ingame res map\0IFace res map\0")) {
        s.root = static_cast<root_mode>(root);
      }
      ImGui::SameLine();
      if (ImGui::Button("Capture##ia")) {
        capture_snapshot();
      }
      ImGui::SameLine();
      if (ImGui::Button("Export CSV##ia")) {
        export_snapshot();
      }
      ImGui::SameLine();
      ImGui::Checkbox("auto##ia", &s.auto_refresh);

      ImGui::SetNextItemWidth(100.0f);
      if (ImGui::InputInt("depth##ia", &s.max_depth, 1, 4)) {
        if (s.max_depth < 1) {
          s.max_depth = 1;
        } else if (s.max_depth > 64) {
          s.max_depth = 64;
        }
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100.0f);
      if (ImGui::InputInt("probe ids##ia", &s.probe_max_id, 16, 64)) {
        if (s.probe_max_id < 0) {
          s.probe_max_id = 0;
        } else if (s.probe_max_id > 2048) {
          s.probe_max_id = 2048;
        }
      }

      ImGui::SetNextItemWidth(-1.0f);
      ImGui::InputTextWithHint("##ia_filter", "Filter class, GDR name, texture, text, id, or address", s.filter, sizeof(s.filter));

      ImGui::Checkbox("visible only##ia", &s.visible_only);
      ImGui::SameLine();
      ImGui::Checkbox("unresolved res only##ia", &s.unresolved_only);
      ImGui::SameLine();
      ImGui::Checkbox("selected only##ia", &s.selected_only);

      ImGui::TextDisabled("%s", s.status);
    }

    auto draw_summary() -> void {
      auto& s = state();
      if (ImGui::BeginChild("ia_summary", ImVec2(0.0f, 118.0f), ImGuiChildFlags_Border)) {
        if (ImGui::BeginTable("ia_summary_cols", 3, ImGuiTableFlags_SizingStretchProp)) {
          ImGui::TableSetupColumn("a");
          ImGui::TableSetupColumn("b");
          ImGui::TableSetupColumn("c");
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          draw_metric("widgets", s.stats.total);
          draw_metric("visible", s.stats.visible);
          draw_metric("hidden", s.stats.hidden);

          ImGui::TableSetColumnIndex(1);
          draw_metric("static", s.stats.static_count);
          draw_metric("textured", s.stats.textured);
          draw_metric("duplicate ids", s.stats.duplicate_ids);

          ImGui::TableSetColumnIndex(2);
          draw_metric("res resolved", s.stats.res_known);
          draw_metric("res named", s.stats.res_named);
          draw_metric("res inferred", s.stats.inferred_res);
          draw_metric("res missing", s.stats.missing_res);

          ImGui::EndTable();
        }
      }
      ImGui::EndChild();
    }

    auto draw_class_summary() -> void {
      auto& s = state();
      if (!ImGui::CollapsingHeader("Class summary##ia")) {
        return;
      }

      if (ImGui::BeginTable("ia_classes", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("class");
        ImGui::TableSetupColumn("count", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();
        for (const auto& item : s.classes) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextUnformatted(item.type_name);
          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%zu", item.count);
        }
        ImGui::EndTable();
      }
    }

    auto draw_rows() -> void {
      auto& s = state();
      std::vector<int> visible_rows;
      visible_rows.reserve(s.rows.size());
      for (std::size_t i = 0; i < s.rows.size(); ++i) {
        if (row_matches_filter(s.rows[i])) {
          visible_rows.push_back(static_cast<int>(i));
        }
      }

      ImGui::TextDisabled("%d shown", static_cast<int>(visible_rows.size()));

      const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable |
                                    ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
      if (!ImGui::BeginTable("ia_rows", 9, flags, ImVec2(0.0f, 0.0f))) {
        return;
      }

      ImGui::TableSetupScrollFreeze(0, 1);
      ImGui::TableSetupColumn("addr", ImGuiTableColumnFlags_WidthFixed, 86.0f);
      ImGui::TableSetupColumn("class", ImGuiTableColumnFlags_WidthFixed, 136.0f);
      ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, 44.0f);
      ImGui::TableSetupColumn("uid", ImGuiTableColumnFlags_WidthFixed, 54.0f);
      ImGui::TableSetupColumn("rect", ImGuiTableColumnFlags_WidthFixed, 118.0f);
      ImGui::TableSetupColumn("res", ImGuiTableColumnFlags_WidthFixed, 56.0f);
      ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 145.0f);
      ImGui::TableSetupColumn("texture");
      ImGui::TableSetupColumn("text");
      ImGui::TableHeadersRow();

      ImGuiListClipper clipper;
      clipper.Begin(static_cast<int>(visible_rows.size()));
      while (clipper.Step()) {
        for (int row_i = clipper.DisplayStart; row_i < clipper.DisplayEnd; ++row_i) {
          const auto& row = s.rows[static_cast<std::size_t>(visible_rows[static_cast<std::size_t>(row_i)])];

          std::string text_utf8 = ::ext_client::utils::string::to_utf8(row.text);

          ImGui::PushID(row.widget);
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          const bool selected = s.selected == row.widget;
          char addr[32]{};
          std::snprintf(addr, sizeof(addr), "%p", row.widget);
          if (ImGui::Selectable(addr, selected, ImGuiSelectableFlags_SpanAllColumns)) {
            s.selected = row.widget;
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("vftable=0x%08X parent=%p kind=%d depth=%d",
                              row.vftable,
                              row.widget ? row.widget->parent() : nullptr,
                              static_cast<int>(row.kind),
                              row.depth);
          }

          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(row.type_name);
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%d", row.control_id);
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%d", cif_manager::unique_id(row.widget));
          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%d,%d %dx%d", row.rect_x, row.rect_y, row.rect_w, row.rect_h);
          ImGui::TableSetColumnIndex(5);
          if (row.lookup_res_key >= 0) {
            ImGui::Text("%d%s", row.lookup_res_key, row.lookup_res_inferred ? "*" : "");
          } else {
            ImGui::TextDisabled("-");
          }
          ImGui::TableSetColumnIndex(6);
          ImGui::TextUnformatted(row.res_name[0] ? row.res_name : "-");
          ImGui::TableSetColumnIndex(7);
          ImGui::TextUnformatted(row.ddj_path[0] ? row.ddj_path : "-");
          ImGui::TableSetColumnIndex(8);
          ImGui::TextUnformatted(!text_utf8.empty() ? text_utf8.c_str() : "-");
          ImGui::PopID();
        }
      }

      ImGui::EndTable();
    }

  } // namespace

  auto draw() -> void {
    auto& s = state();
    if (s.auto_refresh) {
      capture_snapshot();
    }

    draw_toolbar();
    ImGui::Spacing();
    draw_summary();
    draw_class_summary();
    ImGui::Spacing();
    draw_rows();
  }

} // namespace ext_client::menu::interface_analyzer
