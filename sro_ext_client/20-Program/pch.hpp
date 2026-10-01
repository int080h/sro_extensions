#pragma once

// Platform header — always used, expensive to parse
#include <windows.h>

// Common C++ standard library headers
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// Intentionally no ImGui here — include only in render/plugin UI translation units.
