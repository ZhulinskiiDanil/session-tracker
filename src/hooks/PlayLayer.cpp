#include <Geode/Geode.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "../tracker/index.hpp"

using namespace geode::prelude;
using namespace tracker;

class $modify(TrackerPlayLayer, PlayLayer)
{
  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects)
  {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;

    // Watching a replay isn't playing
    if (useReplay)
      return true;

    Tracker::get().begin(this);
    return true;
  }

  // Not called while paused, there is nothing new to save then
  void postUpdate(float dt)
  {
    PlayLayer::postUpdate(dt);
    Tracker::get().tick();
  }

  void resetLevel()
  {
    PlayLayer::resetLevel();
    Tracker::get().onReset(this);
  }

  void destroyPlayer(PlayerObject *player, GameObject *object)
  {
    PlayLayer::destroyPlayer(player, object);

    // The anticheat spike kills nobody, and with noclip the player lives on
    if (object != m_anticheatSpike && m_playerDied)
      Tracker::get().onDeath(this);
  }

  void levelComplete()
  {
    PlayLayer::levelComplete();
    Tracker::get().onComplete(this);
  }

  void pauseGame(bool unfocused)
  {
    PlayLayer::pauseGame(unfocused);

    if (m_isPaused)
      Tracker::get().pause();
  }

  void resume()
  {
    PlayLayer::resume();
    Tracker::get().resume();
  }

  void resumeAndRestart(bool fromStart)
  {
    PlayLayer::resumeAndRestart(fromStart);
    Tracker::get().resume();
  }

  void onQuit()
  {
    Tracker::get().end();
    PlayLayer::onQuit();
  }
};

class $modify(TrackerAppDelegate, AppDelegate)
{
  void trySaveGame(bool force)
  {
    Tracker::get().flush();
    AppDelegate::trySaveGame(force);
  }
};
