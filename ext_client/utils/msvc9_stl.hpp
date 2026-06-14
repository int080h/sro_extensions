#pragma once

// VC++ 2005 (MSVC 8.0 / _MSC_VER 1400) STL layouts used by Silkroad Online.
//
// ext_client is built with VS2022 — never reinterpret game memory as std::wstring,
// std::string, std::map, or std::vector from this DLL.
//
// Use:
//   * *_ref     — non-owning view of objects in game memory (read / limited write via game fn)
//   * owned types — storage in project space with the correct VS2005/MSVC8 layout; safe to pass &obj to game APIs

#include <cstddef>
#include <cstdint>

namespace ext_client::msvc9 {

  inline constexpr std::size_t wstring_object_size = 28;     // basic_string<wchar_t>
  inline constexpr std::size_t string_object_size = 28;      // basic_string<char> (includes 4-byte allocator)
  inline constexpr std::size_t ui_res_map_size = 48;         // CResIDManager / res map @ CPS+0xB0
  inline constexpr std::size_t child_list_node_size = 12;    // sub_409090 / sub_D6BC70 insert node
  inline constexpr std::size_t child_list_sentinel_size = 8; // sub_864310 empty head node
  inline constexpr std::size_t list_node_size = 12;          // std::list node (_Next, _Prev, _Myval)
  inline constexpr std::size_t vector_object_size = 16;      // vector<T> header (includes 4-byte allocator)
  inline constexpr std::size_t stdext_hash_map_size = 40;    // sub_924100 / operator new(0x28)

  // CResIDManager (sub_9D0190): sentinel @ +4, size @ +8.
  inline constexpr std::size_t res_map_sentinel_offset = 4;
  inline constexpr std::size_t res_map_size_offset = 8;

  inline constexpr std::uint32_t wstring_sso_capacity = 7; // _Myres < 8  => SSO
  inline constexpr std::uint32_t string_sso_capacity = 15; // _Myres < 16 => SSO

  // Plausible 32-bit user-mode pointer (filters null, low stub, and kernel range).
  inline auto is_game_ptr(const void* ptr) -> bool {
    if (!ptr) {
      return false;
    }
    const auto addr = reinterpret_cast<std::uintptr_t>(ptr);
    return addr >= 0x10000 && addr <= 0x7FFE0000;
  }

  // True when [ptr, ptr+bytes) lies in committed, readable virtual memory.
  auto is_readable_ptr(const void* ptr, std::size_t bytes) -> bool;

  // Fault-safe read for game heap pointers (VirtualQuery alone misses freed slots).
  auto try_read_u32(const void* ptr, std::uint32_t* out) -> bool;

  // Game heap allocation and free (VS2005/MSVC8 layout compatible)
  auto game_heap_alloc(std::size_t bytes) -> void*;
  auto game_heap_free(void* block, std::size_t bytes) -> void;

  // ---------------------------------------------------------------------------
  // Read-only views (game memory)
  // ---------------------------------------------------------------------------

  class wstring_ref {
  public:
    static auto from(const void* object) -> wstring_ref;

    auto object() const -> const void* { return object_; }
    auto capacity() const -> std::uint32_t;
    auto length() const -> std::uint32_t;
    auto data() const -> const wchar_t*;
    auto empty() const -> bool;
    auto copy_to(wchar_t* dst, std::size_t dst_count) const -> bool;

  private:
    const void* object_ = nullptr;
  };

  class string_ref {
  public:
    static auto from(const void* object) -> string_ref;

    auto object() const -> const void* { return object_; }
    auto capacity() const -> std::uint32_t;
    auto length() const -> std::uint32_t;
    auto data() const -> const char*;
    auto empty() const -> bool;
    auto copy_to(char* dst, std::size_t dst_count) const -> bool;

  private:
    const void* object_ = nullptr;
  };

  // MSVC 2008 std::map tree node (sub_4F8230 lower_bound): key @ +0x0C, value @ +0x10, _Isnil @ +0x15.
  struct map_tree_node {
    map_tree_node* left;
    map_tree_node* parent;
    map_tree_node* right;
    std::uint32_t key;
    void* value;

    auto is_nil() const -> bool;
  };

  inline constexpr std::size_t map_tree_node_size = 22;

  class map_ref {
  public:
    static auto from(const void* object) -> map_ref;

    auto object() const -> const void* { return object_; }
    auto size() const -> std::uint32_t;
    auto find(std::uint32_t key) const -> void*;

    template<typename Fn> auto for_each(Fn&& fn) const -> void {
      const auto* end = sentinel();
      if (!end) {
        return;
      }
      for (const auto* node = minimum(); node && node != end; node = next(node, end)) {
        if (!is_readable_ptr(node, map_tree_node_size)) {
          break;
        }
        if (node->is_nil()) {
          continue;
        }
        fn(node->key, node->value);
      }
    }

    static auto next(const map_tree_node* node, const map_tree_node* end) -> const map_tree_node*;

  private:
    const void* object_ = nullptr;

    auto sentinel() const -> const map_tree_node*;
    auto minimum() const -> const map_tree_node*;
  };

  // CGWnd::m_child_list (+0x7C) -> circular list head (sub_864310 / sub_D6BC70 / sub_D6C560).
  struct child_list_node {
    child_list_node* prev; // +0
    child_list_node* next; // +4
    void* widget;          // +8 (CGWnd*), absent on 8-byte sentinel head
  };

  class child_list_ref {
  public:
    // sentinel is the pointer stored in CGWnd::m_child_list (+0x7C).
    static auto from_sentinel(const void* sentinel) -> child_list_ref;

    auto sentinel() const -> const child_list_node* { return sentinel_; }

    template<typename Fn> auto for_each(Fn&& fn) const -> void {
      const auto* head = sentinel_;
      if (!head || !is_readable_ptr(head, child_list_sentinel_size)) {
        return;
      }

      const auto* node = head->next;
      for (int guard = 0; node && node != head && guard < 4096; ++guard) {
        if (!is_readable_ptr(node, child_list_node_size)) {
          break;
        }
        if (node->widget) {
          fn(0u, node->widget);
        }
        node = node->next;
      }
    }

  private:
    const child_list_node* sentinel_ = nullptr;
  };

  // stdext::hash_map (VC++ 2005 xhash, sub_9CF8B0 / sub_925B00 document @ 0x28 bytes).
  namespace stdext_hash_map {
    inline constexpr std::size_t list = 0x04;         // _List  (this + 1)
    inline constexpr std::size_t list_end = 0x08;     // end node ptr (this + 2)
    inline constexpr std::size_t bucket_begin = 0x14; // _Vec._Myfirst (this + 5)
    inline constexpr std::size_t bucket_end = 0x18;   // _Vec._Mylast  (this + 6)
    inline constexpr std::size_t mask = 0x20;         // _Mask (this + 8)
    inline constexpr std::size_t maxidx = 0x24;       // _Maxidx (this + 9)

    // std::list node holding pair<const string, Section*> (sub_9CF8B0 / sub_9CFB30).
    inline constexpr std::size_t node_pair_key = 0x08;    // _Myval.first  (basic_string<char>)
    inline constexpr std::size_t node_pair_mapped = 0x24; // _Myval.second (Section*)
  } // namespace stdext_hash_map

  struct list_node {
    list_node* next;
    list_node* prev;
    void* value;
  };

  class list_ref {
  public:
    // std::list object: _Myhead sentinel pointer stored @ object + 4.
    static auto from(const void* object) -> list_ref;
    // Circular-list sentinel node pointer (e.g. ui res section + 0x04).
    static auto from_sentinel(const void* sentinel_node) -> list_ref;

    auto object() const -> const void* { return object_; }
    auto size() const -> std::uint32_t;
    auto sentinel() const -> const void*;

    template<typename Fn> auto for_each(Fn&& fn) const -> void;

  private:
    const void* object_ = nullptr;
    const void* sentinel_node_ = nullptr;
  };

  // Parsed pstitle document header (stdext::hash_map<string, Section>).
  class stdext_hash_map_ref {
  public:
    static auto from(const void* object) -> stdext_hash_map_ref;

    auto object() const -> const void* { return object_; }
    auto element_count() const -> std::uint32_t;
    auto list_view() const -> list_ref;

    // fn(const char* section_name, const void* section)
    template<typename Fn> auto for_each_section(Fn&& fn) const -> void;

  private:
    const void* object_ = nullptr;
  };

  class vector_ref {
  public:
    static auto from(const void* object) -> vector_ref;
    static auto from(const void* object, std::size_t element_size) -> vector_ref;

    auto object() const -> const void* { return object_; }
    auto begin() const -> void*;
    auto end() const -> void*;
    auto size_bytes() const -> std::size_t;
    auto element_size() const -> std::size_t;
    auto count() const -> std::size_t;

    template<typename Fn> auto for_each(Fn&& fn) const -> void;

  private:
    const void* object_ = nullptr;
    std::size_t element_size_ = 0;
  };

  // ---------------------------------------------------------------------------
  // Owned objects (project / DLL storage — pass raw() to game APIs)
  // ---------------------------------------------------------------------------

  class wstring {
  public:
    wstring();
    wstring(const wchar_t* text);
    wstring(const wstring& other);
    wstring(wstring&& other) noexcept;
    ~wstring();

    auto operator=(const wstring& other) -> wstring&;
    auto operator=(wstring&& other) noexcept -> wstring&;
    auto operator=(const wchar_t* text) -> wstring&;

    auto raw() -> void* { return storage_; }
    auto raw() const -> const void* { return storage_; }
    auto capacity() const -> std::uint32_t;
    auto length() const -> std::uint32_t;
    auto data() const -> const wchar_t*;
    auto empty() const -> bool;

    auto assign(const wchar_t* text) -> wstring&;
    auto assign(const wchar_t* text, std::size_t char_count) -> wstring&;
    auto clear() -> void;

    auto view() const -> wstring_ref;

  private:
    alignas(4) std::uint8_t storage_[wstring_object_size]{};

    auto init_empty() -> void;
    auto words() -> std::uint32_t*;
    auto words() const -> const std::uint32_t*;
  };

  class string {
  public:
    string();
    string(const char* text);
    string(const string& other);
    string(string&& other) noexcept;
    ~string();

    auto operator=(const string& other) -> string&;
    auto operator=(string&& other) noexcept -> string&;
    auto operator=(const char* text) -> string&;

    auto raw() -> void* { return storage_; }
    auto raw() const -> const void* { return storage_; }
    auto capacity() const -> std::uint32_t;
    auto length() const -> std::uint32_t;
    auto data() const -> const char*;
    auto empty() const -> bool;

    auto assign(const char* text) -> string&;
    auto assign(const char* text, std::size_t byte_count) -> string&;
    auto clear() -> void;

    auto view() const -> string_ref;

  private:
    alignas(4) std::uint8_t storage_[string_object_size]{};

    auto init_empty() -> void;
    auto words() -> std::uint32_t*;
    auto words() const -> const std::uint32_t*;
  };

  // UI resource map (int key -> void*), same layout as CPSilkroad+0xB0 (48 bytes).
  // The second lookup flag is not a creation flag. sub_9CF790 adds the map's
  // base key at +0x2C when it is set, then performs a normal map lookup.
  class ui_res_map {
  public:
    ui_res_map();
    ui_res_map(const ui_res_map&) = delete;
    ui_res_map(ui_res_map&& other) noexcept;
    ~ui_res_map();

    auto operator=(const ui_res_map&) = delete;
    auto operator=(ui_res_map&& other) noexcept -> ui_res_map&;

    auto raw() -> void* { return storage_; }
    auto raw() const -> const void* { return storage_; }
    auto size() const -> std::uint32_t;
    auto find(std::uint32_t key, bool add_base_key = false) const -> void*;
    auto insert(std::uint32_t key, void* value) -> bool;
    auto erase(std::uint32_t key) -> bool;
    auto clear() -> void;

    auto view() const -> map_ref;

  private:
    alignas(4) std::uint8_t storage_[ui_res_map_size]{};
    bool initialized_ = false;

    auto construct() -> void;
    auto destroy() -> void;
  };

  // vector header (12 bytes) backed by game heap; elements are plain-old-data.
  class vector_u8 {
  public:
    vector_u8();
    vector_u8(const vector_u8&) = delete;
    vector_u8(vector_u8&& other) noexcept;
    ~vector_u8();

    auto operator=(const vector_u8&) = delete;
    auto operator=(vector_u8&& other) noexcept -> vector_u8&;

    auto raw() -> void* { return storage_; }
    auto raw() const -> const void* { return storage_; }
    auto size() const -> std::size_t;
    auto capacity() const -> std::size_t;
    auto data() -> std::uint8_t*;
    auto data() const -> const std::uint8_t*;
    auto empty() const -> bool;

    auto push_back(std::uint8_t value) -> void;
    auto clear() -> void;

  private:
    alignas(4) std::uint8_t storage_[vector_object_size]{};

    auto init_empty() -> void;
    auto destroy() -> void;
  };

  template<typename T> class vector {
  public:
    auto raw() -> void* { return storage_; }
    auto raw() const -> const void* { return storage_; }
    auto first() -> T* { return *reinterpret_cast<T**>(&storage_[4]); }
    auto first() const -> const T* { return *reinterpret_cast<const T* const*>(&storage_[4]); }
    auto last() -> T* { return *reinterpret_cast<T**>(&storage_[8]); }
    auto last() const -> const T* { return *reinterpret_cast<const T* const*>(&storage_[8]); }
    auto end() -> T* { return *reinterpret_cast<T**>(&storage_[12]); }
    auto end() const -> const T* { return *reinterpret_cast<const T* const*>(&storage_[12]); }

    auto size() const -> std::size_t { return last() - first(); }
    auto empty() const -> bool { return first() == last(); }
    auto operator[](std::size_t index) -> T& { return first()[index]; }
    auto operator[](std::size_t index) const -> const T& { return first()[index]; }

  private:
    alignas(4) std::uint8_t storage_[vector_object_size]{};
  };

  // ---------------------------------------------------------------------------
  // list_ref / stdext_hash_map_ref / vector_ref templates (header-only iteration)
  // ---------------------------------------------------------------------------

  template<typename Fn> auto list_ref::for_each(Fn&& fn) const -> void {
    const void* end = sentinel();
    if (!end || !is_readable_ptr(end, list_node_size)) {
      return;
    }

    std::uint32_t next_addr = 0;
    if (!try_read_u32(end, &next_addr)) {
      return;
    }
    const void* node = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(next_addr));

    for (int guard = 0; node && node != end && guard < 4096; ++guard) {
      if (!is_readable_ptr(node, list_node_size)) {
        break;
      }
      const auto value_addr = static_cast<const std::uint8_t*>(node) + 8;
      fn(const_cast<void*>(static_cast<const void*>(value_addr)));

      if (!try_read_u32(node, &next_addr)) {
        break;
      }
      node = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(next_addr));
    }
  }

  template<typename Fn> auto stdext_hash_map_ref::for_each_section(Fn&& fn) const -> void {
    if (!object_ || !is_readable_ptr(object_, stdext_hash_map_size)) {
      return;
    }

    list_view().for_each([&](void* pair_base) {
      if (!pair_base || !is_readable_ptr(pair_base, string_object_size + sizeof(void*))) {
        return;
      }

      char section_name[64]{};
      string_ref::from(pair_base).copy_to(section_name, sizeof(section_name));

      std::uint32_t section_addr = 0;
      const auto* mapped = static_cast<const std::uint8_t*>(pair_base) + stdext_hash_map::node_pair_mapped;
      if (!try_read_u32(mapped, &section_addr)) {
        return;
      }
      const auto* section = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(section_addr));
      if (!is_game_ptr(section)) {
        return;
      }

      fn(section_name, section);
    });
  }

  template<typename Fn> auto vector_ref::for_each(Fn&& fn) const -> void {
    if (!object_ || element_size_ == 0) {
      return;
    }

    const auto* begin_ptr = static_cast<const std::uint8_t*>(begin());
    const auto* end_ptr = static_cast<const std::uint8_t*>(end());
    if (!begin_ptr || !end_ptr || end_ptr < begin_ptr) {
      return;
    }

    const auto bytes = static_cast<std::size_t>(end_ptr - begin_ptr);
    if (!is_readable_ptr(begin_ptr, bytes)) {
      return;
    }

    for (const auto* cursor = begin_ptr; cursor < end_ptr; cursor += element_size_) {
      fn(const_cast<void*>(static_cast<const void*>(cursor)));
    }
  }

} // namespace ext_client::msvc9
