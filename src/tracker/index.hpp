#pragma once

#include <Geode/Geode.hpp>

#include "../data/index.hpp"

using namespace geode::prelude;

namespace tracker
{
  using Clock = std::chrono::steady_clock;

  // The session of the level being played, from entering it to leaving it
  class Tracker
  {
  private:
    struct Current
    {
      std::string key;
      data::Session session;
      // Where the session sits in its level's list once saved there
      std::optional<size_t> index;
      bool platformer = false;

      Clock::time_point started;
      Clock::time_point lastSave;
      std::optional<Clock::time_point> pausedAt;
      double pausedSeconds = 0;

      int lastAttempt = 0;
      bool practice = false;
      bool fromZero = true;
      bool died = false;
    };

    std::optional<Current> m_current;

    // This launch of the game
    double m_launchSeconds = 0;
    std::set<std::string> m_launchLevels;

    Tracker() = default;

    void refreshTimes();
    void store();

  public:
    static Tracker &get();

    void begin(PlayLayer *layer);
    void onReset(PlayLayer *layer);
    void onDeath(PlayLayer *layer);
    void onComplete(PlayLayer *layer);
    void pause();
    void resume();
    void end();

    // Every frame: saves once a minute
    void tick();
    // Save the session as it is without ending it, when the game closes
    void flush();

    bool active() const;
    std::string const &key() const;
    data::Session const &session();
    // The stats of the current level's group, this session as it is now included
    data::Totals groupTotals();

    double launchSeconds();
    size_t launchLevels() const;
  };
}
