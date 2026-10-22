#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

struct MetaRow
{
  std::string label;
  std::string value;
};

// "Label: ......... value" lines, as in Blitzkrieg's expanded stage cells
class MetaTable : public CCNode
{
private:
  bool init(std::vector<MetaRow> const &rows, float width, float padding, float fontSize);

public:
  static MetaTable *create(std::vector<MetaRow> const &rows, float width, float padding = 5.f, float fontSize = .25f);
};
