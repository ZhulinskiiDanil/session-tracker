#pragma once

#include <Geode/Geode.hpp>

#include <ctime>

namespace fmt_time
{
  inline std::tm local(int64_t unix)
  {
    std::time_t time = unix;
    std::tm result{};
#ifdef GEODE_IS_WINDOWS
    localtime_s(&result, &time);
#else
    localtime_r(&time, &result);
#endif
    return result;
  }

  // 2026-10-21 14:30, local time
  inline std::string date(int64_t unix)
  {
    auto tm = local(unix);
    return fmt::format("{:04}-{:02}-{:02} {:02}:{:02}", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
  }

  // For a Label: 21 Oct <small>14:30</small>
  inline std::string richDate(int64_t unix)
  {
    static constexpr char const *MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    auto tm = local(unix);
    return fmt::format("{} {} <small>{:02}:{:02}</small>", tm.tm_mday, MONTHS[tm.tm_mon], tm.tm_hour, tm.tm_min);
  }

  // For a Label: 1<small>h</small> 5<small>m</small>, as Blitzkrieg shows time played
  inline std::string rich(double seconds)
  {
    auto total = static_cast<int64_t>(std::max(0.0, seconds));
    auto days = total / 86400;
    auto hours = total / 3600 % 24;
    auto minutes = total / 60 % 60;
    auto secs = total % 60;

    std::vector<std::string> parts;
    if (days > 0)
      parts.push_back(fmt::format("{}<small>d</small>", days));
    if (hours > 0)
      parts.push_back(fmt::format("{}<small>h</small>", hours));
    if (minutes > 0)
      parts.push_back(fmt::format("{}<small>{}</small>", minutes, days == 0 && hours == 0 ? "min" : "m"));
    // Seconds only matter while it's short
    if (days == 0 && hours == 0 && (secs > 0 || parts.empty()))
      parts.push_back(fmt::format("{}<small>s</small>", secs));

    std::string result;
    for (auto const &part : parts)
      result += (result.empty() ? "" : " ") + part;
    return result;
  }

  // 1234 attempt(s) for a RichLabel
  inline std::string count(int value, std::string_view singular, std::string_view plural)
  {
    return fmt::format("{} <small>{}</small>", value, value == 1 ? singular : plural);
  }
}
