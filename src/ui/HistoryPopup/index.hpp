#pragma once

#include <Geode/Geode.hpp>

#include "../../data/index.hpp"
#include "../components/ListPanel.hpp"

using namespace geode::prelude;

// Everything about a level and its group, in tabs laid out like Blitzkrieg's stages popup
class HistoryPopup : public geode::Popup
{
public:
  enum class Tab
  {
    // The session being played, when it's this group's
    Session,
    Overview,
    Sessions,
    Levels
  };

private:
  std::string m_key;
  Tab m_tab = Tab::Overview;
  // The level's group, or only the level itself
  bool m_wholeGroup = true;
  bool m_newestFirst = true;

  std::vector<std::pair<Tab, TabButton *>> m_tabs;
  CCNode *m_content = nullptr;
  ListPanel *m_list = nullptr;

  bool init(std::string key, Tab tab);

  std::string root();
  bool grouped();
  bool liveHere();
  std::vector<data::Level const *> shownLevels();
  // "Main", "Copy #2" or "Linked #3"
  std::string tagFor(data::Level const *level);

  void drawTabs();
  void onTab(CCObject *sender);
  void selectTab(Tab tab);
  void drawContent();

  void drawHeader(std::string const &title, std::string const &subtitle);
  // The scope and sort toggles in the header's right side
  void drawFilters(bool sort);
  void drawCards(std::vector<CCNode *> const &cards, float top);
  CCNode *createDeathsChart(data::Totals const &totals, CCSize size, std::string const &title);
  CCNode *createTimeChart(std::vector<data::Session const *> const &sessions, CCSize size);
  void drawBottomButton(char const *text, geode::Function<void()> callback);

  void drawSession();
  void drawOverview();
  void drawSessions();
  void drawLevels();

  void fillSessions();
  void fillLevels();

  void onExport();
  void onMerge();
  void onSplit(std::string const &key, std::string const &name);

public:
  static HistoryPopup *create(std::string key, Tab tab = Tab::Overview);
};
