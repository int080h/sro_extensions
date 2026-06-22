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
#include <cstring>
#include <cwchar>
#include <string_view>
#include "utils/memory.hpp"

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

  // Forward pointer check functions to centralized memory module
  using ext_client::utils::memory::is_game_ptr;
  using ext_client::utils::memory::is_readable_ptr;

  struct string_pod {
    std::uint32_t words[7];
  };

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

    class iterator {
    public:
      iterator() = default;
      explicit iterator(const list_node* n) : node_(n) {}
      auto operator*() const -> const T& { return node_->_myval; }
      auto operator->() const -> const T* { return &node_->_myval; }
      auto operator++() -> iterator& { if (node_) node_ = node_->_next; return *this; }
      auto operator++(int) -> iterator { iterator tmp = *this; ++*this; return tmp; }
      auto operator--() -> iterator& { if (node_) node_ = node_->_prev; return *this; }
      auto operator--(int) -> iterator { iterator tmp = *this; --*this; return tmp; }
      auto operator==(const iterator& o) const -> bool { return node_ == o.node_; }
      auto operator!=(const iterator& o) const -> bool { return node_ != o.node_; }
    private:
      const list_node* node_ = nullptr;
    };

    auto begin() const -> iterator { return iterator{_next}; }
    auto end() const -> iterator { return iterator{this}; }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void {
      for (const auto* n = _next; n && n != this; n = n->_next) {
        fn(n->_myval);
      }
    }
  };

  template<typename T>
  class n_list {
  public:
    using node_type = list_node<T>;
    using value_type = T;

    void* allocator_;         // +0x00
    node_type* sentinel_;     // +0x04
    std::uint32_t size_;      // +0x08

    static auto from_object(const void* object) -> n_list {
      n_list result;
      if (object) {
        result.allocator_ = *reinterpret_cast<void* const*>(static_cast<const std::uint8_t*>(object) + 0);
        result.sentinel_ = *reinterpret_cast<node_type* const*>(static_cast<const std::uint8_t*>(object) + 4);
        result.size_ = *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(object) + 8);
      } else {
        result.allocator_ = nullptr;
        result.sentinel_ = nullptr;
        result.size_ = 0;
      }
      return result;
    }

    static auto from(const void* list_head) -> n_list {
      n_list result;
      result.allocator_ = nullptr;
      result.sentinel_ = const_cast<node_type*>(static_cast<const node_type*>(list_head));
      result.size_ = static_cast<std::uint32_t>(-1);
      return result;
    }

    auto sentinel() const -> const node_type* {
      return sentinel_;
    }

    auto empty() const -> bool {
      const auto* end_node = sentinel();
      if (!end_node) {
        return true;
      }
      return end_node->_next == end_node;
    }

    auto size() const -> std::size_t {
      return size_;
    }

    class iterator {
    public:
      iterator() = default;
      explicit iterator(node_type* node) : node_(node) {}

      auto operator*() -> T& { return node_->_myval; }
      auto operator*() const -> const T& { return node_->_myval; }
      auto operator->() -> T* { return &node_->_myval; }
      auto operator->() const -> const T* { return &node_->_myval; }
      auto operator++() -> iterator& { if (node_) node_ = node_->_next; return *this; }
      auto operator++(int) -> iterator { iterator tmp = *this; ++*this; return tmp; }
      auto operator--() -> iterator& { if (node_) node_ = node_->_prev; return *this; }
      auto operator--(int) -> iterator { iterator tmp = *this; --*this; return tmp; }
      auto operator==(const iterator& o) const -> bool { return node_ == o.node_; }
      auto operator!=(const iterator& o) const -> bool { return node_ != o.node_; }
      auto node() const -> node_type* { return node_; }
    private:
      node_type* node_ = nullptr;
    };

    class const_iterator {
    public:
      const_iterator() = default;
      explicit const_iterator(const node_type* node) : node_(node) {}

      auto operator*() const -> const T& { return node_->_myval; }
      auto operator->() const -> const T* { return &node_->_myval; }
      auto operator++() -> const_iterator& { if (node_) node_ = node_->_next; return *this; }
      auto operator++(int) -> const_iterator { const_iterator tmp = *this; ++*this; return tmp; }
      auto operator--() -> const_iterator& { if (node_) node_ = node_->_prev; return *this; }
      auto operator--(int) -> const_iterator { const_iterator tmp = *this; --*this; return tmp; }
      auto operator==(const const_iterator& o) const -> bool { return node_ == o.node_; }
      auto operator!=(const const_iterator& o) const -> bool { return node_ != o.node_; }
      auto node() const -> const node_type* { return node_; }
    private:
      const node_type* node_ = nullptr;
    };

    auto begin() -> iterator {
      const auto* end_node = sentinel();
      if (!end_node) return iterator{};
      return iterator{end_node->_next};
    }

    auto begin() const -> const_iterator {
      const auto* end_node = sentinel();
      if (!end_node) return const_iterator{};
      return const_iterator{end_node->_next};
    }

    auto end() -> iterator {
      return iterator{const_cast<node_type*>(sentinel())};
    }

    auto end() const -> const_iterator {
      return const_iterator{sentinel()};
    }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void {
      const auto* end_node = sentinel();
      if (!end_node || !is_game_ptr(end_node) || !is_readable_ptr(end_node)) {
        return;
      }
      const auto* node = end_node->_next;
      for (int guard = 0; node && node != end_node && is_game_ptr(node) && is_readable_ptr(node) && guard < 4096; ++guard) {
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

    auto push_front(const T& value) -> void {}
    auto erase(iterator it) -> void {}
  };

  template<typename T>
  using list = n_list<T>;

  // Alias for backward compatibility during the transition.
  template<typename T>
  using list_view = list<T>;

  // ---------------------------------------------------------------------------
  // std::n_map<K, V> — typed overlay for the game's std::map with custom allocator.
  //
  // Node layout confirmed via IDA (sub_402110 lower_bound, sub_4016F0 GetInterfaceObj,
  // sub_4029F0 cleanup, sub_403DC0 node alloc):
  //   +0x00: left, +0x04: parent, +0x08: right
  //   +0x0C: key (K), +0x10: value (V)
  //   +0x14: color (byte), +0x15: isnil (byte)
  //   Node size: 0x18
  //
  // Map object layout (12 bytes):
  //   +0x00: allocator (4 bytes)
  //   +0x04: sentinel node pointer
  //   +0x08: size (uint32)
  //
  // Use directly: n_map<DWORD, void*> m_map; m_map.for_each(...)
  // For pointers: reinterpret_cast<const n_map<int, void*>*>(ptr)->for_each(...)
  // ---------------------------------------------------------------------------
  template<typename K, typename V>
  struct n_pair {
    const K& first;
    V& second;
  };

  template<typename K, typename V>
  struct n_const_pair {
    const K& first;
    const V& second;
  };

  template<typename K, typename V>
  struct n_map_pair_proxy {
    n_pair<K, V> val;
    auto operator->() -> n_pair<K, V>* { return &val; }
    auto operator->() const -> const n_pair<K, V>* { return &val; }
  };

  template<typename K, typename V>
  struct n_map_const_pair_proxy {
    n_const_pair<K, V> val;
    auto operator->() const -> const n_const_pair<K, V>* { return &val; }
  };

  template<typename K, typename V>
  struct n_map_node {
    n_map_node* left;     // +0x00
    n_map_node* parent;   // +0x04
    n_map_node* right;    // +0x08
    K key;                // +0x0C
    V value;              // +0x10
    std::uint8_t color;   // +0x14
    std::uint8_t isnil;   // +0x15
  };

  template<typename K, typename V>
  struct n_map {
    using node_type = n_map_node<K, V>;
    using key_type = K;
    using mapped_type = V;

    void* allocator_;         // +0x00
    node_type* sentinel_;     // +0x04
    std::uint32_t size_;      // +0x08

    auto sentinel() const -> const node_type* { return sentinel_; }
    auto size() const -> std::uint32_t { return size_; }
    auto empty() const -> bool { return size_ == 0; }

    static auto is_nil(const node_type* node) -> bool {
      if (!node || !is_game_ptr(node) || !is_readable_ptr(node)) {
        return true;
      }
      return node->isnil != 0;
    }

    static auto next_node(const node_type* node, const node_type* end) -> const node_type* {
      if (!node || !end || is_nil(node)) return end;
      if (!is_nil(node->right)) {
        auto* walk = node->right;
        while (!is_nil(walk->left)) walk = walk->left;
        return walk;
      }
      auto* parent = node->parent;
      while (!is_nil(parent) && parent != end && node == parent->right) {
        node = parent;
        parent = parent->parent;
      }
      return parent;
    }

    static auto prev_node(const node_type* node, const node_type* end) -> const node_type* {
      if (!node || !end) return end;
      if (node == end) {
        return end->right;
      }
      if (!is_nil(node->left)) {
        auto* walk = node->left;
        while (!is_nil(walk->right)) walk = walk->right;
        return walk;
      }
      auto* parent = node->parent;
      while (!is_nil(parent) && parent != end && node == parent->left) {
        node = parent;
        parent = parent->parent;
      }
      return parent;
    }

    class iterator {
    public:
      iterator() = default;
      iterator(node_type* node, node_type* end) : node_(node), end_(end) {}

      auto operator*() -> n_pair<K, V> { return n_pair<K, V>{node_->key, node_->value}; }
      auto operator->() -> n_map_pair_proxy<K, V> { return n_map_pair_proxy<K, V>{n_pair<K, V>{node_->key, node_->value}}; }

      auto operator++() -> iterator& {
        if (node_) node_ = const_cast<node_type*>(next_node(node_, end_));
        return *this;
      }
      auto operator++(int) -> iterator {
        iterator tmp = *this;
        ++*this;
        return tmp;
      }
      auto operator--() -> iterator& {
        if (node_) node_ = const_cast<node_type*>(prev_node(node_, end_));
        return *this;
      }
      auto operator--(int) -> iterator {
        iterator tmp = *this;
        --*this;
        return tmp;
      }
      auto operator==(const iterator& other) const -> bool { return node_ == other.node_; }
      auto operator!=(const iterator& other) const -> bool { return node_ != other.node_; }

      node_type* node_ = nullptr;
      node_type* end_ = nullptr;
    };

    class const_iterator {
    public:
      const_iterator() = default;
      const_iterator(const node_type* node, const node_type* end) : node_(node), end_(end) {}

      auto operator*() const -> n_const_pair<K, V> { return n_const_pair<K, V>{node_->key, node_->value}; }
      auto operator->() const -> n_map_const_pair_proxy<K, V> { return n_map_const_pair_proxy<K, V>{n_const_pair<K, V>{node_->key, node_->value}}; }

      auto operator++() -> const_iterator& {
        if (node_) node_ = next_node(node_, end_);
        return *this;
      }
      auto operator++(int) -> const_iterator {
        const_iterator tmp = *this;
        ++*this;
        return tmp;
      }
      auto operator--() -> const_iterator& {
        if (node_) node_ = prev_node(node_, end_);
        return *this;
      }
      auto operator--(int) -> const_iterator {
        const_iterator tmp = *this;
        --*this;
        return tmp;
      }
      auto operator==(const const_iterator& other) const -> bool { return node_ == other.node_; }
      auto operator!=(const const_iterator& other) const -> bool { return node_ != other.node_; }

      const node_type* node_ = nullptr;
      const node_type* end_ = nullptr;
    };

    auto begin() -> iterator {
      const auto* end_node = sentinel();
      if (!end_node || is_nil(end_node->parent)) return iterator{const_cast<node_type*>(end_node), const_cast<node_type*>(end_node)};
      auto* walk = end_node->parent;
      while (!is_nil(walk->left)) walk = walk->left;
      return iterator{const_cast<node_type*>(walk), const_cast<node_type*>(end_node)};
    }

    auto begin() const -> const_iterator {
      const auto* end_node = sentinel();
      if (!end_node || is_nil(end_node->parent)) return const_iterator{end_node, end_node};
      auto* walk = end_node->parent;
      while (!is_nil(walk->left)) walk = walk->left;
      return const_iterator{walk, end_node};
    }

    auto end() -> iterator {
      return iterator{const_cast<node_type*>(sentinel()), const_cast<node_type*>(sentinel())};
    }

    auto end() const -> const_iterator {
      return const_iterator{sentinel(), sentinel()};
    }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void {
      const auto* end_node = sentinel();
      if (!end_node || !is_game_ptr(end_node) || !is_readable_ptr(end_node)) return;
      for (const auto* node = end_node->parent; node && node != end_node && is_game_ptr(node) && is_readable_ptr(node) && !is_nil(node); node = next_node(node, end_node)) {
        fn(node->key, node->value);
      }
    }

    auto find(const K& key) const -> const_iterator {
      const auto* end_node = sentinel();
      if (!end_node) return end();
      auto* node = end_node->parent;
      while (!is_nil(node)) {
        if (key < node->key) {
          node = node->left;
        } else if (node->key < key) {
          node = node->right;
        } else {
          return const_iterator{node, end_node};
        }
      }
      return end();
    }

    auto find(const K& key) -> iterator {
      const auto* end_node = sentinel();
      if (!end_node) return end();
      auto* node = end_node->parent;
      while (!is_nil(node)) {
        if (key < node->key) {
          node = node->left;
        } else if (node->key < key) {
          node = node->right;
        } else {
          return iterator{const_cast<node_type*>(node), const_cast<node_type*>(end_node)};
        }
      }
      return end();
    }
  };

  using n_map_int_void = n_map<int, void*>;
  static_assert(sizeof(n_map_int_void) == 0x0C, "n_map size must be 0x0C");
  static_assert(offsetof(n_map_int_void, sentinel_) == 0x04, "n_map sentinel offset");
  static_assert(offsetof(n_map_int_void, size_) == 0x08, "n_map size offset");

  // std::n_set
  template<typename T>
  struct n_set_node {
    n_set_node* left;
    n_set_node* parent;
    n_set_node* right;
    T key;
    std::uint8_t color;
    std::uint8_t isnil;
  };

  template<typename T>
  struct n_set {
    using node_type = n_set_node<T>;
    using value_type = T;
    using key_type = T;

    void* allocator_;         // +0x00
    node_type* sentinel_;     // +0x04
    std::uint32_t size_;      // +0x08

    auto sentinel() const -> const node_type* { return sentinel_; }
    auto size() const -> std::uint32_t { return size_; }
    auto empty() const -> bool { return size_ == 0; }

    static auto is_nil(const node_type* node) -> bool {
      return !node || node->isnil != 0;
    }

    static auto next_node(const node_type* node, const node_type* end) -> const node_type* {
      if (!node || !end || is_nil(node)) return end;
      if (!is_nil(node->right)) {
        auto* walk = node->right;
        while (!is_nil(walk->left)) walk = walk->left;
        return walk;
      }
      auto* parent = node->parent;
      while (!is_nil(parent) && parent != end && node == parent->right) {
        node = parent;
        parent = parent->parent;
      }
      return parent;
    }

    static auto prev_node(const node_type* node, const node_type* end) -> const node_type* {
      if (!node || !end) return end;
      if (node == end) {
        return end->right;
      }
      if (!is_nil(node->left)) {
        auto* walk = node->left;
        while (!is_nil(walk->right)) walk = walk->right;
        return walk;
      }
      auto* parent = node->parent;
      while (!is_nil(parent) && parent != end && node == parent->left) {
        node = parent;
        parent = parent->parent;
      }
      return parent;
    }

    class iterator {
    public:
      iterator() = default;
      iterator(node_type* node, node_type* end) : node_(node), end_(end) {}

      auto operator*() const -> const T& { return node_->key; }
      auto operator->() const -> const T* { return &node_->key; }
      auto operator++() -> iterator& {
        if (node_) node_ = const_cast<node_type*>(next_node(node_, end_));
        return *this;
      }
      auto operator++(int) -> iterator {
        iterator tmp = *this;
        ++*this;
        return tmp;
      }
      auto operator--() -> iterator& {
        if (node_) node_ = const_cast<node_type*>(prev_node(node_, end_));
        return *this;
      }
      auto operator--(int) -> iterator {
        iterator tmp = *this;
        --*this;
        return tmp;
      }
      auto operator==(const iterator& other) const -> bool { return node_ == other.node_; }
      auto operator!=(const iterator& other) const -> bool { return node_ != other.node_; }

      node_type* node_ = nullptr;
      node_type* end_ = nullptr;
    };

    using const_iterator = iterator;

    auto begin() const -> iterator {
      const auto* end_node = sentinel();
      if (!end_node || is_nil(end_node->parent)) return iterator{const_cast<node_type*>(end_node), const_cast<node_type*>(end_node)};
      auto* walk = end_node->parent;
      while (!is_nil(walk->left)) walk = walk->left;
      return iterator{const_cast<node_type*>(walk), const_cast<node_type*>(end_node)};
    }

    auto end() const -> iterator {
      return iterator{const_cast<node_type*>(sentinel()), const_cast<node_type*>(sentinel())};
    }
  };

  // std::n_hash_map
  template<typename K, typename V>
  struct n_hash_node {
    n_hash_node* next;
    n_hash_node* prev;
    K first;
    V second;
  };

  template<typename K, typename V>
  class n_hash_map {
  public:
    using value_type = std::pair<const K, V>;
    using node_type = n_hash_node<K, V>;

    void* traits_;            // +0x00
    void* list_alloc_;        // +0x04
    node_type* list_sentinel_;// +0x08
    std::uint32_t list_size_; // +0x0C
    void* vec_alloc_;         // +0x10
    void* vec_first_;         // +0x14
    void* vec_last_;          // +0x18
    void* vec_end_;           // +0x1C
    std::uint32_t mask_;      // +0x20
    std::uint32_t maxidx_;    // +0x24

    class iterator {
    public:
      iterator() = default;
      explicit iterator(node_type* node) : node_(node) {}

      auto operator*() -> n_pair<K, V> { return n_pair<K, V>{node_->first, node_->second}; }
      auto operator->() -> n_map_pair_proxy<K, V> { return n_map_pair_proxy<K, V>{n_pair<K, V>{node_->first, node_->second}}; }

      auto operator++() -> iterator& {
        if (node_) node_ = node_->next;
        return *this;
      }
      auto operator++(int) -> iterator {
        iterator tmp = *this;
        ++*this;
        return tmp;
      }
      auto operator--() -> iterator& {
        if (node_) node_ = node_->prev;
        return *this;
      }
      auto operator--(int) -> iterator {
        iterator tmp = *this;
        --*this;
        return tmp;
      }
      auto operator==(const iterator& other) const -> bool { return node_ == other.node_; }
      auto operator!=(const iterator& other) const -> bool { return node_ != other.node_; }

      node_type* node_ = nullptr;
    };

    class const_iterator {
    public:
      const_iterator() = default;
      explicit const_iterator(const node_type* node) : node_(node) {}

      auto operator*() const -> n_const_pair<K, V> { return n_const_pair<K, V>{node_->first, node_->second}; }
      auto operator->() const -> n_map_const_pair_proxy<K, V> { return n_map_const_pair_proxy<K, V>{n_const_pair<K, V>{node_->first, node_->second}}; }

      auto operator++() -> const_iterator& {
        if (node_) node_ = node_->next;
        return *this;
      }
      auto operator++(int) -> const_iterator {
        const_iterator tmp = *this;
        ++*this;
        return tmp;
      }
      auto operator--() -> const_iterator& {
        if (node_) node_ = node_->prev;
        return *this;
      }
      auto operator--(int) -> const_iterator {
        const_iterator tmp = *this;
        --*this;
        return tmp;
      }
      auto operator==(const const_iterator& other) const -> bool { return node_ == other.node_; }
      auto operator!=(const const_iterator& other) const -> bool { return node_ != other.node_; }

      const node_type* node_ = nullptr;
    };

    auto begin() -> iterator { return iterator{list_sentinel_ ? list_sentinel_->next : nullptr}; }
    auto begin() const -> const_iterator { return const_iterator{list_sentinel_ ? list_sentinel_->next : nullptr}; }
    auto end() -> iterator { return iterator{list_sentinel_}; }
    auto end() const -> const_iterator { return const_iterator{list_sentinel_}; }

    auto size() const -> std::size_t { return list_size_; }
    auto empty() const -> bool { return list_size_ == 0; }
  };

  static_assert(sizeof(n_hash_map<int, void*>) == 40, "n_hash_map size must be 40 bytes");

  // stdext::hash_map (VC++ 2005 xhash, sub_9CF8B0 / sub_925B00 document @ 0x28 bytes).
  namespace stdext_hash_map {
    inline constexpr std::size_t internal = 0x00;     // 4-byte field (this + 0)
    inline constexpr std::size_t list = 0x04;         // std::list<pair> _List (this + 1, 12 bytes)
    inline constexpr std::size_t list_alloc = 0x04;   // list allocator (this + 1)
    inline constexpr std::size_t list_end = 0x08;     // list sentinel ptr (this + 2)
    inline constexpr std::size_t list_size = 0x0C;    // list _Mysize (this + 3)
    inline constexpr std::size_t vec = 0x10;          // std::vector<iter> _Vec (this + 4, 16 bytes)
    inline constexpr std::size_t vec_alloc = 0x10;    // vector allocator (this + 4)
    inline constexpr std::size_t bucket_begin = 0x14; // _Vec._Myfirst (this + 5)
    inline constexpr std::size_t bucket_end = 0x18;   // _Vec._Mylast  (this + 6)
    inline constexpr std::size_t bucket_cap = 0x1C;   // _Vec._Myend   (this + 7)
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

    auto object() const -> const void* { return object_; }
    auto size() const -> std::uint32_t;
    auto sentinel() const -> const void*;

    template<typename Fn> auto for_each(Fn&& fn) const -> void {
      impl_.for_each([&](void* value) { fn(value); });
    }

  private:
    list<void*> impl_;
    const void* object_ = nullptr;
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

    // Standard additions
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);
    auto c_str() const -> const wchar_t* { return data(); }
    auto size() const -> std::size_t { return length(); }

    using iterator = wchar_t*;
    using const_iterator = const wchar_t*;
    auto begin() -> iterator { return const_cast<wchar_t*>(data()); }
    auto begin() const -> const_iterator { return data(); }
    auto end() -> iterator { return const_cast<wchar_t*>(data()) + length(); }
    auto end() const -> const_iterator { return data() + length(); }

    auto operator[](std::size_t index) -> wchar_t& { return const_cast<wchar_t*>(data())[index]; }
    auto operator[](std::size_t index) const -> const wchar_t& { return data()[index]; }

    auto operator==(const wstring& other) const -> bool {
      return length() == other.length() && std::wmemcmp(data(), other.data(), length()) == 0;
    }
    auto operator==(const wchar_t* other) const -> bool {
      return other && std::wcscmp(data(), other) == 0;
    }
    auto operator!=(const wstring& other) const -> bool { return !(*this == other); }
    auto operator!=(const wchar_t* other) const -> bool { return !(*this == other); }
    auto operator<(const wstring& other) const -> bool {
      return std::wcscmp(data(), other.data()) < 0;
    }

    auto find(const wchar_t* s, std::size_t pos = 0) const noexcept -> std::size_t {
      std::wstring_view self(data(), length());
      return self.find(s, pos);
    }
    auto find(wchar_t c, std::size_t pos = 0) const noexcept -> std::size_t {
      std::wstring_view self(data(), length());
      return self.find(c, pos);
    }
    auto find(const wstring& str, std::size_t pos = 0) const noexcept -> std::size_t {
      std::wstring_view self(data(), length());
      return self.find(str.data(), pos, str.length());
    }

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

    // Standard additions
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);
    auto c_str() const -> const char* { return data(); }
    auto size() const -> std::size_t { return length(); }

    using iterator = char*;
    using const_iterator = const char*;
    auto begin() -> iterator { return const_cast<char*>(data()); }
    auto begin() const -> const_iterator { return data(); }
    auto end() -> iterator { return const_cast<char*>(data()) + length(); }
    auto end() const -> const_iterator { return data() + length(); }

    auto operator[](std::size_t index) -> char& { return const_cast<char*>(data())[index]; }
    auto operator[](std::size_t index) const -> const char& { return data()[index]; }

    auto operator==(const string& other) const -> bool {
      return length() == other.length() && std::memcmp(data(), other.data(), length()) == 0;
    }
    auto operator==(const char* other) const -> bool {
      return other && std::strcmp(data(), other) == 0;
    }
    auto operator!=(const string& other) const -> bool { return !(*this == other); }
    auto operator!=(const char* other) const -> bool { return !(*this == other); }
    auto operator<(const string& other) const -> bool {
      return std::strcmp(data(), other.data()) < 0;
    }

    auto find(const char* s, std::size_t pos = 0) const noexcept -> std::size_t {
      std::string_view self(data(), length());
      return self.find(s, pos);
    }
    auto find(char c, std::size_t pos = 0) const noexcept -> std::size_t {
      std::string_view self(data(), length());
      return self.find(c, pos);
    }
    auto find(const string& str, std::size_t pos = 0) const noexcept -> std::size_t {
      std::string_view self(data(), length());
      return self.find(str.data(), pos, str.length());
    }

  private:
    alignas(4) std::uint8_t storage_[string_object_size]{};

    auto init_empty() -> void;
    auto words() -> std::uint32_t*;
    auto words() const -> const std::uint32_t*;
  };

  inline auto operator==(const char* lhs, const string& rhs) -> bool { return rhs == lhs; }
  inline auto operator!=(const char* lhs, const string& rhs) -> bool { return rhs != lhs; }
  inline auto operator==(const wchar_t* lhs, const wstring& rhs) -> bool { return rhs == lhs; }
  inline auto operator!=(const wchar_t* lhs, const wstring& rhs) -> bool { return rhs != lhs; }

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

  template<typename T>
  class n_vector {
  public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    void* allocator_;         // +0x00
    T* first_;                // +0x04
    T* last_;                 // +0x08
    T* end_;                  // +0x0C

    auto raw() -> void* { return this; }
    auto raw() const -> const void* { return this; }
    auto first() -> T* { return first_; }
    auto first() const -> const T* { return first_; }
    auto last() -> T* { return last_; }
    auto last() const -> const T* { return last_; }
    auto end_ptr() -> T* { return end_; }
    auto end_ptr() const -> const T* { return end_; }

    auto begin() -> iterator { return first_; }
    auto begin() const -> const_iterator { return first_; }
    auto end() -> iterator { return last_; }
    auto end() const -> const_iterator { return last_; }

    auto size() const -> std::size_t { return last_ - first_; }
    auto empty() const -> bool { return first_ == last_; }
    auto operator[](std::size_t index) -> T& { return first_[index]; }
    auto operator[](std::size_t index) const -> const T& { return first_[index]; }
    auto front() -> T& { return *first_; }
    auto front() const -> const T& { return *first_; }
    auto back() -> T& { return *(last_ - 1); }
    auto back() const -> const T& { return *(last_ - 1); }
  };
  static_assert(sizeof(n_vector<void*>) == 16, "n_vector size must be 16 bytes");

  template<typename T>
  using vector = n_vector<T>;

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

namespace std {
  using n_string = ext_client::msvc9::string;
  using n_wstring = ext_client::msvc9::wstring;

  template<typename T>
  using n_vector = ext_client::msvc9::n_vector<T>;

  template<typename T>
  using n_list = ext_client::msvc9::n_list<T>;

  template<typename K, typename V>
  using n_map = ext_client::msvc9::n_map<K, V>;

  template<typename T>
  using n_set = ext_client::msvc9::n_set<T>;

  template<typename K, typename V>
  using n_hash_map = ext_client::msvc9::n_hash_map<K, V>;
}
