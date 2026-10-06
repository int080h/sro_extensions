#include "pch.hpp"
#include "render/item_tooltip_renderer.hpp"
#include "render/item_icon_renderer.hpp"
#include "sdk/ui/cui_string_manager.hpp"
#include "utils/string.hpp"

#include <imgui.h>
#include <cstdio>
#include <cmath>

namespace ext_client::render {

  namespace {

    inline auto loc(const wchar_t* key) -> std::wstring {
      return ext_client::sdk::ui::get_string(key);
    }

    inline auto to_u8(const std::wstring& w) -> std::string {
      return w.empty() ? std::string{} : ext_client::utils::string::to_utf8(w.c_str());
    }

    inline auto argb_to_imvec4(std::uint32_t argb) -> ImVec4 {
      return ImVec4(static_cast<float>((argb >> 16) & 0xFF) / 255.0f,
                    static_cast<float>((argb >> 8) & 0xFF) / 255.0f,
                    static_cast<float>(argb & 0xFF) / 255.0f,
                    static_cast<float>((argb >> 24) & 0xFF) / 255.0f);
    }

    struct markup_span {
      std::string text;
      ImVec4 color;
      bool bold{false};
    };

    inline auto parse_markup_color(const std::string& val, ImVec4 default_col) -> ImVec4 {
      std::string s = val;
      while (!s.empty() && (s.front() == ' ' || s.front() == '"' || s.front() == '\'')) s.erase(s.begin());
      while (!s.empty() && (s.back() == ' ' || s.back() == '"' || s.back() == '\'')) s.pop_back();

      if (!s.empty() && s[0] == '#') {
        unsigned int hex_val = 0;
        if (std::sscanf(s.c_str() + 1, "%x", &hex_val) == 1) {
          if (s.size() <= 7) {
            return ImVec4(((hex_val >> 16) & 0xFF) / 255.0f,
                          ((hex_val >> 8) & 0xFF) / 255.0f,
                          (hex_val & 0xFF) / 255.0f,
                          1.0f);
          } else {
            return ImVec4(((hex_val >> 16) & 0xFF) / 255.0f,
                          ((hex_val >> 8) & 0xFF) / 255.0f,
                          (hex_val & 0xFF) / 255.0f,
                          std::max(0.85f, ((hex_val >> 24) & 0xFF) / 255.0f));
          }
        }
      }

      int r = 255, g = 255, b = 255, a = 255;
      int n = std::sscanf(s.c_str(), "%d, %d, %d, %d", &r, &g, &b, &a);
      if (n >= 3) {
        float fa = (n == 4) ? std::max(0.85f, static_cast<float>(a) / 255.0f) : 1.0f;
        return ImVec4(static_cast<float>(r) / 255.0f,
                      static_cast<float>(g) / 255.0f,
                      static_cast<float>(b) / 255.0f,
                      fa);
      }
      return default_col;
    }

    inline auto replace_str(std::string& str, const std::string& from, const std::string& to) -> void {
      if (from.empty()) return;
      size_t start_pos = 0;
      while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
      }
    }

    auto render_markup_text(const std::string& raw_markup, float wrap_width) -> void {
      if (raw_markup.empty()) return;

      std::string text = raw_markup;

      // Decode HTML entities
      replace_str(text, "&lt;", "<");
      replace_str(text, "&gt;", ">");
      replace_str(text, "&amp;", "&");
      replace_str(text, "&quot;", "\"");
      replace_str(text, "&apos;", "'");
      replace_str(text, "&nbsp;", " ");

      // Strip outer container tags if present
      auto strip_tag_pair = [](std::string& s, const std::string& open_tag, const std::string& close_tag) {
        size_t o = s.find(open_tag);
        if (o != std::string::npos) s.erase(o, open_tag.length());
        size_t c = s.rfind(close_tag);
        if (c != std::string::npos) s.erase(c, close_tag.length());
      };
      strip_tag_pair(text, "<sml2>", "</sml2>");
      strip_tag_pair(text, "<sml>", "</sml>");
      strip_tag_pair(text, "<xml2>", "</xml2>");
      strip_tag_pair(text, "<xml>", "</xml>");
      strip_tag_pair(text, "<?xml", "?>");

      const ImVec4 default_text_col(0.85f, 0.85f, 0.88f, 1.0f);
      const ImVec4 bold_highlight_col(1.0f, 0.92f, 0.70f, 1.0f);
      std::vector<ImVec4> color_stack;
      bool is_bold = false;

      // Split into lines by <br>, <br/>, or \n
      std::vector<std::vector<markup_span>> lines;
      lines.emplace_back();

      size_t i = 0;
      while (i < text.size()) {
        if (text[i] == '<') {
          size_t close_pos = text.find('>', i);
          size_t next_open = text.find('<', i + 1);

          // If no closing '>', or another '<' occurs before '>', treat as literal text
          if (close_pos == std::string::npos || (next_open != std::string::npos && next_open < close_pos)) {
            size_t take = (next_open != std::string::npos) ? (next_open - i) : 1;
            std::string rem = text.substr(i, take);
            ImVec4 col = !color_stack.empty() ? color_stack.back() : (is_bold ? bold_highlight_col : default_text_col);
            lines.back().push_back({rem, col, is_bold});
            i += take;
            continue;
          }

          std::string tag = text.substr(i + 1, close_pos - i - 1);
          std::string tag_lower = tag;
          std::transform(tag_lower.begin(), tag_lower.end(), tag_lower.begin(),
                         [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

          while (!tag_lower.empty() && tag_lower.front() == ' ') tag_lower.erase(tag_lower.begin());
          while (!tag_lower.empty() && tag_lower.back() == ' ') tag_lower.pop_back();

          if (tag_lower == "br" || tag_lower == "br/" || tag_lower == "br /") {
            lines.emplace_back();
          } else if (tag_lower.rfind("font", 0) == 0) {
            size_t cpos = tag.find("color=");
            if (cpos != std::string::npos) {
              std::string col_str = tag.substr(cpos + 6);
              if (!col_str.empty() && (col_str[0] == '"' || col_str[0] == '\'')) {
                char q = col_str[0];
                size_t qend = col_str.find(q, 1);
                if (qend != std::string::npos) {
                  col_str = col_str.substr(1, qend - 1);
                }
              }
              color_stack.push_back(parse_markup_color(col_str, default_text_col));
            }
          } else if (tag_lower == "/font") {
            if (!color_stack.empty()) color_stack.pop_back();
          } else if (tag_lower == "strong" || tag_lower == "b") {
            is_bold = true;
          } else if (tag_lower == "/strong" || tag_lower == "/b") {
            is_bold = false;
          } else {
            // Ignored container markup: sml, sml2, xml, p, div, span, etc.
          }

          i = close_pos + 1;
        } else if (text[i] == '\r' || text[i] == '\n') {
          if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
            i += 2;
          } else {
            i += 1;
          }
          lines.emplace_back();
        } else {
          size_t next_ctrl = text.find_first_of("<\r\n", i);
          if (next_ctrl == std::string::npos) next_ctrl = text.size();

          std::string chunk = text.substr(i, next_ctrl - i);
          ImVec4 col = !color_stack.empty() ? color_stack.back() : (is_bold ? bold_highlight_col : default_text_col);
          lines.back().push_back({chunk, col, is_bold});
          i = next_ctrl;
        }
      }

      // Render lines with word wrapping
      const float space_width = ImGui::CalcTextSize(" ").x;

      for (const auto& line_spans : lines) {
        if (line_spans.empty()) {
          ImGui::Spacing();
          continue;
        }

        float current_x = 0.0f;
        bool is_first_word = true;

        for (const auto& span : line_spans) {
          if (span.text.empty()) continue;

          size_t pos = 0;
          while (pos < span.text.size()) {
            while (pos < span.text.size() && span.text[pos] == ' ') {
              pos++;
            }
            if (pos >= span.text.size()) break;

            size_t end = span.text.find(' ', pos);
            if (end == std::string::npos) end = span.text.size();
            std::string word = span.text.substr(pos, end - pos);
            pos = end;

            const ImVec2 word_sz = ImGui::CalcTextSize(word.c_str());

            if (!is_first_word) {
              if (current_x + space_width + word_sz.x > wrap_width) {
                ImGui::NewLine();
                current_x = 0.0f;
                is_first_word = true;
              } else {
                ImGui::SameLine(0.0f, space_width);
                current_x += space_width;
              }
            }

            ImGui::PushStyleColor(ImGuiCol_Text, span.color);
            ImGui::TextUnformatted(word.c_str());
            ImGui::PopStyleColor();

            current_x += word_sz.x;
            is_first_word = false;
          }
        }
      }
    }

  } // namespace

  auto render_item_tooltip_from_data(const sdk::game::item_tooltip_data& d) -> void {
    // Exact Silkroad native tooltip style: compact padding, sharp rounding, subtle dark navy backdrop
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(11, 16, 25, 230));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(45, 65, 95, 215));

    ImGui::BeginTooltip();

    // 1. Title Header & Opt Level (sub_75DFE0 / sub_74D1C0)
    const bool is_sox = !d.subtitle.empty() || d.is_sox;
    const ImVec4 title_col = is_sox ? ImVec4(0.42f, 0.60f, 0.95f, 1.0f) :
                            (d.opt_level > 0 ? ImVec4(1.0f, 0.85f, 0.25f, 1.0f) : ImVec4(0.96f, 0.96f, 0.98f, 1.0f));

    if (d.opt_level > 0) {
      ImGui::TextColored(title_col, "%s (+%u)", d.title.c_str(), d.opt_level);
    } else {
      ImGui::TextColored(title_col, "%s", d.title.c_str());
    }

    if (!d.subtitle.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", d.subtitle.c_str());
    }

    // 2. Classification, Position & Degree (Exact localized client strings from sub_768B10 / sub_7577E0)
    if (!d.slot_name.empty()) {
      const auto s_type_lbl = loc(L"UIIT_STT_ITEM_TYPE");
      const auto u8_type = to_u8(s_type_lbl);
      if (!u8_type.empty()) {
        ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s: %s", u8_type.c_str(), d.slot_name.c_str());
      } else {
        ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s", d.slot_name.c_str());
      }
    }
    if (!d.armor_position_name.empty()) {
      const auto s_pos_lbl = loc(L"UIIT_STT_ARMOR_POSITION");
      const auto u8_pos = to_u8(s_pos_lbl);
      if (!u8_pos.empty()) {
        ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s: %s", u8_pos.c_str(), d.armor_position_name.c_str());
      } else {
        ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s", d.armor_position_name.c_str());
      }
    }
    if (d.degree > 0) {
      const auto deg_fmt = loc(L"UIIT_TOOLTIP_EQUIPMENT_CLASS");
      if (!deg_fmt.empty()) {
        wchar_t deg_buf[128]{};
        swprintf_s(deg_buf, deg_fmt.c_str(), d.degree);
        ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 1.0f), "%s", ext_client::utils::string::to_utf8(deg_buf).c_str());
      }
    }

    ImGui::Spacing();

    // 2B. Description (sub_74CE80) - with full HTML/XML tag rendering (colors, bold, <br>, entities)
    if (!d.description.empty()) {
      render_markup_text(d.description, ImGui::GetFontSize() * 26.0f);
      ImGui::Spacing();
    }

    // 2C. Pet / Fellow Summon Information (sub_76A9D0 / sub_76A250)
    if (d.is_pet_item) {
      const auto s_pet_info = loc(L"UIIT_STT_COSNEWUI_TOOLTIP_PETINFO");
      const auto u8_pet_info = to_u8(s_pet_info);
      ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s",
                         !u8_pet_info.empty() ? u8_pet_info.c_str() : "Pet information");

      if (!d.pet_type_name.empty()) {
        const auto s_type_lbl = loc(L"UIIT_CTL_PARTYMATCH_PSEARCH_LIST_TYPE");
        const auto u8_type_lbl = to_u8(s_type_lbl);
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s: %s",
                           !u8_type_lbl.empty() ? u8_type_lbl.c_str() : "Type", d.pet_type_name.c_str());
      }
      if (!d.pet_name.empty()) {
        const auto s_name_lbl = loc(L"UIIT_STT_COSNEWUI_TOOLTIP_PETNAME");
        const auto u8_name_lbl = to_u8(s_name_lbl);
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s: %s",
                           !u8_name_lbl.empty() ? u8_name_lbl.c_str() : "Pet Name", d.pet_name.c_str());
      }
      if (d.pet_level > 0) {
        const auto s_lvl_lbl = loc(L"UIIT_STT_COSNEWUI_TOOLTIP_PETLEVEL");
        const auto u8_lvl_lbl = to_u8(s_lvl_lbl);
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s: %u",
                           !u8_lvl_lbl.empty() ? u8_lvl_lbl.c_str() : "Pet level", d.pet_level);
      }
      if (!d.pet_condition.empty()) {
        const auto s_cond_lbl = loc(L"UIIT_STT_COSNEWUI_TOOLTIP_PETSTATE");
        const auto u8_cond_lbl = to_u8(s_cond_lbl);
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s: %s",
                           !u8_cond_lbl.empty() ? u8_cond_lbl.c_str() : "Pet condition", d.pet_condition.c_str());
      }
      ImGui::Spacing();
    }

    // 2D. Consumable & Scroll Effect Lines (sub_769640 / sub_75BA30)
    if (!d.effect_lines.empty()) {
      for (const auto& eff : d.effect_lines) {
        if (!eff.text.empty()) {
          ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s", eff.text.c_str());
        }
      }
      ImGui::Spacing();
    }

    // 2E. Avatar attachment wear state / magic option slot count (sub_768770 / sub_74A0A0)
    if (!d.avatar_wear_text.empty()) {
      ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s", d.avatar_wear_text.c_str());
    }
    if (!d.avatar_magic_slots_text.empty()) {
      ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s", d.avatar_magic_slots_text.c_str());
    }

    // 3. Combat & Defense Stats (Equipment only: sub_7494B0 / sub_76C0E0)
    if (d.is_weapon || d.is_armor || d.is_shield || d.is_accessory) {
      if (d.max_phy_atk > 0) {
        const int pct = static_cast<int>((static_cast<float>(d.phy_atk_pct) / 31.0f) * 100.0f);
        const auto s_pa = to_u8(loc(L"PARAM_PA"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %u ~ %u (+%d%%)", s_pa.c_str(), d.min_phy_atk, d.max_phy_atk, pct);
      }
      if (d.max_mag_atk > 0) {
        const int pct = static_cast<int>((static_cast<float>(d.mag_atk_pct) / 31.0f) * 100.0f);
        const auto s_ma = to_u8(loc(L"PARAM_MA"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %u ~ %u (+%d%%)", s_ma.c_str(), d.min_mag_atk, d.max_mag_atk, pct);
      }
      if (d.phy_def > 0.0f) {
        const int pct = static_cast<int>((static_cast<float>(d.phy_def_pct) / 31.0f) * 100.0f);
        const auto s_pd = to_u8(loc(L"PARAM_PD"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %.1f (+%d%%)", s_pd.c_str(), d.phy_def, pct);
      }
      if (d.mag_def > 0.0f) {
        const int pct = static_cast<int>((static_cast<float>(d.mag_def_pct) / 31.0f) * 100.0f);
        const auto s_md = to_u8(loc(L"PARAM_MD"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %.1f (+%d%%)", s_md.c_str(), d.mag_def, pct);
      }
      if (d.max_durability > 0) {
        const int pct = static_cast<int>((static_cast<float>(d.durability_pct) / 31.0f) * 100.0f);
        const auto s_dur = to_u8(loc(L"PARAM_DUR"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %u/%u (+%d%%)", s_dur.c_str(), d.current_durability, d.max_durability, pct);
      }
      if (d.attack_range > 0.0f) {
        const auto s_range = to_u8(loc(L"PARAM_RANGE"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %.1f m", s_range.c_str(), d.attack_range);
      }
      if (d.hit_rate > 0) {
        const int pct = static_cast<int>((static_cast<float>(d.hit_pct) / 31.0f) * 100.0f);
        const auto s_hr = to_u8(loc(L"PARAM_HR"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %u (+%d%%)", s_hr.c_str(), d.hit_rate, pct);
      }
      if (d.parry_rate > 0) {
        const int pct = static_cast<int>((static_cast<float>(d.parry_pct) / 31.0f) * 100.0f);
        const auto s_er = to_u8(loc(L"PARAM_ER"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %u (+%d%%)", s_er.c_str(), d.parry_rate, pct);
      }
      if (d.critical > 0) {
        const int pct = static_cast<int>((static_cast<float>(d.critical_pct) / 31.0f) * 100.0f);
        const auto s_crit = to_u8(loc(L"PARAM_CRITICAL"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %u (+%d%%)", s_crit.c_str(), d.critical, pct);
      }
      if (d.blocking_rate > 0) {
        const int pct = static_cast<int>((static_cast<float>(d.blocking_pct) / 31.0f) * 100.0f);
        const auto s_block = to_u8(loc(L"PARAM_BLOCKING"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %u (+%d%%)", s_block.c_str(), d.blocking_rate, pct);
      }
      if (d.max_phy_reinforce > 0.0f || d.min_phy_reinforce > 0.0f) {
        const int pct = static_cast<int>((static_cast<float>(d.phy_reinforce_pct) / 31.0f) * 100.0f);
        const auto s_pr = to_u8(loc(L"PARAM_PHYSICAL_SPECIALIZE"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %.1f %% ~ %.1f %% (+%d%%)", s_pr.c_str(), d.min_phy_reinforce * 100.0f, d.max_phy_reinforce * 100.0f, pct);
      }
      if (d.max_mag_reinforce > 0.0f || d.min_mag_reinforce > 0.0f) {
        const int pct = static_cast<int>((static_cast<float>(d.mag_reinforce_pct) / 31.0f) * 100.0f);
        const auto s_mr = to_u8(loc(L"PARAM_MAGICAL_SPECIALIZE"));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %.1f %% ~ %.1f %% (+%d%%)", s_mr.c_str(), d.min_mag_reinforce * 100.0f, d.max_mag_reinforce * 100.0f, pct);
      }
    }

    // 4. Requirements & Race & Advanced Elixir status (sub_74D5F0 / sub_766B90)
    const bool has_adv_elixir_line = (d.is_weapon || d.is_armor || d.is_shield || d.is_accessory) && d.degree >= 1 && d.degree <= 11;
    const bool has_reqs = (!d.requirement_lines.empty() || has_adv_elixir_line);
    if (has_reqs) {
      ImGui::Spacing();
      for (const auto& line : d.requirement_lines) {
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s", line.c_str());
      }
      if (has_adv_elixir_line) {
        if (d.adv_elixir_level > 0) {
          const auto fmt = loc(L"UIIT_STT_SOCKET_TIP_UPPER_REINFOREC_USED");
          if (!fmt.empty()) {
            wchar_t elix_buf[128]{};
            swprintf_s(elix_buf, fmt.c_str(), d.adv_elixir_level);
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "%s", ext_client::utils::string::to_utf8(elix_buf).c_str());
          }
        } else {
          const auto unuse = to_u8(loc(L"UIIT_STT_SOCKET_TIP_UPPER_REINFOREC_UNUSE"));
          if (!unuse.empty()) {
            ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "%s", unuse.c_str());
          }
        }
      }
    }

    // 5. Sockets
    if (!d.sockets.empty()) {
      ImGui::Separator();
      const auto s_sock = to_u8(loc(L"UIIT_STT_SOCKET_INFO"));
      if (!s_sock.empty()) {
        ImGui::TextColored(ImVec4(0.85f, 0.65f, 1.0f, 1.0f), "%s", s_sock.c_str());
      }
      for (const auto& s : d.sockets) {
        ImGui::BulletText("%s", s.c_str());
      }
    }

    // 5B. Devil's Spirit option sections (sub_75E9E0)
    for (const auto& section : d.devil_sections) {
      if (section.lines.empty()) continue;
      ImGui::Spacing();
      if (!section.heading.empty()) {
        ImGui::TextColored(argb_to_imvec4(section.heading_argb), "%s", section.heading.c_str());
      }
      for (const auto& line : section.lines) {
        ImGui::TextColored(argb_to_imvec4(line.argb), "%s", line.text.c_str());
      }
    }

    // 6. Magic Options (Blues) - client colour 0xFF00EAFF
    if (!d.blues.empty()) {
      if (d.blues_heading.empty()) {
        ImGui::Separator();
      } else {
        ImGui::Spacing();
        ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s", d.blues_heading.c_str());
      }
      for (const auto& blue : d.blues) {
        ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::magic), "%s", blue.c_str());
      }
    }

    // 7. Devil's Spirit awaken period
    if (!d.awaken_header.empty() || !d.awaken_text.empty()) {
      ImGui::Spacing();
      if (!d.awaken_header.empty()) {
        ImGui::TextColored(argb_to_imvec4(sdk::game::tooltip_argb::heading), "%s", d.awaken_header.c_str());
      }
      if (!d.awaken_text.empty()) {
        ImGui::TextUnformatted(d.awaken_text.c_str());
      }
    }

    // 8. Quantity at bottom for all items that have quantity (sub_760250)
    // Matches native game layout 1:1: "Quantity <count>" (e.g. Quantity 1910)
    if (d.stack_count > 0 && !d.is_weapon && !d.is_armor && !d.is_shield && !d.is_accessory && !d.is_avatar && !d.is_devil_spirit) {
      const auto q_lbl = loc(L"UIIT_STT_AMOUNT");
      const auto u8_q = to_u8(q_lbl);
      const std::string label = !u8_q.empty() ? u8_q : "Quantity";
      ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "%s %u", label.c_str(), d.stack_count);
    }

    ImGui::EndTooltip();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
  }

  auto render_item_tooltip(const cso_item* item) -> void {
    if (!item || !item->is_valid()) return;
    const auto data = sdk::game::extract_tooltip_data(item);
    render_item_tooltip_from_data(data);
  }

} // namespace ext_client::render
