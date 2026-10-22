#include "index.hpp"

namespace tracker
{
  static double secondsSince(Clock::time_point point)
  {
    return std::chrono::duration<double>(Clock::now() - point).count();
  }

  Tracker &Tracker::get()
  {
    static Tracker instance;
    return instance;
  }

  void Tracker::begin(PlayLayer *layer)
  {
    // Left without onQuit, through the editor for one
    if (m_current)
      end();

    auto &level = data::Storage::get().touch(layer->m_level);
    level.platformer = layer->m_isPlatformer;

    Current current;
    current.key = level.key;
    current.platformer = layer->m_isPlatformer;
    current.started = Clock::now();
    current.lastSave = current.started;
    current.session.start = data::now();
    current.session.end = current.session.start;
    current.session.inProgress = true;

    m_current = std::move(current);
    m_launchLevels.insert(level.key);
  }

  void Tracker::onReset(PlayLayer *layer)
  {
    if (!m_current)
      return;

    if (!layer->m_isPaused)
      resume();

    auto &current = *m_current;
    int attempt = layer->m_attempts;

    // Not every reset starts a new attempt
    if (attempt == current.lastAttempt)
      return;

    current.lastAttempt = attempt;
    current.died = false;
    current.practice = layer->m_isPracticeMode;
    current.fromZero = !current.practice && !layer->m_isTestMode && !layer->m_startPosObject;

    if (current.practice)
      current.session.practiceAttempts++;
    else
    {
      current.session.attempts++;
      if (!current.fromZero)
        current.session.startPosAttempts++;
    }
  }

  void Tracker::onDeath(PlayLayer *layer)
  {
    if (!m_current || m_current->died)
      return;

    auto &current = *m_current;
    current.died = true;

    // A platformer's percent is only how far right the player got
    if (current.practice || current.platformer)
      return;

    int percent = std::clamp(layer->getCurrentPercentInt(), 0, 100);
    current.session.deaths[percent]++;

    if (current.fromZero)
      current.session.best = std::max(current.session.best, percent);
  }

  void Tracker::onComplete(PlayLayer *)
  {
    if (!m_current)
      return;

    auto &current = *m_current;

    if (current.practice || !current.fromZero)
      return;

    current.session.completions++;
    current.session.best = 100;
  }

  void Tracker::pause()
  {
    if (m_current && !m_current->pausedAt)
      m_current->pausedAt = Clock::now();
  }

  void Tracker::resume()
  {
    if (!m_current || !m_current->pausedAt)
      return;

    m_current->pausedSeconds += secondsSince(*m_current->pausedAt);
    m_current->pausedAt.reset();
  }

  void Tracker::refreshTimes()
  {
    auto &current = *m_current;

    double paused = current.pausedSeconds;
    if (current.pausedAt)
      paused += secondsSince(*current.pausedAt);

    double total = secondsSince(current.started);
    current.session.pausedSeconds = paused;
    current.session.activeSeconds = std::max(0.0, total - paused);
    current.session.end = data::now();
  }

  // Puts the session into its level's list and writes the level file
  void Tracker::store()
  {
    auto &current = *m_current;
    auto level = data::Storage::get().find(current.key);

    if (!level)
      return;

    if (current.index && *current.index < level->sessions.size())
      level->sessions[*current.index] = current.session;
    else
    {
      current.index = level->sessions.size();
      level->sessions.push_back(current.session);
    }

    data::Storage::get().save(current.key);
    current.lastSave = Clock::now();
  }

  void Tracker::end()
  {
    if (!m_current)
      return;

    resume();
    refreshTimes();

    auto &current = *m_current;
    current.session.inProgress = false;
    m_launchSeconds += current.session.activeSeconds;

    double minSeconds = static_cast<double>(Mod::get()->getSettingValue<int64_t>("min-session-seconds"));

    if (current.session.activeSeconds >= minSeconds)
      store();
    // Too short to keep: take back what an autosave already wrote
    else if (current.index)
    {
      auto level = data::Storage::get().find(current.key);
      if (level && *current.index < level->sessions.size())
      {
        level->sessions.erase(level->sessions.begin() + *current.index);
        data::Storage::get().save(current.key);
      }
    }

    m_current.reset();
  }

  void Tracker::tick()
  {
    if (!m_current || secondsSince(m_current->lastSave) < 60.0)
      return;

    refreshTimes();
    store();
  }

  void Tracker::flush()
  {
    if (!m_current)
      return;

    refreshTimes();
    store();
  }

  bool Tracker::active() const
  {
    return m_current.has_value();
  }

  std::string const &Tracker::key() const
  {
    static std::string const none;
    return m_current ? m_current->key : none;
  }

  data::Session const &Tracker::session()
  {
    static data::Session const none;

    if (!m_current)
      return none;

    refreshTimes();
    return m_current->session;
  }

  data::Totals Tracker::groupTotals()
  {
    data::Totals totals;

    if (!m_current)
      return totals;

    auto &storage = data::Storage::get();

    for (auto level : storage.members(storage.rootOf(m_current->key)))
    {
      for (size_t i = 0; i < level->sessions.size(); i++)
      {
        // Already saved by an autosave, but older than the live one
        if (level->key == m_current->key && m_current->index == i)
          continue;
        totals.add(level->sessions[i]);
      }
    }

    totals.add(session());
    return totals;
  }

  double Tracker::launchSeconds()
  {
    return m_launchSeconds + (m_current ? session().activeSeconds : 0.0);
  }

  size_t Tracker::launchLevels() const
  {
    return m_launchLevels.size();
  }
}
