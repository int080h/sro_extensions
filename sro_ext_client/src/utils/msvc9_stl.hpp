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
#include <type_traits>
#include <utility>

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

  // Check whether the memory at ptr is currently readable (VirtualQuery).
  // This is slower than is_game_ptr but actually proves the page is accessible.
  auto is_readable_ptr(const void* ptr) -> bool;

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

  // ---------------------------------------------------------------------------
  // MSVC 2008 std::list<T>
  //
  // The game stores the child list as a sentinel pointer (CGWnd+0x7C) in some
  // places and as a full std::list object (12 bytes incl. allocator) in others.
  // Both forms use the same node layout.
  // ---------------------------------------------------------------------------
  template<typename T>
  struct list_node {
    list_node* _next; // +0
    list_node* _prev; // +4
    T _myval;         // +8
  };

  template<typename T>
  class list {
  public:
    using node_type = list_node<T>;
    using value_type = T;

    // Construct from a full std::list<T> object (allocator+_Myhead+_Mysize).
    static auto from_object(const void* object) -> list {
      list result;
      result.object_ = object;
      result.is_sentinel_only_ = false;
      return result;
    }

    // Construct from the raw list head pointer (CGWnd::m_child_list stores the
    // MSVC8 std::list sentinel directly).
    static auto from(const void* list_head) -> list {
      list result;
      result.sentinel_ = static_cast<const node_type*>(list_head);
      result.is_sentinel_only_ = true;
      return result;
    }

    auto object() const -> const void* { return object_; }
    auto sentinel() const -> const node_type* {
      if (is_sentinel_only_) {
        return sentinel_;
      }
      if (!object_) {
        return nullptr;
      }
      const auto* end = *reinterpret_cast<const node_type* const*>(static_cast<const std::uint8_t*>(object_) + 4);
      if (!is_game_ptr(end)) {
        return nullptr;
      }
      return end;
    }

    auto empty() const -> bool {
      const auto* end = sentinel();
      if (!end) {
        return true;
      }
      return end->_next == end;
    }

    auto size() const -> std::size_t {
      if (is_sentinel_only_) {
        return empty() ? 0 : unknown_size_sentinel;
      }
      if (!object_) {
        return 0;
      }
      return *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(object_) + 8);
    }

    class iterator {
    public:
      iterator() = default;
      explicit iterator(const node_type* node) : node_(node) {}

      auto operator*() const -> const T& { return node_->_myval; }
      auto operator->() const -> const T* { return &node_->_myval; }
      auto operator++() -> iterator& {
        if (node_) {
          node_ = node_->_next;
        }
        return *this;
      }
      auto operator==(const iterator& other) const -> bool { return node_ == other.node_; }
      auto operator!=(const iterator& other) const -> bool { return node_ != other.node_; }
      auto node() const -> const node_type* { return node_; }

    private:
      const node_type* node_ = nullptr;
    };

    auto begin() const -> iterator {
      const auto* end = sentinel();
      if (!end) {
        return iterator{};
      }
      return iterator{end->_next};
    }

    auto end() const -> iterator {
      const auto* end = sentinel();
      if (!end) {
        return iterator{};
      }
      return iterator{end};
    }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void {
      const auto* end = sentinel();
      if (!end) {
        return;
      }
      const auto* node = end->_next;
      for (int guard = 0; node && node != end && is_game_ptr(node) && guard < 4096; ++guard) {
        if constexpr (std::is_pointer_v<T>) {
          if (is_game_ptr(node->_myval)) {
            fn(node->_myval);
          }
        } else {
          fn(node->_myval);
        }
        node = node->_next;
      }
    }

    // Insert a new node before begin().
    auto push_front(const T& value) -> void {
      // Requires the caller to use the game's insert function; this is a stub.
    }

    // Erase the node at the iterator.
    auto erase(iterator it) -> void {
      // Requires the caller to use the game's erase function; this is a stub.
    }

  private:
    static constexpr std::size_t unknown_size_sentinel = static_cast<std::size_t>(-1);

    const void* object_ = nullptr;
    const node_type* sentinel_ = nullptr;
    bool is_sentinel_only_ = false;
  };

  // Alias for backward compatibility during the transition.
  template<typename T>
  using list_view = list<T>;

  // ---------------------------------------------------------------------------
  // MSVC 2008 std::map<K,V> tree node
  //
  // Standard layout: left, parent, right, color, isnil, value.
  // NOTE: CResIDManager / ui_res_map uses a different node layout (map_ref).
  // ---------------------------------------------------------------------------
  template<typename K, typename V>
  struct map_node {
    map_node* left;
    map_node* parent;
    map_node* right;
    std::uint8_t color;
    std::uint8_t isnil;
    std::pair<const K, V> value;
  };

  template<typename K, typename V>
  class map {
  public:
    using node_type = map_node<K, V>;
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;

    // Construct from a full std::map object (allocator + sentinel pointer + size).
    static auto from_object(const void* object) -> map {
      map result;
      result.object_ = object;
      return result;
    }

    auto object() const -> const void* { return object_; }

    auto sentinel() const -> const node_type* {
      if (!object_) {
        return nullptr;
      }
      const auto* end = *reinterpret_cast<const node_type* const*>(static_cast<const std::uint8_t*>(object_) + 4);
      if (!is_game_ptr(end)) {
        return nullptr;
      }
      return end;
    }

    auto size() const -> std::size_t {
      if (!object_) {
        return 0;
      }
      return *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(object_) + 8);
    }

    auto empty() const -> bool { return size() == 0; }

    static auto is_nil(const node_type* node) -> bool {
      if (!node) {
        return true;
      }
      return node->isnil != 0;
    }

    static auto next_node(const node_type* node, const node_type* end) -> const node_type* {
      if (!node || !end || is_nil(node)) {
        return end;
      }
      if (!is_nil(node->right)) {
        auto* walk = node->right;
        while (!is_nil(walk->left)) {
          walk = walk->left;
        }
        return walk;
      }
      auto* parent = node->parent;
      while (!is_nil(parent) && parent != end && node == parent->right) {
        node = parent;
        parent = parent->parent;
      }
      return parent;
    }

    class iterator {
    public:
      iterator() = default;
      iterator(const node_type* node, const node_type* end) : node_(node), end_(end) {}

      auto operator*() const -> const value_type& { return node_->value; }
      auto operator->() const -> const value_type* { return &node_->value; }
      auto operator++() -> iterator& {
        if (node_) {
          node_ = next_node(node_, end_);
        }
        return *this;
      }
      auto operator==(const iterator& other) const -> bool { return node_ == other.node_; }
      auto operator!=(const iterator& other) const -> bool { return node_ != other.node_; }
      auto node() const -> const node_type* { return node_; }

    private:
      const node_type* node_ = nullptr;
      const node_type* end_ = nullptr;
    };

    auto begin() const -> iterator {
      const auto* end = sentinel();
      if (!end || is_nil(end->parent)) {
        return iterator{end, end};
      }
      auto* walk = end->parent;
      while (!is_nil(walk->left)) {
        walk = walk->left;
      }
      return iterator{walk, end};
    }

    auto end() const -> iterator {
      const auto* end = sentinel();
      return iterator{end, end};
    }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void {
      const auto* end = sentinel();
      if (!end) {
        return;
      }
      for (auto* node = end->parent; node && node != end && !is_nil(node); node = next_node(node, end)) {
        fn(node->value.first, node->value.second);
      }
    }

    auto find(const K& key) const -> const node_type* {
      const auto* end = sentinel();
      if (!end) {
        return end;
      }
      auto* node = end->parent;
      while (!is_nil(node)) {
        if (key < node->value.first) {
          node = node->left;
        } else if (node->value.first < key) {
          node = node->right;
        } else {
          return node;
        }
      }
      return end;
    }

  private:
    const void* object_ = nullptr;
  };

  // Legacy CResIDManager node (different layout from standard std::map).
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
      if (!object_) {
        return;
      }
      if (size() == 0) {
        return;
      }
      const auto* end = sentinel();
      if (!is_game_ptr(end)) {
        return;
      }
      for (const auto* node = minimum(); node && node != end && is_game_ptr(node); node = next(node, end)) {
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

  // Legacy list_ref wrapper around list<void*>.
  class list_ref {
  public:
    // Raw list head pointer (e.g. ui res section + 0x04).
    static auto from(const void* list_head) -> list_ref;
    // std::list object: _Myhead sentinel pointer stored @ object + 4.
    static auto from_object(const void* object) -> list_ref;

    auto object() const -> const void* { return impl_.object(); }
    auto size() const -> std::uint32_t;
    auto sentinel() const -> const void*;

    template<typename Fn> auto for_each(Fn&& fn) const -> void {
      impl_.for_each([&](void* value) { fn(value); });
    }

  private:
    list<void*> impl_;
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

  // ---------------------------------------------------------------------------
  // MSVC 2008 std::vector<T> view (read-only)
  // ---------------------------------------------------------------------------
  template<typename T>
  class vector_view {
  public:
    using value_type = T;

    static auto from(const void* object) -> vector_view {
      vector_view result;
      result.object_ = object;
      return result;
    }

    auto object() const -> const void* { return object_; }

    auto data() const -> const T* {
      if (!object_) {
        return nullptr;
      }
      return *reinterpret_cast<const T* const*>(static_cast<const std::uint8_t*>(object_) + 4);
    }

    auto size() const -> std::size_t {
      if (!object_) {
        return 0;
      }
      const auto* first = data();
      const auto* last = *reinterpret_cast<const T* const*>(static_cast<const std::uint8_t*>(object_) + 8);
      if (!first || !last || last < first) {
        return 0;
      }
      return static_cast<std::size_t>(last - first);
    }

    auto empty() const -> bool { return size() == 0; }
    auto capacity() const -> std::size_t {
      if (!object_) {
        return 0;
      }
      const auto* first = data();
      const auto* end_cap = *reinterpret_cast<const T* const*>(static_cast<const std::uint8_t*>(object_) + 12);
      if (!first || !end_cap || end_cap < first) {
        return 0;
      }
      return static_cast<std::size_t>(end_cap - first);
    }

    auto operator[](std::size_t index) const -> const T& { return data()[index]; }

    auto begin() const -> const T* { return data(); }
    auto end() const -> const T* {
      if (!object_) {
        return nullptr;
      }
      return *reinterpret_cast<const T* const*>(static_cast<const std::uint8_t*>(object_) + 8);
    }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void {
      const auto* b = data();
      const auto* e = end();
      if (!b || !e || e < b) {
        return;
      }
      for (const auto* cursor = b; cursor < e; ++cursor) {
        fn(*cursor);
      }
    }

  private:
    const void* object_ = nullptr;
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

  template<typename Fn> auto stdext_hash_map_ref::for_each_section(Fn&& fn) const -> void {
    if (!object_) {
      return;
    }
    list_view().for_each([&](void* pair_base) {
      if (!is_game_ptr(pair_base)) {
        return;
      }

      char section_name[64]{};
      string_ref::from(pair_base).copy_to(section_name, sizeof(section_name));

      const auto* mapped = static_cast<const std::uint8_t*>(pair_base) + stdext_hash_map::node_pair_mapped;
      const auto* section = *reinterpret_cast<const void* const*>(mapped);

      fn(section_name, section);
    });
  }

} // namespace ext_client::msvc9
