#pragma once

#include <Geode/Geode.hpp>

#include "../../data/index.hpp"

using namespace geode::prelude;

// One session in the list: a summary line that expands into the details, like Blitzkrieg's stage cells
class SessionCell : public CCNode
{
private:
  data::Session m_session;
  bool m_platformer = false;
  // The level it was played on when the list shows a whole group
  std::string m_levelText;
  bool m_live = false;
  bool m_expanded = false;
  float m_width = 0.f;
  geode::Function<void()> m_onResize;

  bool init(float width, data::Session const &session, bool platformer, std::string levelText, bool live, geode::Function<void()> onResize);
  void build();
  CCNode *createHead();
  CCNode *createDetails();

public:
  static SessionCell *create(float width, data::Session const &session, bool platformer, std::string levelText, bool live,
                             geode::Function<void()> onResize);
};
