#include "index.hpp"

namespace data
{
  static bool countPractice()
  {
    return Mod::get()->getSettingValue<bool>("count-practice");
  }

  int Session::shownAttempts() const
  {
    return attempts + (countPractice() ? practiceAttempts : 0);
  }

  void Totals::add(Session const &session)
  {
    activeSeconds += session.activeSeconds;
    pausedSeconds += session.pausedSeconds;
    sessions++;
    attempts += session.attempts;
    startPosAttempts += session.startPosAttempts;
    practiceAttempts += session.practiceAttempts;
    completions += session.completions;
    best = std::max(best, session.best);
    first = first ? std::min(first, session.start) : session.start;
    last = std::max(last, session.end);

    for (auto const &[percent, count] : session.deaths)
      deaths[percent] += count;
  }

  int Totals::shownAttempts() const
  {
    return attempts + (countPractice() ? practiceAttempts : 0);
  }

  std::vector<std::pair<int, int>> Totals::hardest(size_t count) const
  {
    std::vector<std::pair<int, int>> sorted(deaths.begin(), deaths.end());
    std::ranges::sort(sorted, [](auto const &a, auto const &b)
                      { return a.second != b.second ? a.second > b.second : a.first < b.first; });

    if (sorted.size() > count)
      sorted.resize(count);

    return sorted;
  }

  int64_t now()
  {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  std::string keyFor(GJGameLevel *level)
  {
    int id = level->m_levelID.value();

    switch (level->m_levelType)
    {
    case GJLevelType::Main:
      return fmt::format("main-{}", id);
    // Local levels have no ID until uploaded, and lose it again when copied
    case GJLevelType::Editor:
      return fmt::format("local-{}", std::string(level->m_levelName));
    default:
      return fmt::format("online-{}", id);
    }
  }

  // Level names can hold anything, file names can't
  static std::string fileName(std::string const &key)
  {
    std::string name;

    for (unsigned char c : key)
    {
      if (std::isalnum(c) || c == '-')
        name += static_cast<char>(c);
      else
        name += fmt::format("_{:02x}", c);
    }

    return name + ".json";
  }

  // ! --- JSON --- !

  static matjson::Value toJson(Session const &session)
  {
    auto deaths = matjson::Value::object();
    for (auto const &[percent, count] : session.deaths)
      deaths.set(std::to_string(percent), count);

    auto json = matjson::Value::object();
    json.set("start", session.start);
    json.set("end", session.end);
    json.set("active", session.activeSeconds);
    json.set("paused", session.pausedSeconds);
    json.set("attempts", session.attempts);
    json.set("start-pos-attempts", session.startPosAttempts);
    json.set("practice-attempts", session.practiceAttempts);
    json.set("completions", session.completions);
    json.set("best", session.best);
    json.set("deaths", deaths);
    json.set("in-progress", session.inProgress);
    return json;
  }

  static Session sessionFromJson(matjson::Value const &json)
  {
    Session session;
    session.start = json.get<int64_t>("start").unwrapOr(0);
    session.end = json.get<int64_t>("end").unwrapOr(session.start);
    session.activeSeconds = json.get<double>("active").unwrapOr(0);
    session.pausedSeconds = json.get<double>("paused").unwrapOr(0);
    session.attempts = json.get<int>("attempts").unwrapOr(0);
    session.startPosAttempts = json.get<int>("start-pos-attempts").unwrapOr(0);
    session.practiceAttempts = json.get<int>("practice-attempts").unwrapOr(0);
    session.completions = json.get<int>("completions").unwrapOr(0);
    session.best = json.get<int>("best").unwrapOr(0);
    session.inProgress = json.get<bool>("in-progress").unwrapOr(false);

    if (auto deaths = json.get("deaths"); deaths && deaths.unwrap().isObject())
    {
      for (auto const &entry : deaths.unwrap())
      {
        auto percent = numFromString<int>(entry.getKey().value_or(""));
        if (percent)
          session.deaths[percent.unwrap()] = entry.as<int>().unwrapOr(0);
      }
    }

    return session;
  }

  static matjson::Value toJson(Level const &level)
  {
    std::vector<matjson::Value> sessions;
    for (auto const &session : level.sessions)
      sessions.push_back(toJson(session));

    auto json = matjson::Value::object();
    json.set("key", level.key);
    json.set("name", level.name);
    json.set("creator", level.creator);
    json.set("level-id", level.levelID);
    json.set("original-id", level.originalID);
    json.set("local", level.local);
    json.set("platformer", level.platformer);
    json.set("sessions", matjson::Value(std::move(sessions)));
    return json;
  }

  static std::optional<Level> levelFromJson(matjson::Value const &json)
  {
    auto key = json.get<std::string>("key");
    if (!key)
      return std::nullopt;

    Level level;
    level.key = key.unwrap();
    level.name = json.get<std::string>("name").unwrapOr("");
    level.creator = json.get<std::string>("creator").unwrapOr("");
    level.levelID = json.get<int>("level-id").unwrapOr(0);
    level.originalID = json.get<int>("original-id").unwrapOr(0);
    level.local = json.get<bool>("local").unwrapOr(false);
    level.platformer = json.get<bool>("platformer").unwrapOr(false);

    if (auto sessions = json.get("sessions"); sessions && sessions.unwrap().isArray())
    {
      for (auto const &session : sessions.unwrap())
        level.sessions.push_back(sessionFromJson(session));
    }

    return level;
  }

  // ! --- Storage --- !

  Storage &Storage::get()
  {
    static Storage instance;
    instance.load();
    return instance;
  }

  std::filesystem::path Storage::levelsDir() const
  {
    return Mod::get()->getSaveDir() / "levels";
  }

  std::filesystem::path Storage::groupsPath() const
  {
    return Mod::get()->getSaveDir() / "groups.json";
  }

  void Storage::load()
  {
    if (m_loaded)
      return;

    m_loaded = true;

    std::error_code error;
    for (auto const &entry : std::filesystem::directory_iterator(levelsDir(), error))
    {
      if (entry.path().extension() != ".json")
        continue;

      auto json = file::readJson(entry.path());
      auto level = json ? levelFromJson(json.unwrap()) : std::nullopt;

      if (!level)
      {
        log::warn("Skipping unreadable level file {}", entry.path());
        continue;
      }

      // The game closed or crashed mid-session: keep it as it was at the last autosave
      for (auto &session : level->sessions)
        session.inProgress = false;

      m_levels[level->key] = std::move(*level);
    }

    auto groups = file::readJson(groupsPath());
    if (!groups)
      return;

    auto json = groups.unwrap();

    if (auto links = json.get("links"); links && links.unwrap().isObject())
    {
      for (auto const &entry : links.unwrap())
      {
        auto key = entry.getKey();
        auto root = entry.asString();
        if (key && root)
          m_links[*key] = root.unwrap();
      }
    }

    if (auto manual = json.get("manual"); manual && manual.unwrap().isArray())
    {
      for (auto const &entry : manual.unwrap())
      {
        if (auto key = entry.asString())
          m_manual.insert(key.unwrap());
      }
    }
  }

  void Storage::save(std::string const &key) const
  {
    auto it = m_levels.find(key);
    if (it == m_levels.end())
      return;

    (void)file::createDirectoryAll(levelsDir());

    auto result = file::writeStringSafe(levelsDir() / fileName(key), toJson(it->second).dump(matjson::NO_INDENTATION));
    if (!result)
      log::error("Failed to save {}: {}", key, result.unwrapErr());
  }

  void Storage::saveGroups() const
  {
    auto links = matjson::Value::object();
    for (auto const &[key, root] : m_links)
      links.set(key, root);

    std::vector<matjson::Value> manual(m_manual.begin(), m_manual.end());

    auto json = matjson::Value::object();
    json.set("links", links);
    json.set("manual", matjson::Value(std::move(manual)));

    (void)file::createDirectoryAll(Mod::get()->getSaveDir());

    auto result = file::writeStringSafe(groupsPath(), json.dump());
    if (!result)
      log::error("Failed to save groups: {}", result.unwrapErr());
  }

  Level &Storage::touch(GJGameLevel *gameLevel)
  {
    auto key = keyFor(gameLevel);
    auto &level = m_levels[key];

    level.key = key;
    level.name = gameLevel->m_levelName;
    level.creator = gameLevel->m_creatorName;
    level.levelID = gameLevel->m_levelID.value();
    level.originalID = gameLevel->m_originalLevel.value();
    level.local = gameLevel->m_levelType == GJLevelType::Editor;

    relink(key);
    return level;
  }

  Level *Storage::find(std::string const &key)
  {
    auto it = m_levels.find(key);
    return it == m_levels.end() ? nullptr : &it->second;
  }

  std::vector<Level const *> Storage::all()
  {
    std::vector<Level const *> levels;
    for (auto const &[key, level] : m_levels)
      levels.push_back(&level);
    return levels;
  }

  // ! --- Groups --- !

  std::optional<int> Storage::onlineID(std::string const &key)
  {
    if (!key.starts_with("online-"))
      return std::nullopt;

    auto id = numFromString<int>(key.substr(7));
    return id ? std::optional(id.unwrap()) : std::nullopt;
  }

  std::string Storage::rootOf(std::string const &key)
  {
    auto root = key;

    // Links are kept flat, the limit only guards against a hand-edited loop
    for (int i = 0; i < 16; i++)
    {
      auto it = m_links.find(root);
      if (it == m_links.end() || it->second == root)
        break;
      root = it->second;
    }

    return root;
  }

  void Storage::relink(std::string const &key)
  {
    auto level = find(key);

    if (!level || m_manual.contains(key) || !Mod::get()->getSettingValue<bool>("auto-merge-copies"))
      return;

    // A copy goes with the level it was copied from, an uploaded local level with its online version
    std::string autoRoot = key;
    if (level->originalID > 0)
      autoRoot = fmt::format("online-{}", level->originalID);
    else if (level->local && level->levelID > 0)
      autoRoot = fmt::format("online-{}", level->levelID);

    auto root = rootOf(autoRoot);
    auto current = m_links.find(key);

    if (root == key)
    {
      if (current == m_links.end())
        return;
      m_links.erase(current);
    }
    else
    {
      if (current != m_links.end() && current->second == root)
        return;
      m_links[key] = root;
    }

    saveGroups();
  }

  // Every key that takes part in groups: played levels and roots that may not be played yet
  static std::set<std::string> allKeys(std::map<std::string, Level> const &levels, std::map<std::string, std::string> const &links)
  {
    std::set<std::string> keys;
    for (auto const &[key, level] : levels)
      keys.insert(key);
    for (auto const &[key, root] : links)
    {
      keys.insert(key);
      keys.insert(root);
    }
    return keys;
  }

  std::vector<Level const *> Storage::members(std::string const &root)
  {
    std::vector<Level const *> levels;

    for (auto const &[key, level] : m_levels)
    {
      if (rootOf(key) == root)
        levels.push_back(&level);
    }

    std::ranges::sort(levels, [&](Level const *a, Level const *b)
                      {
      if ((a->key == root) != (b->key == root))
        return a->key == root;
      auto firstPlayed = [](Level const *level)
      { return level->sessions.empty() ? INT64_MAX : level->sessions.front().start; };
      return firstPlayed(a) < firstPlayed(b); });

    return levels;
  }

  Totals Storage::totals(std::string const &root)
  {
    Totals totals;

    for (auto level : members(root))
    {
      for (auto const &session : level->sessions)
        totals.add(session);
    }

    return totals;
  }

  std::string Storage::groupName(std::string const &root)
  {
    if (auto level = find(root))
      return level->name;

    auto levels = members(root);
    if (!levels.empty())
      return levels.front()->name;

    return root;
  }

  void Storage::merge(std::string const &key, std::string const &target)
  {
    auto from = rootOf(key);
    auto to = rootOf(target);

    if (from == to)
      return;

    for (auto const &other : allKeys(m_levels, m_links))
    {
      if (other == to || rootOf(other) != from)
        continue;

      m_links[other] = to;
      m_manual.insert(other);
    }

    m_links[from] = to;
    m_manual.insert(from);

    saveGroups();
  }

  void Storage::split(std::string const &key)
  {
    auto root = rootOf(key);

    std::vector<std::string> rest;
    for (auto const &other : allKeys(m_levels, m_links))
    {
      if (other != key && rootOf(other) == root)
        rest.push_back(other);
    }

    // The root leaves: one of the played levels left takes its place
    if (key == root && !rest.empty())
    {
      auto newRoot = rest.front();
      for (auto const &other : rest)
      {
        if (m_levels.contains(other))
        {
          newRoot = other;
          break;
        }
      }

      for (auto const &other : rest)
      {
        if (other == newRoot)
          m_links.erase(other);
        else
          m_links[other] = newRoot;
        m_manual.insert(other);
      }
    }

    m_links.erase(key);
    m_manual.insert(key);

    saveGroups();
  }
}
