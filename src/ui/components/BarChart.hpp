#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

struct ChartBar
{
  float value = 0.f;
  ccColor4B color = {120, 120, 120, 255};
};

struct ChartTick
{
  // Across the chart, 0 to 1
  float at = 0.f;
  std::string text;
};

// Bars over a grid on a dark rounded panel, in Blitzkrieg's graph colors
class BarChart : public CCNode
{
private:
  bool init(CCSize size, std::string const &title, std::string const &legend, std::vector<ChartBar> const &bars,
            std::vector<ChartTick> const &ticks, std::optional<ChartTick> marker, std::string const &emptyText);

public:
  // `legend` is drawn top right, `marker` is a green line with its text, at the best percent for one
  static BarChart *create(CCSize size, std::string const &title, std::string const &legend, std::vector<ChartBar> const &bars,
                          std::vector<ChartTick> const &ticks, std::optional<ChartTick> marker, std::string const &emptyText);
};
