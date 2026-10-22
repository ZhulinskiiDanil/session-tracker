#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// A filled rectangle with rounded corners
class RectNode : public CCNode
{
private:
  CCDrawNode *m_draw = nullptr;
  ccColor4F m_color = {1.f, 1.f, 1.f, 1.f};
  float m_radius = 0.f;

  bool init(CCSize size, ccColor4F color, float radius);
  void redraw();

public:
  static RectNode *create(CCSize size, ccColor4F color, float radius = 0.f);
  static RectNode *create(CCSize size, ccColor4B color, float radius = 0.f);

  void setSize(CCSize size);
};
