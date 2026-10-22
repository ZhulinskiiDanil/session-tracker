#pragma once

#include <Geode/Geode.hpp>

#include <map>

using namespace geode::prelude;

namespace data
{
  struct Session
  {
    // Unix seconds
    int64_t start = 0;
    int64_t end = 0;
    double activeSeconds = 0;
    double pausedSeconds = 0;

    // Normal mode, start position ones included
    int attempts = 0;
    int startPosAttempts = 0;
    int practiceAttempts = 0;
    // Normal mode from 0%
    int completions = 0;
    int best = 0;

    // Percent -> deaths there, normal mode only
    std::map<int, int> deaths;

    // Still being played, or the game closed before it ended
    bool inProgress = false;

    int shownAttempts() const;
  };

  struct Level
  {
    std::string key;
    std::string name;
    std::string creator;
    int levelID = 0;
    // The level this one was copied from, 0 for an original
    int originalID = 0;
    bool local = false;
    bool platformer = false;

    std::vector<Session> sessions;
  };

  struct Totals
  {
    double activeSeconds = 0;
    double pausedSeconds = 0;
    int sessions = 0;
    int attempts = 0;
    int startPosAttempts = 0;
    int practiceAttempts = 0;
    int completions = 0;
    int best = 0;
    int64_t first = 0;
    int64_t last = 0;
    std::map<int, int> deaths;

    void add(Session const &session);
    int shownAttempts() const;
    // The percents with the most deaths, most first
    std::vector<std::pair<int, int>> hardest(size_t count) const;
  };

  int64_t now();

  std::string keyFor(GJGameLevel *level);

  class Storage
  {
  private:
    std::map<std::string, Level> m_levels;
    // Level key -> key of its group's root. A level missing here is its own group
    std::map<std::string, std::string> m_links;
    // Merged or split by hand: never relinked automatically
    std::set<std::string> m_manual;
    bool m_loaded = false;

    Storage() = default;

    std::filesystem::path levelsDir() const;
    std::filesystem::path groupsPath() const;
    void load();
    void saveGroups() const;
    void relink(std::string const &key);

  public:
    static Storage &get();

    // Creates the level on first play and refreshes its name and links
    Level &touch(GJGameLevel *level);
    Level *find(std::string const &key);
    std::vector<Level const *> all();

    void save(std::string const &key) const;

    std::string rootOf(std::string const &key);
    // Every played level of the group, the root first
    std::vector<Level const *> members(std::string const &root);
    Totals totals(std::string const &root);
    // Name to show for a group: its root's, else the first played member's
    std::string groupName(std::string const &root);
    // The root of an auto group is an online ID that may never have been played
    static std::optional<int> onlineID(std::string const &key);

    // Moves the whole group of `key` into the group of `target`
    void merge(std::string const &key, std::string const &target);
    // Takes `key` out of its group, the rest stays together
    void split(std::string const &key);
  };
}
