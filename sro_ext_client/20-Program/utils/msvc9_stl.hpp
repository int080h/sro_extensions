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
  inline constexpr std::size_t child_list_sentinel_size = 12; // sub_864310 allocates 0xC bytes via sub_40A100
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
  using ext_client::utils::memory::is_aligned_ptr;
  using ext_client::utils::memory::is_valid_ptr;
  using ext_client::utils::memory::safe_read;

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
      if (!end_node || !is_game_ptr(end_node) || !is_readable_ptr(end_node)) {
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
      if (!end_node || !is_valid_ptr(end_node)) {
        return;
      }
      const auto* node = end_node->_next;
      for (int guard = 0; node && node != end_node && is_valid_ptr(node) && guard < 4096; ++guard) {
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

    // --- Read/write operations ---

    auto init() -> void {
      allocator_ = nullptr;
      sentinel_ = static_cast<node_type*>(game_heap_alloc(list_node_size));
      if (sentinel_) {
        sentinel_->_next = sentinel_;
        sentinel_->_prev = sentinel_;
      }
      size_ = 0;
    }

    auto destroy() -> void {
      clear();
      if (sentinel_) {
        game_heap_free(sentinel_, list_node_size);
        sentinel_ = nullptr;
      }
      allocator_ = nullptr;
      size_ = 0;
    }

    auto push_back(const T& value) -> void {
      if (!sentinel_) return;
      auto* node = static_cast<node_type*>(game_heap_alloc(list_node_size));
      if (!node) return;
      node->_myval = value;
      node->_next = sentinel_;
      node->_prev = sentinel_->_prev;
      sentinel_->_prev->_next = node;
      sentinel_->_prev = node;
      ++size_;
    }

    auto push_front(const T& value) -> void {
      if (!sentinel_) return;
      auto* node = static_cast<node_type*>(game_heap_alloc(list_node_size));
      if (!node) return;
      node->_myval = value;
      node->_prev = sentinel_;
      node->_next = sentinel_->_next;
      sentinel_->_next->_prev = node;
      sentinel_->_next = node;
      ++size_;
    }

    auto pop_back() -> void {
      if (!sentinel_ || sentinel_->_prev == sentinel_) return;
      auto* node = sentinel_->_prev;
      node->_prev->_next = sentinel_;
      sentinel_->_prev = node->_prev;
      game_heap_free(node, list_node_size);
      --size_;
    }

    auto pop_front() -> void {
      if (!sentinel_ || sentinel_->_next == sentinel_) return;
      auto* node = sentinel_->_next;
      node->_next->_prev = sentinel_;
      sentinel_->_next = node->_next;
      game_heap_free(node, list_node_size);
      --size_;
    }

    auto erase(iterator it) -> void {
      auto* node = it.node();
      if (!node || node == sentinel_) return;
      node->_prev->_next = node->_next;
      node->_next->_prev = node->_prev;
      game_heap_free(node, list_node_size);
      --size_;
    }

    auto clear() -> void {
      if (!sentinel_) return;
      auto* node = sentinel_->_next;
      while (node && node != sentinel_) {
        auto* next = node->_next;
        game_heap_free(node, list_node_size);
        node = next;
      }
      sentinel_->_next = sentinel_;
      sentinel_->_prev = sentinel_;
      size_ = 0;
    }
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
      if (!end_node || !is_valid_ptr(end_node) || is_nil(end_node->parent)) return;
      const auto* start = end_node->left;
      if (is_nil(start)) {
        start = end_node->parent;
        while (!is_nil(start->left)) start = start->left;
      }
      std::size_t visited = 0;
      const std::size_t max_count = size_ + 32;
      for (const auto* node = start; node && node != end_node && is_valid_ptr(node) && !is_nil(node) && visited < max_count; ++visited, node = next_node(node, end_node)) {
        if constexpr (std::is_invocable_r_v<bool, Fn, const K&, const V&>) {
          if (!fn(node->key, node->value)) break;
        } else if constexpr (std::is_invocable_r_v<bool, Fn, const K&, V&>) {
          if (!fn(node->key, const_cast<V&>(node->value))) break;
        } else {
          fn(node->key, node->value);
        }
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

    // --- Read/write operations ---

    static constexpr std::size_t map_node_size = 0x18;

    auto init() -> void {
      allocator_ = nullptr;
      sentinel_ = static_cast<node_type*>(game_heap_alloc(map_node_size));
      if (sentinel_) {
        sentinel_->left = sentinel_;
        sentinel_->parent = sentinel_;
        sentinel_->right = sentinel_;
        sentinel_->isnil = 1;
        sentinel_->color = 1;
      }
      size_ = 0;
    }

    auto destroy() -> void {
      clear();
      if (sentinel_) {
        game_heap_free(sentinel_, map_node_size);
        sentinel_ = nullptr;
      }
      allocator_ = nullptr;
      size_ = 0;
    }

  private:
    static auto rotate_left(node_type* x, node_type** root, node_type* sentinel) -> void {
      auto* y = x->right;
      x->right = y->left;
      if (!is_nil(y->left)) y->left->parent = x;
      y->parent = x->parent;
      if (x == *root) {
        *root = y;
      } else if (x == x->parent->left) {
        x->parent->left = y;
      } else {
        x->parent->right = y;
      }
      y->left = x;
      x->parent = y;
    }

    static auto rotate_right(node_type* x, node_type** root, node_type* sentinel) -> void {
      auto* y = x->left;
      x->left = y->right;
      if (!is_nil(y->right)) y->right->parent = x;
      y->parent = x->parent;
      if (x == *root) {
        *root = y;
      } else if (x == x->parent->right) {
        x->parent->right = y;
      } else {
        x->parent->left = y;
      }
      y->right = x;
      x->parent = y;
    }

    static auto rebalance_insert(node_type* x, node_type* root, node_type* sentinel) -> void {
      x->color = 0; // red
      while (x != root && x->parent->color == 0) {
        if (x->parent == x->parent->parent->left) {
          auto* y = x->parent->parent->right;
          if (y->color == 0) {
            x->parent->color = 1;
            y->color = 1;
            x->parent->parent->color = 0;
            x = x->parent->parent;
          } else {
            if (x == x->parent->right) {
              x = x->parent;
              rotate_left(x, &root, sentinel);
            }
            x->parent->color = 1;
            x->parent->parent->color = 0;
            rotate_right(x->parent->parent, &root, sentinel);
          }
        } else {
          auto* y = x->parent->parent->left;
          if (y->color == 0) {
            x->parent->color = 1;
            y->color = 1;
            x->parent->parent->color = 0;
            x = x->parent->parent;
          } else {
            if (x == x->parent->left) {
              x = x->parent;
              rotate_right(x, &root, sentinel);
            }
            x->parent->color = 1;
            x->parent->parent->color = 0;
            rotate_left(x->parent->parent, &root, sentinel);
          }
        }
      }
      root->color = 1;
    }

  public:
    auto insert(const K& key, const V& value) -> std::pair<iterator, bool> {
      if (!sentinel_) return {end(), false};
      auto* end_node = sentinel_;
      auto** root = &end_node->parent;

      // Find insertion point
      node_type* parent = end_node;
      node_type* walk = *root;
      bool go_left = true;
      while (!is_nil(walk)) {
        parent = walk;
        if (key < walk->key) {
          walk = walk->left;
          go_left = true;
        } else if (walk->key < key) {
          walk = walk->right;
          go_left = false;
        } else {
          // Key already exists
          return {iterator{walk, end_node}, false};
        }
      }

      // Allocate new node
      auto* node = static_cast<node_type*>(game_heap_alloc(map_node_size));
      if (!node) return {end(), false};
      node->left = end_node;
      node->right = end_node;
      node->parent = parent;
      node->key = key;
      node->value = value;
      node->isnil = 0;

      if (parent == end_node) {
        *root = node;
      } else if (go_left) {
        parent->left = node;
      } else {
        parent->right = node;
      }

      // Update leftmost/rightmost in sentinel
      if (is_nil(end_node->left) || key < end_node->left->key) {
        end_node->left = node;
      }
      if (is_nil(end_node->right) || end_node->right->key < key) {
        end_node->right = node;
      }

      rebalance_insert(node, *root, end_node);
      *root = end_node->parent;
      (*root)->parent = end_node;

      ++size_;
      return {iterator{node, end_node}, true};
    }

    auto erase(const K& key) -> std::size_t {
      auto it = find(key);
      if (it == end()) return 0;
      erase(it);
      return 1;
    }

    auto erase(iterator pos) -> void {
      auto* z = pos.node_;
      if (!z || is_nil(z) || !sentinel_) return;
      auto* end_node = sentinel_;
      auto** root = &end_node->parent;

      auto* y = z;
      auto* x = end_node;
      auto* x_parent = end_node;
      std::uint8_t y_orig_color = y->color;

      if (is_nil(z->left)) {
        x = z->right;
        x_parent = z->parent;
        if (z == *root) {
          *root = x;
        } else if (z == z->parent->left) {
          z->parent->left = x;
        } else {
          z->parent->right = x;
        }
        if (!is_nil(x)) x->parent = z->parent;
      } else if (is_nil(z->right)) {
        x = z->left;
        x_parent = z->parent;
        if (z == *root) {
          *root = x;
        } else if (z == z->parent->left) {
          z->parent->left = x;
        } else {
          z->parent->right = x;
        }
        if (!is_nil(x)) x->parent = z->parent;
      } else {
        // Find successor (leftmost of right subtree)
        y = z->right;
        while (!is_nil(y->left)) y = y->left;
        y_orig_color = y->color;
        x = y->right;
        if (y->parent == z) {
          x_parent = y;
        } else {
          x_parent = y->parent;
          if (!is_nil(x)) x->parent = y->parent;
          y->parent->left = x;
          y->right = z->right;
          z->right->parent = y;
        }
        if (z == *root) {
          *root = y;
        } else if (z == z->parent->left) {
          z->parent->left = y;
        } else {
          z->parent->right = y;
        }
        y->parent = z->parent;
        y->color = z->color;
        y->left = z->left;
        z->left->parent = y;
        if (!is_nil(x)) x->parent = x_parent;
      }

      // Fixup if removed node was black
      if (y_orig_color == 1 && !is_nil(x)) {
        // RB-delete fixup
        while (x != *root && x->color == 1) {
          if (x == x_parent->left) {
            auto* w = x_parent->right;
            if (w->color == 0) {
              w->color = 1;
              x_parent->color = 0;
              rotate_left(x_parent, root, end_node);
              w = x_parent->right;
            }
            if (w->left->color == 1 && w->right->color == 1) {
              w->color = 0;
              x = x_parent;
              x_parent = x->parent;
            } else {
              if (w->right->color == 1) {
                w->left->color = 1;
                w->color = 0;
                rotate_right(w, root, end_node);
                w = x_parent->right;
              }
              w->color = x_parent->color;
              x_parent->color = 1;
              w->right->color = 1;
              rotate_left(x_parent, root, end_node);
              break;
            }
          } else {
            auto* w = x_parent->left;
            if (w->color == 0) {
              w->color = 1;
              x_parent->color = 0;
              rotate_right(x_parent, root, end_node);
              w = x_parent->left;
            }
            if (w->right->color == 1 && w->left->color == 1) {
              w->color = 0;
              x = x_parent;
              x_parent = x->parent;
            } else {
              if (w->left->color == 1) {
                w->right->color = 1;
                w->color = 0;
                rotate_left(w, root, end_node);
                w = x_parent->left;
              }
              w->color = x_parent->color;
              x_parent->color = 1;
              w->left->color = 1;
              rotate_right(x_parent, root, end_node);
              break;
            }
          }
        }
        x->color = 1;
      }

      // Update leftmost/rightmost
      if (end_node->left == z) {
        if (is_nil(z->right)) {
          end_node->left = z->parent;
        } else {
          auto* walk = z->right;
          while (!is_nil(walk->left)) walk = walk->left;
          end_node->left = walk;
        }
      }
      if (end_node->right == z) {
        if (is_nil(z->left)) {
          end_node->right = z->parent;
        } else {
          auto* walk = z->left;
          while (!is_nil(walk->right)) walk = walk->right;
          end_node->right = walk;
        }
      }

      if (*root != end_node) {
        (*root)->parent = end_node;
      }

      game_heap_free(z, map_node_size);
      --size_;
    }

    auto clear() -> void {
      if (!sentinel_) return;
      // Recursively free all nodes except sentinel
      auto* end_node = sentinel_;
      auto* root = end_node->parent;
      if (!is_nil(root)) {
        clear_subtree(root, end_node);
      }
      end_node->parent = end_node;
      end_node->left = end_node;
      end_node->right = end_node;
      size_ = 0;
    }

  private:
    static auto clear_subtree(node_type* node, node_type* sentinel) -> void {
      if (is_nil(node) || node == sentinel) return;
      clear_subtree(node->left, sentinel);
      clear_subtree(node->right, sentinel);
      game_heap_free(node, map_node_size);
    }

  public:
    auto operator[](const K& key) -> V& {
      if (!sentinel_) init();
      auto it = find(key);
      if (it != end()) return it.node_->value;
      auto result = insert(key, V{});
      return result.first.node_->value;
    }
  };

  using n_map_int_void = n_map<int, void*>;
  static_assert(sizeof(n_map_int_void) == 0x0C, "n_map size must be 0x0C");
  static_assert(offsetof(n_map_int_void, sentinel_) == 0x04, "n_map sentinel offset");
  static_assert(offsetof(n_map_int_void, size_) == 0x08, "n_map size offset");

  // ---------------------------------------------------------------------------
  // map_view<Key, Value> — Read-only projection over MSVC9 std::map<Key, Value>
  // Supports full 12-byte headers, head/size pointer pairs, or member offsets.
  // ---------------------------------------------------------------------------
  template<typename K, typename V>
  class map_view {
  public:
    using node_type = n_map_node<K, V>;
    using key_type = K;
    using mapped_type = V;
    using const_iterator = typename n_map<K, V>::const_iterator;

    map_view() = default;

    // 1. Factory from a full 12-byte std::map header address
    static auto from_map_addr(std::uintptr_t addr) -> map_view {
      map_view v{};
      if (!addr || !is_game_ptr(reinterpret_cast<const void*>(addr))) return v;
      v.sentinel_ = *reinterpret_cast<const node_type* const*>(addr + 4);
      v.size_ = *reinterpret_cast<const std::uint32_t*>(addr + 8);
      return v;
    }

    static auto from_object(const void* object, std::size_t offset = 0) -> map_view {
      if (!object || !is_game_ptr(object)) return map_view{};
      return from_map_addr(reinterpret_cast<std::uintptr_t>(object) + offset);
    }

    // 2. Factory from separate head pointer address and size address (e.g. g_sHashGID)
    static auto from_head_and_size_addrs(std::uintptr_t head_addr, std::uintptr_t size_addr) -> map_view {
      map_view v{};
      if (head_addr && is_game_ptr(reinterpret_cast<const void*>(head_addr))) {
        const auto* head_ptr = *reinterpret_cast<const node_type* const*>(head_addr);
        if (head_ptr && is_readable_ptr(head_ptr)) {
          v.sentinel_ = head_ptr;
          if (size_addr && is_game_ptr(reinterpret_cast<const void*>(size_addr))) {
            v.size_ = *reinterpret_cast<const std::uint32_t*>(size_addr);
          }
        }
      }
      return v;
    }

    static auto from_head_and_size(const node_type* head, std::size_t size) -> map_view {
      map_view v{};
      v.sentinel_ = head;
      v.size_ = static_cast<std::uint32_t>(size);
      return v;
    }

    [[nodiscard]] auto sentinel() const -> const node_type* { return sentinel_; }
    [[nodiscard]] auto root() const -> const node_type* {
      return (sentinel_ && !n_map<K, V>::is_nil(sentinel_->parent)) ? sentinel_->parent : nullptr;
    }

    [[nodiscard]] auto size() const -> std::size_t {
      if (!sentinel_ || !is_readable_ptr(sentinel_)) return 0;
      return size_;
    }

    [[nodiscard]] auto empty() const -> bool {
      return size() == 0;
    }

    // O(log N) Binary Search returning direct pointer to value or nullptr
    [[nodiscard]] auto find_value(const K& key) const -> V* {
      const auto* s = sentinel();
      if (!s || !is_valid_ptr(s)) return nullptr;
      const auto* curr = s->parent; // root
      std::size_t depth = 0;
      while (curr && is_valid_ptr(curr) && curr != s && !n_map<K, V>::is_nil(curr) && depth < 64) {
        ++depth;
        if (key < curr->key) {
          curr = curr->left;
        } else if (curr->key < key) {
          curr = curr->right;
        } else {
          return const_cast<V*>(&curr->value);
        }
      }
      return nullptr;
    }

    [[nodiscard]] auto contains(const K& key) const -> bool {
      return find_value(key) != nullptr;
    }

    auto begin() const -> const_iterator {
      const auto* end_node = sentinel();
      if (!end_node || n_map<K, V>::is_nil(end_node->parent)) return const_iterator{end_node, end_node};
      const auto* start = end_node->left;
      if (n_map<K, V>::is_nil(start)) {
        start = end_node->parent;
        while (!n_map<K, V>::is_nil(start->left)) start = start->left;
      }
      return const_iterator{start, end_node};
    }

    auto end() const -> const_iterator {
      return const_iterator{sentinel(), sentinel()};
    }

    // In-order traversal across all key/value pairs with early-exit
    template<typename Func>
    auto for_each(Func&& fn) const -> void {
      const auto* end_node = sentinel();
      if (!end_node || !is_valid_ptr(end_node) || n_map<K, V>::is_nil(end_node->parent)) return;

      const auto* it = end_node->left;
      if (n_map<K, V>::is_nil(it)) {
        it = end_node->parent;
        while (!n_map<K, V>::is_nil(it->left)) it = it->left;
      }

      std::size_t visited = 0;
      const std::size_t max_count = size_ + 32;

      while (it && is_valid_ptr(it) && it != end_node && !n_map<K, V>::is_nil(it) && visited < max_count) {
        ++visited;
        if constexpr (std::is_invocable_r_v<bool, Func, const K&, V>) {
          if (!fn(it->key, it->value)) break;
        } else if constexpr (std::is_invocable_r_v<bool, Func, const K&, const V&>) {
          if (!fn(it->key, it->value)) break;
        } else if constexpr (std::is_invocable_r_v<bool, Func, const K&, V&>) {
          if (!fn(it->key, const_cast<V&>(it->value))) break;
        } else {
          fn(it->key, it->value);
        }

        it = n_map<K, V>::next_node(it, end_node);
      }
    }

    template<typename Predicate>
    auto find_if(Predicate&& pred) const -> V* {
      V* result = nullptr;
      for_each([&](const K& /*k*/, V& val) -> bool {
        if (pred(val)) {
          result = &val;
          return false;
        }
        return true;
      });
      return result;
    }

  private:
    const node_type* sentinel_ = nullptr;
    std::uint32_t size_ = 0;
  };

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

    auto find(const T& key) const -> iterator {
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

    // --- Read/write operations ---

    static constexpr std::size_t set_node_size = sizeof(n_set_node<T>);

    auto init() -> void {
      allocator_ = nullptr;
      sentinel_ = static_cast<node_type*>(game_heap_alloc(set_node_size));
      if (sentinel_) {
        sentinel_->left = sentinel_;
        sentinel_->parent = sentinel_;
        sentinel_->right = sentinel_;
        sentinel_->isnil = 1;
        sentinel_->color = 1;
      }
      size_ = 0;
    }

    auto destroy() -> void {
      clear();
      if (sentinel_) {
        game_heap_free(sentinel_, set_node_size);
        sentinel_ = nullptr;
      }
      allocator_ = nullptr;
      size_ = 0;
    }

    auto insert(const T& key) -> std::pair<iterator, bool> {
      if (!sentinel_) return {end(), false};
      auto* end_node = sentinel_;
      auto** root = &end_node->parent;

      node_type* parent = end_node;
      node_type* walk = *root;
      bool go_left = true;
      while (!is_nil(walk)) {
        parent = walk;
        if (key < walk->key) {
          walk = walk->left;
          go_left = true;
        } else if (walk->key < key) {
          walk = walk->right;
          go_left = false;
        } else {
          return {iterator{walk, end_node}, false};
        }
      }

      auto* node = static_cast<node_type*>(game_heap_alloc(set_node_size));
      if (!node) return {end(), false};
      node->left = end_node;
      node->right = end_node;
      node->parent = parent;
      node->key = key;
      node->isnil = 0;

      if (parent == end_node) {
        *root = node;
      } else if (go_left) {
        parent->left = node;
      } else {
        parent->right = node;
      }

      if (is_nil(end_node->left) || key < end_node->left->key) {
        end_node->left = node;
      }
      if (is_nil(end_node->right) || end_node->right->key < key) {
        end_node->right = node;
      }

      // Rebalance (same RB-tree logic as n_map)
      node->color = 0;
      auto* x = node;
      while (x != *root && x->parent->color == 0) {
        if (x->parent == x->parent->parent->left) {
          auto* y = x->parent->parent->right;
          if (y->color == 0) {
            x->parent->color = 1; y->color = 1;
            x->parent->parent->color = 0;
            x = x->parent->parent;
          } else {
            if (x == x->parent->right) {
              x = x->parent;
              // rotate_left
              auto* rl_y = x->right;
              x->right = rl_y->left;
              if (!is_nil(rl_y->left)) rl_y->left->parent = x;
              rl_y->parent = x->parent;
              if (x == *root) *root = rl_y;
              else if (x == x->parent->left) x->parent->left = rl_y;
              else x->parent->right = rl_y;
              rl_y->left = x; x->parent = rl_y;
            }
            x->parent->color = 1;
            x->parent->parent->color = 0;
            // rotate_right
            auto* rr_x = x->parent->parent;
            auto* rr_y = rr_x->left;
            rr_x->left = rr_y->right;
            if (!is_nil(rr_y->right)) rr_y->right->parent = rr_x;
            rr_y->parent = rr_x->parent;
            if (rr_x == *root) *root = rr_y;
            else if (rr_x == rr_x->parent->right) rr_x->parent->right = rr_y;
            else rr_x->parent->left = rr_y;
            rr_y->right = rr_x; rr_x->parent = rr_y;
          }
        } else {
          auto* y = x->parent->parent->left;
          if (y->color == 0) {
            x->parent->color = 1; y->color = 1;
            x->parent->parent->color = 0;
            x = x->parent->parent;
          } else {
            if (x == x->parent->left) {
              x = x->parent;
              // rotate_right
              auto* rr_y = x->left;
              x->left = rr_y->right;
              if (!is_nil(rr_y->right)) rr_y->right->parent = x;
              rr_y->parent = x->parent;
              if (x == *root) *root = rr_y;
              else if (x == x->parent->right) x->parent->right = rr_y;
              else x->parent->left = rr_y;
              rr_y->right = x; x->parent = rr_y;
            }
            x->parent->color = 1;
            x->parent->parent->color = 0;
            // rotate_left
            auto* rl_x = x->parent->parent;
            auto* rl_y = rl_x->right;
            rl_x->right = rl_y->left;
            if (!is_nil(rl_y->left)) rl_y->left->parent = rl_x;
            rl_y->parent = rl_x->parent;
            if (rl_x == *root) *root = rl_y;
            else if (rl_x == rl_x->parent->left) rl_x->parent->left = rl_y;
            else rl_x->parent->right = rl_y;
            rl_y->left = rl_x; rl_x->parent = rl_y;
          }
        }
      }
      (*root)->color = 1;
      *root = end_node->parent;
      (*root)->parent = end_node;

      ++size_;
      return {iterator{node, end_node}, true};
    }

    auto erase(const T& key) -> std::size_t {
      auto it = find(key);
      if (it == end()) return 0;
      // Simple erase: swap with successor, free
      auto* z = it.node_;
      if (!z || is_nil(z) || !sentinel_) return 0;
      auto* end_node = sentinel_;

      // Find successor if both children exist
      if (!is_nil(z->left) && !is_nil(z->right)) {
        auto* succ = z->right;
        while (!is_nil(succ->left)) succ = succ->left;
        // Move successor's key to z, then z = succ
        z->key = succ->key;
        z = succ;
      }

      // Now z has at most one non-nil child
      auto* child = is_nil(z->left) ? z->right : z->left;
      if (z == z->parent->left) z->parent->left = child;
      else z->parent->right = child;
      if (!is_nil(child)) child->parent = z->parent;

      if (z == end_node->parent) {
        end_node->parent = child;
      }
      if (z == end_node->left) {
        if (!is_nil(child)) {
          auto* walk = child;
          while (!is_nil(walk->left)) walk = walk->left;
          end_node->left = walk;
        } else {
          end_node->left = z->parent;
        }
      }
      if (z == end_node->right) {
        if (!is_nil(child)) {
          auto* walk = child;
          while (!is_nil(walk->right)) walk = walk->right;
          end_node->right = walk;
        } else {
          end_node->right = z->parent;
        }
      }

      if (!is_nil(end_node->parent)) {
        end_node->parent->parent = end_node;
      }

      game_heap_free(z, set_node_size);
      --size_;
      return 1;
    }

    auto clear() -> void {
      if (!sentinel_) return;
      auto* end_node = sentinel_;
      auto* root = end_node->parent;
      if (!is_nil(root)) {
        clear_subtree(root, end_node);
      }
      end_node->parent = end_node;
      end_node->left = end_node;
      end_node->right = end_node;
      size_ = 0;
    }

  private:
    static auto clear_subtree(node_type* node, node_type* sentinel) -> void {
      if (is_nil(node) || node == sentinel) return;
      clear_subtree(node->left, sentinel);
      clear_subtree(node->right, sentinel);
      game_heap_free(node, set_node_size);
    }
  };

  // ---------------------------------------------------------------------------
  // MSVC 2008 std::set<T> view (read-only projection over live game memory)
  // ---------------------------------------------------------------------------
  template<typename T>
  class set_view {
  public:
    using node_type = n_set_node<T>;
    using key_type = T;
    using value_type = T;
    using const_iterator = typename n_set<T>::const_iterator;

    set_view() = default;

    // 1. Factory from a full 12-byte std::set header address
    static auto from_set_addr(std::uintptr_t addr) -> set_view {
      set_view v{};
      if (!addr || !is_game_ptr(reinterpret_cast<const void*>(addr))) return v;
      v.sentinel_ = *reinterpret_cast<const node_type* const*>(addr + 4);
      v.size_ = *reinterpret_cast<const std::uint32_t*>(addr + 8);
      return v;
    }

    static auto from_object(const void* object, std::size_t offset = 0) -> set_view {
      if (!object || !is_game_ptr(object)) return set_view{};
      return from_set_addr(reinterpret_cast<std::uintptr_t>(object) + offset);
    }

    // 2. Factory from separate head pointer address and size address
    static auto from_head_and_size_addrs(std::uintptr_t head_addr, std::uintptr_t size_addr) -> set_view {
      set_view v{};
      if (head_addr && is_game_ptr(reinterpret_cast<const void*>(head_addr))) {
        const auto* head_ptr = *reinterpret_cast<const node_type* const*>(head_addr);
        if (head_ptr && is_readable_ptr(head_ptr)) {
          v.sentinel_ = head_ptr;
          if (size_addr && is_game_ptr(reinterpret_cast<const void*>(size_addr))) {
            v.size_ = *reinterpret_cast<const std::uint32_t*>(size_addr);
          }
        }
      }
      return v;
    }

    static auto from_head_and_size(const node_type* head, std::size_t size) -> set_view {
      set_view v{};
      v.sentinel_ = head;
      v.size_ = static_cast<std::uint32_t>(size);
      return v;
    }

    [[nodiscard]] auto sentinel() const -> const node_type* { return sentinel_; }
    [[nodiscard]] auto root() const -> const node_type* {
      return (sentinel_ && !n_set<T>::is_nil(sentinel_->parent)) ? sentinel_->parent : nullptr;
    }

    [[nodiscard]] auto size() const -> std::size_t {
      if (!sentinel_ || !is_readable_ptr(sentinel_)) return 0;
      return size_;
    }

    [[nodiscard]] auto empty() const -> bool {
      return size() == 0;
    }

    // O(log N) Binary Search returning direct pointer to element or nullptr
    [[nodiscard]] auto find_element(const T& key) const -> const T* {
      const auto* s = sentinel();
      if (!s || !is_valid_ptr(s)) return nullptr;
      const auto* curr = s->parent; // root
      std::size_t depth = 0;
      while (curr && is_valid_ptr(curr) && curr != s && !n_set<T>::is_nil(curr) && depth < 64) {
        ++depth;
        if (key < curr->key) {
          curr = curr->left;
        } else if (curr->key < key) {
          curr = curr->right;
        } else {
          return &curr->key;
        }
      }
      return nullptr;
    }

    [[nodiscard]] auto contains(const T& key) const -> bool {
      return find_element(key) != nullptr;
    }

    auto begin() const -> const_iterator {
      const auto* end_node = sentinel();
      if (!end_node || n_set<T>::is_nil(end_node->parent)) {
        return const_iterator{const_cast<node_type*>(end_node), const_cast<node_type*>(end_node)};
      }
      const auto* start = end_node->left;
      if (n_set<T>::is_nil(start)) {
        start = end_node->parent;
        while (!n_set<T>::is_nil(start->left)) start = start->left;
      }
      return const_iterator{const_cast<node_type*>(start), const_cast<node_type*>(end_node)};
    }

    auto end() const -> const_iterator {
      return const_iterator{const_cast<node_type*>(sentinel()), const_cast<node_type*>(sentinel())};
    }

    // In-order traversal across all keys with early-exit
    template<typename Func>
    auto for_each(Func&& fn) const -> void {
      const auto* end_node = sentinel();
      if (!end_node || !is_valid_ptr(end_node) || n_set<T>::is_nil(end_node->parent)) return;

      const auto* it = end_node->left;
      if (n_set<T>::is_nil(it)) {
        it = end_node->parent;
        while (!n_set<T>::is_nil(it->left)) it = it->left;
      }

      std::size_t visited = 0;
      const std::size_t max_count = size_ + 32;

      while (it && is_valid_ptr(it) && it != end_node && !n_set<T>::is_nil(it) && visited < max_count) {
        ++visited;
        if constexpr (std::is_invocable_r_v<bool, Func, const T&>) {
          if (!fn(it->key)) break;
        } else {
          fn(it->key);
        }

        it = n_set<T>::next_node(it, end_node);
      }
    }

    template<typename Predicate>
    auto find_if(Predicate&& pred) const -> const T* {
      const T* result = nullptr;
      for_each([&](const T& key) -> bool {
        if (pred(key)) {
          result = &key;
          return false;
        }
        return true;
      });
      return result;
    }

  private:
    const node_type* sentinel_ = nullptr;
    std::uint32_t size_ = 0;
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

    // --- Read/write operations ---

    static constexpr std::size_t hash_node_size = sizeof(n_hash_node<K, V>);

    auto init() -> void {
      traits_ = nullptr;
      list_alloc_ = nullptr;
      list_sentinel_ = static_cast<node_type*>(game_heap_alloc(hash_node_size));
      if (list_sentinel_) {
        list_sentinel_->next = list_sentinel_;
        list_sentinel_->prev = list_sentinel_;
      }
      list_size_ = 0;
      vec_alloc_ = nullptr;
      vec_first_ = nullptr;
      vec_last_ = nullptr;
      vec_end_ = nullptr;
      mask_ = 0;
      maxidx_ = 0;
    }

    auto destroy() -> void {
      clear();
      if (list_sentinel_) {
        game_heap_free(list_sentinel_, hash_node_size);
        list_sentinel_ = nullptr;
      }
      if (vec_first_) {
        const std::size_t cap = vec_end_ ? static_cast<std::size_t>(
          reinterpret_cast<const std::uint8_t*>(vec_end_) -
          reinterpret_cast<const std::uint8_t*>(vec_first_)) / sizeof(void*) : 0;
        game_heap_free(vec_first_, cap * sizeof(void*));
        vec_first_ = nullptr;
        vec_last_ = nullptr;
        vec_end_ = nullptr;
      }
      list_size_ = 0;
      mask_ = 0;
      maxidx_ = 0;
    }

    auto find(const K& key) -> iterator {
      if (!list_sentinel_ || list_size_ == 0) return end();
      auto* node = list_sentinel_->next;
      while (node && node != list_sentinel_) {
        if (node->first == key) return iterator{node};
        node = node->next;
      }
      return end();
    }

    auto find(const K& key) const -> const_iterator {
      if (!list_sentinel_ || list_size_ == 0) return end();
      const auto* node = list_sentinel_->next;
      while (node && node != list_sentinel_) {
        if (node->first == key) return const_iterator{node};
        node = node->next;
      }
      return end();
    }

    auto insert(const K& key, const V& value) -> std::pair<iterator, bool> {
      if (!list_sentinel_) return {end(), false};
      // Check if key already exists
      auto existing = find(key);
      if (existing != end()) return {existing, false};

      auto* node = static_cast<node_type*>(game_heap_alloc(hash_node_size));
      if (!node) return {end(), false};
      node->first = key;
      node->second = value;

      // Insert at front of list
      node->next = list_sentinel_->next;
      node->prev = list_sentinel_;
      list_sentinel_->next->prev = node;
      list_sentinel_->next = node;

      ++list_size_;
      return {iterator{node}, true};
    }

    auto erase(const K& key) -> std::size_t {
      auto it = find(key);
      if (it == end()) return 0;
      auto* node = it.node_;
      node->prev->next = node->next;
      node->next->prev = node->prev;
      game_heap_free(node, hash_node_size);
      --list_size_;
      return 1;
    }

    auto clear() -> void {
      if (!list_sentinel_) return;
      auto* node = list_sentinel_->next;
      while (node && node != list_sentinel_) {
        auto* next = node->next;
        game_heap_free(node, hash_node_size);
        node = next;
      }
      list_sentinel_->next = list_sentinel_;
      list_sentinel_->prev = list_sentinel_;
      list_size_ = 0;
    }
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
  // MSVC 2008 std::vector<T> view (read-only projection over live game memory)
  // ---------------------------------------------------------------------------
  template<typename T>
  class vector_view {
  public:
    using value_type = T;
    using const_iterator = const T*;

    vector_view() = default;

    static auto from(const void* object) -> vector_view {
      vector_view result;
      if (!object) return result;
      result.object_ = object;
      result.first_ = *reinterpret_cast<const T* const*>(static_cast<const std::uint8_t*>(object) + 4);
      result.last_ = *reinterpret_cast<const T* const*>(static_cast<const std::uint8_t*>(object) + 8);
      result.end_cap_ = *reinterpret_cast<const T* const*>(static_cast<const std::uint8_t*>(object) + 12);
      return result;
    }

    static auto from_field(const void* object, std::size_t offset = 0) -> vector_view {
      if (!object || !is_game_ptr(object)) return vector_view{};
      return from(static_cast<const std::uint8_t*>(object) + offset);
    }

    static auto from_pointers(const T* first, const T* last, const T* end_cap = nullptr) -> vector_view {
      vector_view result;
      if (first && last && last >= first) {
        result.first_ = first;
        result.last_ = last;
        result.end_cap_ = end_cap ? end_cap : last;
      }
      return result;
    }

    auto object() const -> const void* { return object_; }
    auto data() const -> const T* { return first_; }

    auto size() const -> std::size_t {
      if (!first_ || !last_ || last_ < first_) {
        return 0;
      }
      return static_cast<std::size_t>(last_ - first_);
    }

    auto empty() const -> bool { return size() == 0; }

    auto capacity() const -> std::size_t {
      if (end_cap_ && first_ && end_cap_ >= first_) {
        return static_cast<std::size_t>(end_cap_ - first_);
      }
      return size();
    }

    auto safe_at(std::size_t index) const -> const T* {
      if (index >= size()) {
        return nullptr;
      }
      return &first_[index];
    }

    auto operator[](std::size_t index) const -> const T& { return first_[index]; }

    auto begin() const -> const T* { return first_; }
    auto end() const -> const T* { return last_; }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void {
      const auto* b = data();
      const auto* e = end();
      if (!b || !e || e < b) {
        return;
      }
      for (const auto* cursor = b; cursor < e; ++cursor) {
        if constexpr (std::is_invocable_r_v<bool, Fn, const T&>) {
          if (!fn(*cursor)) break;
        } else {
          fn(*cursor);
        }
      }
    }

    template<typename Predicate>
    auto find_if(Predicate&& pred) const -> const T* {
      const auto* b = data();
      const auto* e = end();
      if (!b || !e || e < b) {
        return nullptr;
      }
      for (const auto* cursor = b; cursor < e; ++cursor) {
        if (pred(*cursor)) {
          return cursor;
        }
      }
      return nullptr;
    }

  private:
    const void* object_ = nullptr;
    const T* first_ = nullptr;
    const T* last_ = nullptr;
    const T* end_cap_ = nullptr;
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

    // --- Read/write operations ---

    auto init() -> void {
      allocator_ = nullptr;
      first_ = nullptr;
      last_ = nullptr;
      end_ = nullptr;
    }

    auto destroy() -> void {
      if (first_) {
        const std::size_t cap = static_cast<std::size_t>(end_ - first_);
        game_heap_free(first_, cap * sizeof(T));
      }
      first_ = nullptr;
      last_ = nullptr;
      end_ = nullptr;
      allocator_ = nullptr;
    }

    auto reserve(std::size_t new_cap) -> void {
      const std::size_t cur_cap = end_ ? static_cast<std::size_t>(end_ - first_) : 0;
      if (new_cap <= cur_cap) return;
      auto* grown = static_cast<T*>(game_heap_alloc(new_cap * sizeof(T)));
      if (!grown) return;
      const std::size_t cur_size = last_ ? static_cast<std::size_t>(last_ - first_) : 0;
      if (first_ && cur_size > 0) {
        std::memcpy(grown, first_, cur_size * sizeof(T));
        game_heap_free(first_, cur_cap * sizeof(T));
      }
      first_ = grown;
      last_ = grown + cur_size;
      end_ = grown + new_cap;
    }

    auto push_back(const T& value) -> void {
      if (!first_ || last_ >= end_) {
        const std::size_t cur_cap = end_ ? static_cast<std::size_t>(end_ - first_) : 0;
        const std::size_t new_cap = cur_cap == 0 ? 4 : cur_cap * 2;
        reserve(new_cap);
        if (!first_) return;
      }
      *last_ = value;
      ++last_;
    }

    auto pop_back() -> void {
      if (first_ && last_ > first_) {
        --last_;
      }
    }

    auto clear() -> void {
      last_ = first_;
    }

    auto insert(iterator pos, const T& value) -> iterator {
      if (!first_) {
        push_back(value);
        return first_;
      }
      if (pos < first_ || pos > last_) return last_;
      const std::size_t idx = static_cast<std::size_t>(pos - first_);
      if (last_ >= end_) {
        const std::size_t cur_cap = static_cast<std::size_t>(end_ - first_);
        const std::size_t new_cap = cur_cap == 0 ? 4 : cur_cap * 2;
        reserve(new_cap);
        if (!first_) return last_;
      }
      const std::size_t tail = static_cast<std::size_t>(last_ - (first_ + idx));
      if (tail > 0) {
        std::memmove(first_ + idx + 1, first_ + idx, tail * sizeof(T));
      }
      first_[idx] = value;
      ++last_;
      return first_ + idx;
    }

    auto erase(iterator pos) -> iterator {
      if (!first_ || pos < first_ || pos >= last_) return last_;
      const std::size_t tail = static_cast<std::size_t>(last_ - (pos + 1));
      if (tail > 0) {
        std::memmove(pos, pos + 1, tail * sizeof(T));
      }
      --last_;
      return pos;
    }

    auto resize(std::size_t new_size) -> void {
      const std::size_t cur_size = first_ ? static_cast<std::size_t>(last_ - first_) : 0;
      if (new_size > cur_size) {
        if (new_size > (end_ ? static_cast<std::size_t>(end_ - first_) : 0)) {
          reserve(new_size);
        }
        if (first_) {
          for (std::size_t i = cur_size; i < new_size; ++i) {
            first_[i] = T{};
          }
          last_ = first_ + new_size;
        }
      } else if (new_size < cur_size && first_) {
        last_ = first_ + new_size;
      }
    }
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

  // ---------------------------------------------------------------------------
  // Owned RAII wrappers — construct on init, destroy on destruct, game-heap backed
  // ---------------------------------------------------------------------------

  template<typename T>
  class own_list {
  public:
    using value_type = T;
    using iterator = typename n_list<T>::iterator;
    using const_iterator = typename n_list<T>::const_iterator;

    own_list() { impl_.init(); }
    ~own_list() { impl_.destroy(); }

    own_list(const own_list&) = delete;
    auto operator=(const own_list&) = delete;

    own_list(own_list&& other) noexcept {
      impl_.init();
      // Move elements
      while (!other.empty()) {
        impl_.push_back(other.front());
        other.pop_front();
      }
    }
    auto operator=(own_list&& other) noexcept -> own_list& {
      impl_.destroy();
      impl_.init();
      while (!other.empty()) {
        impl_.push_back(other.front());
        other.pop_front();
      }
      return *this;
    }

    auto raw() -> n_list<T>* { return &impl_; }
    auto raw() const -> const n_list<T>* { return &impl_; }
    auto empty() const -> bool { return impl_.empty(); }
    auto size() const -> std::size_t { return impl_.size(); }
    auto push_back(const T& v) -> void { impl_.push_back(v); }
    auto push_front(const T& v) -> void { impl_.push_front(v); }
    auto pop_back() -> void { impl_.pop_back(); }
    auto pop_front() -> void { impl_.pop_front(); }
    auto front() -> T& { return impl_.sentinel_->_next->_myval; }
    auto front() const -> const T& { return impl_.sentinel_->_next->_myval; }
    auto back() -> T& { return impl_.sentinel_->_prev->_myval; }
    auto back() const -> const T& { return impl_.sentinel_->_prev->_myval; }
    auto clear() -> void { impl_.clear(); }
    auto erase(iterator it) -> void { impl_.erase(it); }
    auto begin() -> iterator { return impl_.begin(); }
    auto begin() const -> const_iterator { return impl_.begin(); }
    auto end() -> iterator { return impl_.end(); }
    auto end() const -> const_iterator { return impl_.end(); }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void { impl_.for_each(std::forward<Fn>(fn)); }

  private:
    n_list<T> impl_;
  };

  template<typename T>
  class own_vector {
  public:
    using value_type = T;
    using iterator = typename n_vector<T>::iterator;
    using const_iterator = typename n_vector<T>::const_iterator;

    own_vector() { impl_.init(); }
    ~own_vector() { impl_.destroy(); }

    own_vector(const own_vector&) = delete;
    auto operator=(const own_vector&) = delete;

    own_vector(own_vector&& other) noexcept {
      impl_.init();
      for (std::size_t i = 0; i < other.size(); ++i) {
        impl_.push_back(other[i]);
      }
      other.destroy();
      other.impl_.init();
    }
    auto operator=(own_vector&& other) noexcept -> own_vector& {
      impl_.destroy();
      impl_.init();
      for (std::size_t i = 0; i < other.size(); ++i) {
        impl_.push_back(other[i]);
      }
      other.destroy();
      other.impl_.init();
      return *this;
    }

    auto raw() -> n_vector<T>* { return &impl_; }
    auto raw() const -> const n_vector<T>* { return &impl_; }
    auto empty() const -> bool { return impl_.empty(); }
    auto size() const -> std::size_t { return impl_.size(); }
    auto capacity() const -> std::size_t {
      return impl_.end_ptr() ? static_cast<std::size_t>(impl_.end_ptr() - impl_.first()) : 0;
    }
    auto data() -> T* { return impl_.first(); }
    auto data() const -> const T* { return impl_.first(); }
    auto push_back(const T& v) -> void { impl_.push_back(v); }
    auto pop_back() -> void { impl_.pop_back(); }
    auto clear() -> void { impl_.clear(); }
    auto reserve(std::size_t n) -> void { impl_.reserve(n); }
    auto resize(std::size_t n) -> void { impl_.resize(n); }
    auto insert(iterator pos, const T& v) -> iterator { return impl_.insert(pos, v); }
    auto erase(iterator pos) -> iterator { return impl_.erase(pos); }
    auto operator[](std::size_t i) -> T& { return impl_[i]; }
    auto operator[](std::size_t i) const -> const T& { return impl_[i]; }
    auto front() -> T& { return impl_.front(); }
    auto front() const -> const T& { return impl_.front(); }
    auto back() -> T& { return impl_.back(); }
    auto back() const -> const T& { return impl_.back(); }
    auto begin() -> iterator { return impl_.begin(); }
    auto begin() const -> const_iterator { return impl_.begin(); }
    auto end() -> iterator { return impl_.end(); }
    auto end() const -> const_iterator { return impl_.end(); }

    template<typename Fn>
    auto for_each(Fn&& fn) const -> void {
      for (const auto& v : *this) fn(v);
    }

  private:
    n_vector<T> impl_;
  };

  template<typename K, typename V>
  class own_map {
  public:
    using key_type = K;
    using mapped_type = V;
    using iterator = typename n_map<K, V>::iterator;
    using const_iterator = typename n_map<K, V>::const_iterator;

    own_map() { impl_.init(); }
    ~own_map() { impl_.destroy(); }

    own_map(const own_map&) = delete;
    auto operator=(const own_map&) = delete;

    own_map(own_map&& other) noexcept {
      impl_.init();
      other.for_each([&](const K& k, const V& v) { impl_.insert(k, v); });
      other.clear();
    }
    auto operator=(own_map&& other) noexcept -> own_map& {
      impl_.destroy();
      impl_.init();
      other.for_each([&](const K& k, const V& v) { impl_.insert(k, v); });
      other.clear();
      return *this;
    }

    auto raw() -> n_map<K, V>* { return &impl_; }
    auto raw() const -> const n_map<K, V>* { return &impl_; }
    auto empty() const -> bool { return impl_.empty(); }
    auto size() const -> std::uint32_t { return impl_.size(); }
    auto insert(const K& k, const V& v) -> std::pair<iterator, bool> { return impl_.insert(k, v); }
    auto erase(const K& k) -> std::size_t { return impl_.erase(k); }
    auto erase(iterator it) -> void { impl_.erase(it); }
    auto clear() -> void { impl_.clear(); }
    auto find(const K& k) -> iterator { return impl_.find(k); }
    auto find(const K& k) const -> const_iterator { return impl_.find(k); }
    auto operator[](const K& k) -> V& { return impl_[k]; }
    auto begin() -> iterator { return impl_.begin(); }
    auto begin() const -> const_iterator { return impl_.begin(); }
    auto end() -> iterator { return impl_.end(); }
    auto end() const -> const_iterator { return impl_.end(); }

    template<typename Fn>
    auto for_each(Fn&& fn) const { return impl_.for_each(std::forward<Fn>(fn)); }

  private:
    n_map<K, V> impl_;
  };

  template<typename T>
  class own_set {
  public:
    using key_type = T;
    using value_type = T;
    using iterator = typename n_set<T>::iterator;

    own_set() { impl_.init(); }
    ~own_set() { impl_.destroy(); }

    own_set(const own_set&) = delete;
    auto operator=(const own_set&) = delete;

    own_set(own_set&& other) noexcept {
      impl_.init();
      other.for_each([&](const T& k) { impl_.insert(k); });
      other.clear();
    }
    auto operator=(own_set&& other) noexcept -> own_set& {
      impl_.destroy();
      impl_.init();
      other.for_each([&](const T& k) { impl_.insert(k); });
      other.clear();
      return *this;
    }

    auto raw() -> n_set<T>* { return &impl_; }
    auto raw() const -> const n_set<T>* { return &impl_; }
    auto empty() const -> bool { return impl_.empty(); }
    auto size() const -> std::uint32_t { return impl_.size(); }
    auto insert(const T& k) -> std::pair<iterator, bool> { return impl_.insert(k); }
    auto erase(const T& k) -> std::size_t { return impl_.erase(k); }
    auto clear() -> void { impl_.clear(); }
    auto find(const T& k) const -> iterator { return impl_.find(k); }
    auto begin() const -> iterator { return impl_.begin(); }
    auto end() const -> iterator { return impl_.end(); }

    template<typename Fn>
    auto for_each(Fn&& fn) const {
      for (const auto& v : *this) fn(v);
    }

  private:
    n_set<T> impl_;
  };

  template<typename K, typename V>
  class own_hash_map {
  public:
    using key_type = K;
    using mapped_type = V;
    using iterator = typename n_hash_map<K, V>::iterator;
    using const_iterator = typename n_hash_map<K, V>::const_iterator;

    own_hash_map() { impl_.init(); }
    ~own_hash_map() { impl_.destroy(); }

    own_hash_map(const own_hash_map&) = delete;
    auto operator=(const own_hash_map&) = delete;

    own_hash_map(own_hash_map&& other) noexcept {
      impl_.init();
      other.for_each([&](const K& k, const V& v) { impl_.insert(k, v); });
      other.clear();
    }
    auto operator=(own_hash_map&& other) noexcept -> own_hash_map& {
      impl_.destroy();
      impl_.init();
      other.for_each([&](const K& k, const V& v) { impl_.insert(k, v); });
      other.clear();
      return *this;
    }

    auto raw() -> n_hash_map<K, V>* { return &impl_; }
    auto raw() const -> const n_hash_map<K, V>* { return &impl_; }
    auto empty() const -> bool { return impl_.empty(); }
    auto size() const -> std::size_t { return impl_.size(); }
    auto insert(const K& k, const V& v) -> std::pair<iterator, bool> { return impl_.insert(k, v); }
    auto erase(const K& k) -> std::size_t { return impl_.erase(k); }
    auto clear() -> void { impl_.clear(); }
    auto find(const K& k) -> iterator { return impl_.find(k); }
    auto find(const K& k) const -> const_iterator { return impl_.find(k); }
    auto begin() -> iterator { return impl_.begin(); }
    auto begin() const -> const_iterator { return impl_.begin(); }
    auto end() -> iterator { return impl_.end(); }
    auto end() const -> const_iterator { return impl_.end(); }

    template<typename Fn>
    auto for_each(Fn&& fn) const {
      for (auto it = impl_.begin(); it != impl_.end(); ++it) {
        fn(it.node_->first, it.node_->second);
      }
    }

  private:
    n_hash_map<K, V> impl_;
  };
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

  // Non-owning view abstractions
  template<typename K, typename V>
  using map_view = ext_client::msvc9::map_view<K, V>;

  template<typename T>
  using set_view = ext_client::msvc9::set_view<T>;

  template<typename T>
  using vector_view = ext_client::msvc9::vector_view<T>;

  // Owned RAII wrappers
  template<typename T>
  using own_list = ext_client::msvc9::own_list<T>;

  template<typename T>
  using own_vector = ext_client::msvc9::own_vector<T>;

  template<typename K, typename V>
  using own_map = ext_client::msvc9::own_map<K, V>;

  template<typename T>
  using own_set = ext_client::msvc9::own_set<T>;

  template<typename K, typename V>
  using own_hash_map = ext_client::msvc9::own_hash_map<K, V>;
}
