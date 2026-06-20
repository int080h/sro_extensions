#include "utils/msvc9_stl.hpp"

#include "utils/offsets.hpp"

#include <cstddef>
#include <cstring>
#include <cwchar>

#include <windows.h>

namespace {

  using ext_client::offsets::as_fn;

  auto game_wstring_assign(void* self, const wchar_t* src, std::uint32_t char_count) -> void {
    using assign_fn = void*(__thiscall*)(void*, const wchar_t*, unsigned);
    as_fn<assign_fn>(ext_client::offsets::msvc9_stl::functions::wstring_assign)(self, src, char_count);
  }

  auto game_wstring_clear(void* self) -> void {
    auto* words = static_cast<std::uint32_t*>(self);
    if (words[6] >= ext_client::msvc9::wstring_sso_capacity + 1) {
      using free_fn = void(__cdecl*)(int*, unsigned);
      as_fn<free_fn>(ext_client::offsets::msvc9_stl::functions::heap_free)(reinterpret_cast<int*>(words[1]), 2u * words[6] + 2u);
    }
    words[5] = 0;
    words[6] = ext_client::msvc9::wstring_sso_capacity;
    *reinterpret_cast<wchar_t*>(static_cast<std::uint8_t*>(self) + 4) = L'\0';
  }

  auto game_string_assign(void* self, const char* src, std::size_t byte_count) -> void {
    using assign_fn = void*(__thiscall*)(void*, char*, unsigned);
    as_fn<assign_fn>(
      ext_client::offsets::msvc9_stl::functions::string_assign)(self, const_cast<char*>(src), static_cast<unsigned>(byte_count));
  }

  auto game_string_clear(void* self) -> void {
    using clear_fn = void(__thiscall*)(void*);
    as_fn<clear_fn>(ext_client::offsets::msvc9_stl::functions::string_clear)(self);
  }

  auto game_map_construct(void* self) -> void {
    using ctor_fn = void(__thiscall*)(void*);
    as_fn<ctor_fn>(ext_client::offsets::msvc9_stl::functions::ui_res_map_ctor)(self);
  }

  auto game_map_clear_tree(void* root_node) -> void {
    using clear_fn = void(__stdcall*)(int*);
    as_fn<clear_fn>(ext_client::offsets::msvc9_stl::functions::map_tree_clear)(static_cast<int*>(root_node));
  }

  auto game_map_find(void* map, std::uint32_t key, bool add_base_key = false) -> void* {
    using find_fn = int(__thiscall*)(void*, int, int);
    const int result =
      as_fn<find_fn>(ext_client::offsets::msvc9_stl::functions::ui_res_map_find)(map, static_cast<int>(key), add_base_key ? 1 : 0);
    return reinterpret_cast<void*>(result);
  }

  auto map_sentinel(void* map) -> ext_client::msvc9::map_tree_node* {
    return *reinterpret_cast<ext_client::msvc9::map_tree_node**>(static_cast<std::uint8_t*>(map) + 4);
  }

  auto map_reset_sentinel(void* map) -> void {
    auto* sentinel = map_sentinel(map);
    if (!sentinel) {
      return;
    }
    sentinel->parent = sentinel;
    sentinel->left = sentinel;
    sentinel->right = sentinel;
    *reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(map) + 8) = 0;
  }

  auto map_tree_next(ext_client::msvc9::map_tree_node* node, const ext_client::msvc9::map_tree_node* end)
    -> ext_client::msvc9::map_tree_node* {
    if (!node || !end) {
      return const_cast<ext_client::msvc9::map_tree_node*>(end);
    }
    if (node->right && !node->right->is_nil()) {
      auto* walk = node->right;
      while (walk->left && !walk->left->is_nil()) {
        walk = walk->left;
      }
      return walk;
    }
    auto* parent = node->parent;
    while (parent && parent != end && node == parent->right) {
      node = parent;
      parent = parent->parent;
    }
    return parent;
  }

  auto map_walk_set(ext_client::msvc9::map_tree_node* end, std::uint32_t key, void* value) -> bool {
    if (!end) {
      return false;
    }
    for (auto* node = end->parent; node && node != end; node = map_tree_next(node, end)) {
      if (node->is_nil()) {
        continue;
      }
      if (node->key == key) {
        node->value = value;
        return true;
      }
    }
    return false;
  }

} // namespace

namespace ext_client::msvc9 {

  auto game_heap_alloc(std::size_t bytes) -> void* {
    using alloc_fn = void*(__cdecl*)(unsigned);
    return as_fn<alloc_fn>(ext_client::offsets::msvc9_stl::functions::heap_alloc_raw)(static_cast<unsigned>(bytes));
  }

  auto game_heap_free(void* block, std::size_t bytes) -> void {
    if (!block) {
      return;
    }
    using free_fn = void(__cdecl*)(int*, unsigned);
    as_fn<free_fn>(ext_client::offsets::msvc9_stl::functions::heap_free)(static_cast<int*>(block), static_cast<unsigned>(bytes));
  }

  auto is_readable_ptr(const void* ptr) -> bool {
    if (!is_game_ptr(ptr)) {
      return false;
    }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(ptr, &info, sizeof(info)) != sizeof(info)) {
      return false;
    }
    return info.State == MEM_COMMIT && (info.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
  }

  // ---------------------------------------------------------------------------
  // wstring_ref
  // ---------------------------------------------------------------------------

  auto wstring_ref::from(const void* object) -> wstring_ref {
    wstring_ref view{};
    view.object_ = object;
    return view;
  }

  auto wstring_ref::capacity() const -> std::uint32_t {
    if (!object_) {
      return 0;
    }
    return static_cast<const std::uint32_t*>(object_)[6];
  }

  auto wstring_ref::length() const -> std::uint32_t {
    if (!object_) {
      return 0;
    }
    return static_cast<const std::uint32_t*>(object_)[5];
  }

  auto wstring_ref::data() const -> const wchar_t* {
    if (!object_) {
      return L"";
    }
    if (capacity() < wstring_sso_capacity + 1) {
      return reinterpret_cast<const wchar_t*>(static_cast<const char*>(object_) + 4);
    }
    return *reinterpret_cast<const wchar_t* const*>(static_cast<const char*>(object_) + 4);
  }

  auto wstring_ref::empty() const -> bool {
    return length() == 0;
  }

  auto wstring_ref::copy_to(wchar_t* dst, std::size_t dst_count) const -> bool {
    if (!dst || dst_count == 0) {
      return false;
    }
    dst[0] = L'\0';
    const wchar_t* src = data();
    const std::uint32_t len = length();
    if (len + 1 > dst_count) {
      return false;
    }
    for (std::uint32_t i = 0; i <= len; ++i) {
      dst[i] = src[i];
    }
    return true;
  }

  // ---------------------------------------------------------------------------
  // string_ref
  // ---------------------------------------------------------------------------

  auto string_ref::from(const void* object) -> string_ref {
    string_ref view{};
    view.object_ = object;
    return view;
  }

  auto string_ref::capacity() const -> std::uint32_t {
    if (!object_) {
      return 0;
    }
    // VC++ 2005 basic_string<char>: _Mysize @ +20, _Myres @ +24 (28-byte object).
    return static_cast<const std::uint32_t*>(object_)[6];
  }

  auto string_ref::length() const -> std::uint32_t {
    if (!object_) {
      return 0;
    }
    return static_cast<const std::uint32_t*>(object_)[5];
  }

  auto string_ref::data() const -> const char* {
    if (!object_) {
      return "";
    }
    if (capacity() < string_sso_capacity + 1) {
      return static_cast<const char*>(object_) + 4;
    }
    return *reinterpret_cast<const char* const*>(static_cast<const char*>(object_) + 4);
  }

  auto string_ref::empty() const -> bool {
    return length() == 0;
  }

  auto string_ref::copy_to(char* dst, std::size_t dst_count) const -> bool {
    if (!dst || dst_count == 0) {
      return false;
    }
    dst[0] = '\0';
    const char* src = data();
    const std::uint32_t len = length();
    constexpr std::uint32_t k_max_copy = 512;
    if (len > k_max_copy || len + 1 > dst_count) {
      return false;
    }
    for (std::uint32_t i = 0; i <= len; ++i) {
      dst[i] = src[i];
    }
    return true;
  }

  // ---------------------------------------------------------------------------
  // map_ref
  // ---------------------------------------------------------------------------

  auto map_ref::from(const void* object) -> map_ref {
    map_ref view{};
    view.object_ = object;
    return view;
  }

  auto map_ref::size() const -> std::uint32_t {
    if (!object_) {
      return 0;
    }
    return *reinterpret_cast<const std::uint32_t*>(static_cast<const char*>(object_) + res_map_size_offset);
  }

  auto map_ref::sentinel() const -> const map_tree_node* {
    if (!object_) {
      return nullptr;
    }
    return *reinterpret_cast<const map_tree_node* const*>(
      static_cast<const char*>(object_) + res_map_sentinel_offset);
  }

  auto map_ref::minimum() const -> const map_tree_node* {
    if (!object_ || size() == 0) {
      return nullptr;
    }
    const auto* end = sentinel();
    if (!is_game_ptr(end)) {
      return nullptr;
    }
    const auto* node = end->parent;
    if (!is_game_ptr(node) || node->is_nil()) {
      return end;
    }
    return node;
  }

  auto map_ref::next(const map_tree_node* node, const map_tree_node* end) -> const map_tree_node* {
    if (!is_game_ptr(node) || !is_game_ptr(end)) {
      return end;
    }
    if (is_game_ptr(node->right) && !node->right->is_nil()) {
      auto* walk = node->right;
      while (is_game_ptr(walk->left) && !walk->left->is_nil()) {
        walk = walk->left;
      }
      return walk;
    }
    const auto* parent = node->parent;
    while (is_game_ptr(parent) && parent != end && node == parent->right) {
      node = parent;
      parent = parent->parent;
    }
    return parent;
  }

  auto map_ref::find(std::uint32_t key) const -> void* {
    return game_map_find(const_cast<void*>(object_), key);
  }

  auto map_tree_node::is_nil() const -> bool {
    return reinterpret_cast<const std::uint8_t*>(this)[21] != 0;
  }

  // ---------------------------------------------------------------------------
  // list_ref (legacy wrapper around list<void*>)
  // ---------------------------------------------------------------------------

  auto list_ref::from(const void* list_head) -> list_ref {
    list_ref view{};
    view.impl_ = list<void*>::from(list_head);
    return view;
  }

  auto list_ref::from_object(const void* object) -> list_ref {
    list_ref view{};
    view.impl_ = list<void*>::from_object(object);
    return view;
  }

  auto list_ref::size() const -> std::uint32_t {
    return static_cast<std::uint32_t>(impl_.size());
  }

  auto list_ref::sentinel() const -> const void* {
    return impl_.sentinel();
  }

  // ---------------------------------------------------------------------------
  // stdext_hash_map_ref (parsed pstitle document)
  // ---------------------------------------------------------------------------

  auto stdext_hash_map_ref::from(const void* object) -> stdext_hash_map_ref {
    stdext_hash_map_ref view{};
    view.object_ = object;
    return view;
  }

  auto stdext_hash_map_ref::element_count() const -> std::uint32_t {
    if (!object_) {
      return 0;
    }
    return *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(object_) + stdext_hash_map::maxidx);
  }

  auto stdext_hash_map_ref::list_view() const -> list_ref {
    if (!object_) {
      return {};
    }
    return list_ref::from(static_cast<const std::uint8_t*>(object_) + stdext_hash_map::list);
  }

  // ---------------------------------------------------------------------------
  // wstring (owned)
  // ---------------------------------------------------------------------------

  wstring::wstring() {
    init_empty();
  }

  wstring::wstring(const wchar_t* text) {
    init_empty();
    assign(text);
  }

  wstring::wstring(const wstring& other) {
    init_empty();
    assign(other.data(), other.length());
  }

  wstring::wstring(wstring&& other) noexcept {
    std::memcpy(storage_, other.storage_, sizeof(storage_));
    other.init_empty();
  }

  wstring::~wstring() {
    clear();
  }

  auto wstring::operator=(const wstring& other) -> wstring& {
    if (this != &other) {
      assign(other.data(), other.length());
    }
    return *this;
  }

  auto wstring::operator=(wstring&& other) noexcept -> wstring& {
    if (this != &other) {
      clear();
      std::memcpy(storage_, other.storage_, sizeof(storage_));
      other.init_empty();
    }
    return *this;
  }

  auto wstring::operator=(const wchar_t* text) -> wstring& {
    assign(text);
    return *this;
  }

  auto wstring::capacity() const -> std::uint32_t {
    return words()[6];
  }

  auto wstring::length() const -> std::uint32_t {
    return words()[5];
  }

  auto wstring::data() const -> const wchar_t* {
    return view().data();
  }

  auto wstring::empty() const -> bool {
    return length() == 0;
  }

  auto wstring::assign(const wchar_t* text) -> wstring& {
    if (!text) {
      clear();
      return *this;
    }
    return assign(text, std::wcslen(text));
  }

  auto wstring::assign(const wchar_t* text, std::size_t char_count) -> wstring& {
    if (!text || char_count == 0) {
      clear();
      return *this;
    }

    wchar_t stack_buf[128]{};
    const wchar_t* source = text;
    if (text[char_count] != L'\0') {
      if (char_count + 1 > sizeof(stack_buf) / sizeof(wchar_t)) {
        return *this;
      }
      std::wmemcpy(stack_buf, text, char_count);
      stack_buf[char_count] = L'\0';
      source = stack_buf;
    }

    game_wstring_assign(storage_, source, static_cast<std::uint32_t>(char_count));
    return *this;
  }

  auto wstring::clear() -> void {
    game_wstring_clear(storage_);
  }

  auto wstring::view() const -> wstring_ref {
    return wstring_ref::from(storage_);
  }

  auto wstring::init_empty() -> void {
    std::memset(storage_, 0, sizeof(storage_));
    words()[6] = wstring_sso_capacity;
  }

  auto wstring::words() -> std::uint32_t* {
    return reinterpret_cast<std::uint32_t*>(storage_);
  }

  auto wstring::words() const -> const std::uint32_t* {
    return reinterpret_cast<const std::uint32_t*>(storage_);
  }

  // ---------------------------------------------------------------------------
  // string (owned)
  // ---------------------------------------------------------------------------

  string::string() {
    init_empty();
  }

  string::string(const char* text) {
    init_empty();
    assign(text);
  }

  string::string(const string& other) {
    init_empty();
    assign(other.data(), other.length());
  }

  string::string(string&& other) noexcept {
    std::memcpy(storage_, other.storage_, sizeof(storage_));
    other.init_empty();
  }

  string::~string() {
    clear();
  }

  auto string::operator=(const string& other) -> string& {
    if (this != &other) {
      assign(other.data(), other.length());
    }
    return *this;
  }

  auto string::operator=(string&& other) noexcept -> string& {
    if (this != &other) {
      clear();
      std::memcpy(storage_, other.storage_, sizeof(storage_));
      other.init_empty();
    }
    return *this;
  }

  auto string::operator=(const char* text) -> string& {
    assign(text);
    return *this;
  }

  auto string::capacity() const -> std::uint32_t {
    return words()[6];
  }

  auto string::length() const -> std::uint32_t {
    return words()[5];
  }

  auto string::data() const -> const char* {
    return view().data();
  }

  auto string::empty() const -> bool {
    return length() == 0;
  }

  auto string::assign(const char* text) -> string& {
    if (!text) {
      clear();
      return *this;
    }
    return assign(text, std::strlen(text));
  }

  auto string::assign(const char* text, std::size_t byte_count) -> string& {
    if (!text || byte_count == 0) {
      clear();
      return *this;
    }
    game_string_assign(storage_, text, byte_count);
    return *this;
  }

  auto string::clear() -> void {
    game_string_clear(storage_);
  }

  auto string::view() const -> string_ref {
    return string_ref::from(storage_);
  }

  auto string::init_empty() -> void {
    std::memset(storage_, 0, sizeof(storage_));
    words()[6] = string_sso_capacity;
  }

  auto string::words() -> std::uint32_t* {
    return reinterpret_cast<std::uint32_t*>(storage_);
  }

  auto string::words() const -> const std::uint32_t* {
    return reinterpret_cast<const std::uint32_t*>(storage_);
  }

  // ---------------------------------------------------------------------------
  // ui_res_map (owned)
  // ---------------------------------------------------------------------------

  ui_res_map::ui_res_map() {
    construct();
  }

  ui_res_map::ui_res_map(ui_res_map&& other) noexcept {
    std::memcpy(storage_, other.storage_, sizeof(storage_));
    initialized_ = other.initialized_;
    other.initialized_ = false;
    std::memset(other.storage_, 0, sizeof(other.storage_));
  }

  ui_res_map::~ui_res_map() {
    destroy();
  }

  auto ui_res_map::operator=(ui_res_map&& other) noexcept -> ui_res_map& {
    if (this != &other) {
      destroy();
      std::memcpy(storage_, other.storage_, sizeof(storage_));
      initialized_ = other.initialized_;
      other.initialized_ = false;
      std::memset(other.storage_, 0, sizeof(other.storage_));
    }
    return *this;
  }

  auto ui_res_map::size() const -> std::uint32_t {
    return view().size();
  }

  auto ui_res_map::find(std::uint32_t key, bool add_base_key) const -> void* {
    return game_map_find(const_cast<void*>(static_cast<const void*>(storage_)), key, add_base_key);
  }

  auto ui_res_map::insert(std::uint32_t key, void* value) -> bool {
    if (!initialized_) {
      return false;
    }

    if (find(key)) {
      return map_walk_set(map_sentinel(storage_), key, value);
    }

    using lower_bound_fn = void*(__thiscall*)(void*, void*, unsigned*);
    const auto lower = as_fn<lower_bound_fn>(ext_client::offsets::msvc9_stl::functions::map_lower_bound);

    unsigned key_copy = key;
    int iter_out[2]{};
    lower(storage_, iter_out, &key_copy);

    using insert_fn = void*(__thiscall*)(void*, void*, char, void*, unsigned*);
    const auto ins = as_fn<insert_fn>(ext_client::offsets::msvc9_stl::functions::map_insert_node);

    int insert_out[2]{};
    ins(storage_, insert_out, 1, reinterpret_cast<void*>(iter_out[1]), &key_copy);

    return map_walk_set(map_sentinel(storage_), key, value);
  }

  auto ui_res_map::erase(std::uint32_t key) -> bool {
    if (!find(key)) {
      return false;
    }
    return insert(key, nullptr);
  }

  auto ui_res_map::clear() -> void {
    if (!initialized_) {
      return;
    }
    auto* end = map_sentinel(storage_);
    if (!end) {
      return;
    }
    auto* root = end->parent;
    if (root && !root->is_nil()) {
      game_map_clear_tree(root);
    }
    map_reset_sentinel(storage_);
  }

  auto ui_res_map::view() const -> map_ref {
    return map_ref::from(storage_);
  }

  auto ui_res_map::construct() -> void {
    std::memset(storage_, 0, sizeof(storage_));
    game_map_construct(storage_);
    initialized_ = true;
  }

  auto ui_res_map::destroy() -> void {
    if (!initialized_) {
      return;
    }
    clear();
    auto* sentinel = map_sentinel(storage_);
    if (sentinel) {
      game_heap_free(sentinel, 0x18);
    }
    initialized_ = false;
    std::memset(storage_, 0, sizeof(storage_));
  }

  // ---------------------------------------------------------------------------
  // vector_u8 (owned)
  // ---------------------------------------------------------------------------

  vector_u8::vector_u8() {
    init_empty();
  }

  vector_u8::vector_u8(vector_u8&& other) noexcept {
    std::memcpy(storage_, other.storage_, sizeof(storage_));
    std::memset(other.storage_, 0, sizeof(other.storage_));
  }

  vector_u8::~vector_u8() {
    destroy();
  }

  auto vector_u8::operator=(vector_u8&& other) noexcept -> vector_u8& {
    if (this != &other) {
      destroy();
      std::memcpy(storage_, other.storage_, sizeof(storage_));
      std::memset(other.storage_, 0, sizeof(other.storage_));
    }
    return *this;
  }

  auto vector_u8::init_empty() -> void {
    std::memset(storage_, 0, sizeof(storage_));
  }

  auto vector_u8::destroy() -> void {
    auto* begin = *reinterpret_cast<std::uint8_t**>(storage_ + 4);
    if (begin) {
      game_heap_free(begin, capacity());
    }
    init_empty();
  }

  auto vector_u8::size() const -> std::size_t {
    return vector_view<std::uint8_t>::from(storage_).size();
  }

  auto vector_u8::capacity() const -> std::size_t {
    return vector_view<std::uint8_t>::from(storage_).capacity();
  }

  auto vector_u8::data() -> std::uint8_t* {
    return *reinterpret_cast<std::uint8_t**>(storage_ + 4);
  }

  auto vector_u8::data() const -> const std::uint8_t* {
    return *reinterpret_cast<std::uint8_t* const*>(storage_ + 4);
  }

  auto vector_u8::empty() const -> bool {
    return size() == 0;
  }

  auto vector_u8::push_back(std::uint8_t value) -> void {
    auto* begin = data();
    auto* end = *reinterpret_cast<std::uint8_t**>(storage_ + 8);
    auto* cap = *reinterpret_cast<std::uint8_t**>(storage_ + 12);

    if (!begin) {
      begin = static_cast<std::uint8_t*>(game_heap_alloc(16));
      end = begin;
      cap = begin + 16;
      *reinterpret_cast<std::uint8_t**>(storage_ + 4) = begin;
      *reinterpret_cast<std::uint8_t**>(storage_ + 8) = end;
      *reinterpret_cast<std::uint8_t**>(storage_ + 12) = cap;
    } else if (end >= cap) {
      const std::size_t old_bytes = static_cast<std::size_t>(cap - begin);
      const std::size_t new_bytes = old_bytes == 0 ? 16 : old_bytes * 2;
      auto* grown = static_cast<std::uint8_t*>(game_heap_alloc(new_bytes));
      const std::size_t used = static_cast<std::size_t>(end - begin);
      std::memcpy(grown, begin, used);
      game_heap_free(begin, old_bytes);
      begin = grown;
      end = begin + used;
      cap = begin + new_bytes;
      *reinterpret_cast<std::uint8_t**>(storage_ + 4) = begin;
      *reinterpret_cast<std::uint8_t**>(storage_ + 8) = end;
      *reinterpret_cast<std::uint8_t**>(storage_ + 12) = cap;
    }

    *end = value;
    *reinterpret_cast<std::uint8_t**>(storage_ + 8) = end + 1;
  }

  auto vector_u8::clear() -> void {
    *reinterpret_cast<std::uint8_t**>(storage_ + 8) = data();
  }

} // namespace ext_client::msvc9
