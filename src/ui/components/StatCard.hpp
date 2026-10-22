#pragma once

#include <Geode/Geode.hpp>

#include "RichLabel.hpp"

using namespace geode::prelude;

// A big number with a caption under it, on a Blitzkrieg range cell background
class StatCard : public CCNode
{
public:
  enum class Style
  {
    Default,
    Current,
    Completed
  };

private:
  bool init(CCSize size, std::string const &value, std::string const &caption, Style style, RichLabel::Variant variant);

public:
  static StatCard *create(CCSize size, std::string const &value, std::string const &caption,
                          Style style = Style::Default, RichLabel::Variant variant = RichLabel::Variant::Default);
};

// The background of cells and cards for a style
char const *cellBackground(StatCard::Style style);
