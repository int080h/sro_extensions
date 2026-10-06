#include "pch.hpp"
#include "sdk/types/packed_time.hpp"

namespace ext_client::sdk {

  auto is_leap_year(int year) noexcept -> bool {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  }

  auto days_in_month(int year, int month) noexcept -> int {
    static constexpr int k_days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
      return 0;
    }
    if (month == 2 && is_leap_year(year)) {
      return 29;
    }
    return k_days_per_month[month - 1];
  }

  namespace {
    auto add_one_month(SYSTEMTIME& time) -> bool {
      int year = time.wYear;
      int month = time.wMonth + 1;
      if (month > 12) {
        month = 1;
        ++year;
      }

      if (time.wDay > days_in_month(year, month)) {
        return false;
      }

      time.wYear = static_cast<WORD>(year);
      time.wMonth = static_cast<WORD>(month);
      return true;
    }

    auto compare_system_time(const SYSTEMTIME& lhs, const SYSTEMTIME& rhs) -> int {
      FILETIME lhs_file{};
      FILETIME rhs_file{};
      if (!SystemTimeToFileTime(&lhs, &lhs_file) || !SystemTimeToFileTime(&rhs, &rhs_file)) {
        return 0;
      }
      const auto lhs_value = (static_cast<ULONGLONG>(lhs_file.dwHighDateTime) << 32) | lhs_file.dwLowDateTime;
      const auto rhs_value = (static_cast<ULONGLONG>(rhs_file.dwHighDateTime) << 32) | rhs_file.dwLowDateTime;
      if (lhs_value < rhs_value) {
        return -1;
      }
      if (lhs_value > rhs_value) {
        return 1;
      }
      return 0;
    }

    auto diff_minutes_between(const SYSTEMTIME& start, const SYSTEMTIME& end) -> ULONGLONG {
      FILETIME start_file{};
      FILETIME end_file{};
      if (!SystemTimeToFileTime(&start, &start_file) || !SystemTimeToFileTime(&end, &end_file)) {
        return 0;
      }

      ULARGE_INTEGER start_value{};
      start_value.LowPart = start_file.dwLowDateTime;
      start_value.HighPart = start_file.dwHighDateTime;

      ULARGE_INTEGER end_value{};
      end_value.LowPart = end_file.dwLowDateTime;
      end_value.HighPart = end_file.dwHighDateTime;

      if (end_value.QuadPart <= start_value.QuadPart) {
        return 0;
      }

      return (end_value.QuadPart - start_value.QuadPart) / (10ULL * 1000ULL * 1000ULL * 60ULL);
    }
  } // namespace

  auto compute_elapsed_logout(packed_time time, elapsed_logout_parts& out, const SYSTEMTIME* ref_now) -> bool {
    const WORD logout_month = time.month();
    const WORD logout_day = time.day();
    const WORD logout_hour = time.hour();
    const WORD logout_minute = time.minute();

    if (logout_month < 1 || logout_month > 12 || logout_day == 0 || logout_hour > 23 || logout_minute > 59) {
      return false;
    }

    SYSTEMTIME now{};
    if (ref_now) {
      now = *ref_now;
    } else {
      GetLocalTime(&now);
    }

    SYSTEMTIME candidate{};
    candidate.wYear = now.wYear;
    candidate.wMonth = logout_month;
    candidate.wDay = logout_day;
    candidate.wHour = logout_hour;
    candidate.wMinute = logout_minute;
    candidate.wSecond = 0;
    candidate.wMilliseconds = 0;
    candidate.wDayOfWeek = 0;

    const int max_day = days_in_month(candidate.wYear, candidate.wMonth);
    if (logout_day > max_day) {
      return false;
    }

    while (compare_system_time(candidate, now) > 0) {
      if (candidate.wYear == 0) {
        return false;
      }
      --candidate.wYear;
    }

    if (compare_system_time(candidate, now) > 0) {
      return false;
    }

    elapsed_logout_parts result{};
    SYSTEMTIME cursor = candidate;

    while (result.months < 12) {
      SYSTEMTIME next = cursor;
      if (!add_one_month(next) || compare_system_time(next, now) > 0) {
        break;
      }
      cursor = next;
      ++result.months;
    }

    const ULONGLONG total_minutes = diff_minutes_between(cursor, now);
    result.days = static_cast<std::uint32_t>(total_minutes / (24ULL * 60ULL));
    result.hours = static_cast<std::uint32_t>((total_minutes / 60ULL) % 24ULL);
    result.minutes = static_cast<std::uint32_t>(total_minutes % 60ULL);

    out = result;
    return true;
  }

} // namespace ext_client::sdk
