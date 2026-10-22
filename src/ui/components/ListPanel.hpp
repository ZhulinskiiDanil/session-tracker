#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Blitzkrieg's dark list: a rounded panel with borders and a scrolling column inside
class ListPanel : public CCNode
{
private:
  ScrollLayer *m_scroll = nullptr;
  CCLabelBMFont *m_empty = nullptr;

  bool init(CCSize size, float gap);

public:
  static ListPanel *create(CCSize size, float gap = 4.f);

  float innerWidth() const;
  void clear();
  void add(CCNode *node);
  // After adding: lays the column out, at the top or keeping the scroll position
  void finish(bool toTop);
  void setEmptyText(std::string const &text);
};
