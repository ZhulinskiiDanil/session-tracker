#include <Geode/Geode.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include "../tracker/index.hpp"
#include "../ui/HistoryPopup/index.hpp"

using namespace geode::prelude;
using namespace tracker;

// The clock button on level pages and in the pause menu
static void addHistoryButton(CCNode *layer, char const *menuID, std::string key, HistoryPopup::Tab tab, CCPoint fallback)
{
  auto spr = CircleButtonSprite::createWithSpriteFrameName("GJ_timeIcon_001.png", 1.f, CircleBaseColor::Gray, CircleBaseSize::Medium);
  spr->setScale(.8f);

  auto button = CCMenuItemExt::createSpriteExtra(spr, [key = std::move(key), tab](auto)
                                                 { HistoryPopup::create(key, tab)->show(); });
  button->setID("history-button"_spr);

  auto menu = layer->getChildByID(menuID);

  // Another mod moved the menu away: a menu of the mod's own
  if (!menu)
  {
    menu = CCMenu::create();
    menu->setID("menu"_spr);
    menu->setContentSize(button->getContentSize());
    menu->setPosition(fallback);
    layer->addChild(menu, 10);
    button->setPosition(menu->getContentSize() / 2);
  }

  menu->addChild(button);
  menu->updateLayout();
}

class $modify(TrackerLevelInfoLayer, LevelInfoLayer)
{
  bool init(GJGameLevel *level, bool challenge)
  {
    if (!LevelInfoLayer::init(level, challenge))
      return false;

    auto size = CCDirector::get()->getWinSize();
    addHistoryButton(this, "left-side-menu", data::keyFor(level), HistoryPopup::Tab::Overview, {30.f, size.height / 2.f - 60.f});
    return true;
  }
};

class $modify(TrackerEditLevelLayer, EditLevelLayer)
{
  bool init(GJGameLevel *level)
  {
    if (!EditLevelLayer::init(level))
      return false;

    auto size = CCDirector::get()->getWinSize();
    addHistoryButton(this, "level-actions-menu", data::keyFor(level), HistoryPopup::Tab::Overview, {size.width - 30.f, 30.f});
    return true;
  }
};

class $modify(TrackerPauseLayer, PauseLayer)
{
  void customSetup()
  {
    PauseLayer::customSetup();

    if (!Tracker::get().active())
      return;

    auto size = CCDirector::get()->getWinSize();
    addHistoryButton(this, "right-button-menu", Tracker::get().key(), HistoryPopup::Tab::Session, {size.width - 30.f, size.height / 2.f});
  }

  // Straight to the editor, the PlayLayer goes without onQuit
  void onEdit(CCObject *sender)
  {
    Tracker::get().end();
    PauseLayer::onEdit(sender);
  }
};
