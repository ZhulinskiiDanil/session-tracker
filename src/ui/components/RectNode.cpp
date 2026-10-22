#include "RectNode.hpp"

RectNode *RectNode::create(CCSize size, ccColor4F color, float radius)
{
  auto ret = new RectNode();

  if (ret->init(size, color, radius))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

RectNode *RectNode::create(CCSize size, ccColor4B color, float radius)
{
  return create(size, ccc4FFromccc4B(color), radius);
}

bool RectNode::init(CCSize size, ccColor4F color, float radius)
{
  if (!CCNode::init())
    return false;

  m_color = color;
  m_radius = radius;

  m_draw = CCDrawNode::create();
  addChild(m_draw);

  setSize(size);
  return true;
}

void RectNode::setSize(CCSize size)
{
  setContentSize(size);
  m_draw->setContentSize(size);
  redraw();
}

void RectNode::redraw()
{
  m_draw->clear();

  float w = getContentWidth();
  float h = getContentHeight();

  if (w <= 0.f || h <= 0.f)
    return;

  float r = std::clamp(m_radius, 0.f, std::min(w, h) / 2.f);

  if (r <= .01f)
  {
    CCPoint vertices[] = {{0.f, 0.f}, {w, 0.f}, {w, h}, {0.f, h}};
    m_draw->drawPolygon(vertices, 4, m_color, 0.f, {0.f, 0.f, 0.f, 0.f});
    return;
  }

  std::vector<CCPoint> vertices;
  int segments = std::clamp(static_cast<int>(std::ceil(r * 2.f)), 3, 16);

  // Counter-clockwise, one quarter circle per corner
  auto addCorner = [&](CCPoint center, float startAngle)
  {
    for (int i = 0; i <= segments; i++)
    {
      float angle = startAngle + static_cast<float>(M_PI_2) * i / segments;
      vertices.push_back({center.x + std::cos(angle) * r, center.y + std::sin(angle) * r});
    }
  };

  addCorner({w - r, r}, -static_cast<float>(M_PI_2));
  addCorner({w - r, h - r}, 0.f);
  addCorner({r, h - r}, static_cast<float>(M_PI_2));
  addCorner({r, r}, static_cast<float>(M_PI));

  m_draw->drawPolygon(vertices.data(), static_cast<unsigned int>(vertices.size()), m_color, 0.f, {0.f, 0.f, 0.f, 0.f});
}
