#pragma once

#include <Geode/Geode.hpp>

#include "../../data/index.hpp"
#include "../components/ListPanel.hpp"

using namespace geode::prelude;

// Pick a played level to put this level's group together with
class MergePopup : public geode::Popup
{
private:
  std::string m_key;
  geode::Function<void()> m_onMerged;

  TextInput *m_search = nullptr;
  ListPanel *m_list = nullptr;

  bool init(std::string key, geode::Function<void()> onMerged);

  void rebuild();
  void merge(std::string const &target, std::string const &name);

public:
  static MergePopup *create(std::string key, geode::Function<void()> onMerged);
};
