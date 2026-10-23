#include "DeathTracker.hpp"

#include <atomic>
#include <thread>

namespace death_tracker
{
  static constexpr char const *MOD_ID = "elohmrow.death_tracker";
  static constexpr char const *SOURCE = "death-tracker";
  // A session there and one here this close in time are the same play, recorded by both mods
  static constexpr int64_t OVERLAP_SLACK = 60;

  static std::atomic_bool s_importing = false;

  // ! --- Reading Death Tracker's files --- !

  // What Death Tracker counts, in a general.dt or a session
  struct Counts
  {
    // From 0%: percent -> deaths, 100 being a completion
    std::map<int, int> deaths;
    // From a start position: (start, end) -> runs
    std::map<std::pair<int, int>, int> runs;
    std::set<int> newBests;
    double activeSeconds = 0;
    double pausedSeconds = 0;

    bool empty() const { return deaths.empty() && runs.empty() && activeSeconds < 1.0; }
  };

  struct ParsedSession
  {
    int64_t start = 0;
    int64_t lastPlayed = 0;
    Counts counts;
  };

  struct ParsedLevel
  {
    std::string dtKey;
    // The key here, empty when there is no telling which level it is
    std::string key;
    std::string name;
    int levelID = 0;
    bool local = false;
    std::vector<ParsedSession> sessions;
    Counts general;
    int64_t generalWritten = 0;
    std::vector<std::string> links;
  };

  std::optional<std::filesystem::path> levelsDir()
  {
    auto saveDir = Mod::get()->getSaveDir().parent_path() / MOD_ID;
    std::vector<std::filesystem::path> candidates;

    // Its "Save Path" setting can move the folder anywhere
    if (auto settings = file::readJson(saveDir / "settings.json"))
    {
      auto path = settings.unwrap().get<std::string>("save-path-new");
      if (path && !path.unwrap().empty())
        candidates.push_back(std::filesystem::path(utils::string::utf8ToWide(path.unwrap())));
    }
    candidates.push_back(saveDir / "levels");

    std::error_code error;
    for (auto const &path : candidates)
    {
      if (std::filesystem::is_directory(path, error))
        return path;
    }

    return std::nullopt;
  }

  static double nanoseconds(matjson::Value const &json, std::string_view key)
  {
    auto pair = json.get(key);
    if (!pair)
      return 0;

    auto const &value = pair.unwrap();
    return value.get<double>("playtimeF0").unwrapOr(0) + value.get<double>("playtimeRuns").unwrapOr(0);
  }

  static Counts readCounts(matjson::Value const &json)
  {
    Counts counts;

    if (auto deaths = json.get("deaths"); deaths && deaths.unwrap().isObject())
    {
      for (auto const &entry : deaths.unwrap())
      {
        auto percent = numFromString<int>(entry.getKey().value_or(""));
        int count = entry.as<int>().unwrapOr(0);
        if (percent && count > 0)
          counts.deaths[std::clamp(percent.unwrap(), 0, 100)] += count;
      }
    }

    if (auto runs = json.get("runs"); runs && runs.unwrap().isObject())
    {
      for (auto const &entry : runs.unwrap())
      {
        auto key = entry.getKey().value_or("");
        auto dash = key.find('-');
        int count = entry.as<int>().unwrapOr(0);
        if (dash == std::string::npos || count <= 0)
          continue;

        auto start = numFromString<int>(key.substr(0, dash));
        auto end = numFromString<int>(key.substr(dash + 1));
        if (start && end)
          counts.runs[{std::clamp(start.unwrap(), 0, 100), std::clamp(end.unwrap(), 0, 100)}] += count;
      }
    }

    if (auto bests = json.get("newBests"); bests && bests.unwrap().isArray())
    {
      for (auto const &entry : bests.unwrap())
        counts.newBests.insert(std::clamp(static_cast<int>(entry.as<int>().unwrapOr(0)), 0, 100));
    }

    // Time alive and time dead before the respawn are both play; nanoseconds there
    counts.activeSeconds = (nanoseconds(json, "playtimeGeneral") + nanoseconds(json, "playtimeDead")) / 1e9;
    counts.pausedSeconds = nanoseconds(json, "playtimePaused") / 1e9;

    return counts;
  }

  // `123` online, `123-local` a main level, `17-editor` a local one, any of them with `-daily` or `-gauntlet`
  static void resolveKey(ParsedLevel &level)
  {
    auto key = level.dtKey;

    for (auto suffix : {"-daily", "-gauntlet"})
    {
      if (key.ends_with(suffix))
        key.resize(key.size() - std::strlen(suffix));
    }

    // Its editor IDs are its own: the name is what matches a local level here
    if (key.ends_with("-editor"))
    {
      if (!level.name.empty() && level.name != "-1")
      {
        level.key = fmt::format("local-{}", level.name);
        level.local = true;
      }
      return;
    }

    bool main = key.ends_with("-local");
    if (main)
      key.resize(key.size() - 6);

    auto id = numFromString<int>(key);
    if (!id || id.unwrap() <= 0)
      return;

    level.levelID = id.unwrap();
    level.key = fmt::format("{}-{}", main ? "main" : "online", level.levelID);
  }

  static int64_t writeTime(std::filesystem::path const &path)
  {
    std::error_code error;
    auto time = std::filesystem::last_write_time(path, error);
    if (error)
      return 0;

    auto system = std::chrono::clock_cast<std::chrono::system_clock>(time);
    return std::chrono::duration_cast<std::chrono::seconds>(system.time_since_epoch()).count();
  }

  static std::optional<ParsedLevel> readLevel(std::filesystem::path const &dir)
  {
    ParsedLevel level;
    level.dtKey = utils::string::pathToString(dir.filename());

    if (auto metadata = file::readJson(dir / "metadata"))
    {
      auto const &json = metadata.unwrap();
      level.name = json.get<std::string>("levelName").unwrapOr("");

      if (auto links = json.get("LinkedLevels"); links && links.unwrap().isArray())
      {
        for (auto const &link : links.unwrap())
        {
          if (auto key = link.asString())
            level.links.push_back(key.unwrap());
        }
      }
    }

    resolveKey(level);
    if (level.key.empty())
      return std::nullopt;

    if (auto general = file::readJson(dir / "general.dt"))
    {
      level.general = readCounts(general.unwrap());
      level.generalWritten = writeTime(dir / "general.dt");
    }

    // Only the live sessions: backups/ keeps old copies of the same ones
    std::error_code error;
    for (auto const &entry : std::filesystem::directory_iterator(dir / "sessions", error))
    {
      if (entry.path().extension() != ".dt")
        continue;

      auto json = file::readJson(entry.path());
      if (!json)
        continue;

      auto const &value = json.unwrap();
      ParsedSession session;
      session.start = value.get<int64_t>("sessionStartDate").unwrapOr(0);
      session.lastPlayed = value.get<int64_t>("lastPlayed").unwrapOr(0);
      // Older files keep the counts at the top, newer ones under "data"
      session.counts = readCounts(value.contains("data") ? value["data"] : value);

      if (session.start > 0 && !session.counts.empty())
        level.sessions.push_back(std::move(session));
    }

    return level;
  }

  static std::vector<ParsedLevel> readAll(std::filesystem::path const &dir)
  {
    std::vector<ParsedLevel> levels;

    std::error_code error;
    for (auto const &entry : std::filesystem::directory_iterator(dir, error))
    {
      if (!entry.is_directory())
        continue;

      if (auto level = readLevel(entry.path()))
        levels.push_back(std::move(*level));
    }

    return levels;
  }

  // ! --- Into sessions here --- !

  static data::Session toSession(Counts const &counts)
  {
    data::Session session;

    for (auto const &[percent, count] : counts.deaths)
    {
      session.attempts += count;
      if (percent >= 100)
        session.completions += count;
      else
      {
        session.deaths[percent] += count;
        session.best = std::max(session.best, percent);
      }
    }

    for (auto const &[run, count] : counts.runs)
    {
      session.attempts += count;
      session.startPosAttempts += count;
      if (run.second < 100)
        session.deaths[run.second] += count;
    }

    // The bests it noted, from 0% only
    for (int best : counts.newBests)
      session.best = std::max(session.best, std::min(best, 100));
    if (session.completions > 0)
      session.best = 100;

    session.activeSeconds = counts.activeSeconds;
    session.pausedSeconds = counts.pausedSeconds;
    session.source = SOURCE;

    return session;
  }

  // What general.dt counts beyond all the sessions: the time before Death Tracker kept sessions
  static Counts remainder(ParsedLevel const &level)
  {
    Counts rest = level.general;

    for (auto const &session : level.sessions)
    {
      for (auto const &[percent, count] : session.counts.deaths)
        rest.deaths[percent] -= count;
      for (auto const &[run, count] : session.counts.runs)
        rest.runs[run] -= count;
      rest.activeSeconds -= session.counts.activeSeconds;
      rest.pausedSeconds -= session.counts.pausedSeconds;
    }

    std::erase_if(rest.deaths, [](auto const &entry)
                  { return entry.second <= 0; });
    std::erase_if(rest.runs, [](auto const &entry)
                  { return entry.second <= 0; });
    rest.activeSeconds = std::max(0.0, rest.activeSeconds);
    rest.pausedSeconds = std::max(0.0, rest.pausedSeconds);

    // Its bests belong to the sessions they were made in
    rest.newBests.clear();
    for (auto const &[percent, count] : rest.deaths)
      if (percent < 100)
        rest.newBests.insert(percent);

    return rest;
  }

  static std::vector<data::Session> toSessions(ParsedLevel const &level)
  {
    std::vector<data::Session> sessions;
    int64_t firstStart = INT64_MAX;

    for (auto const &parsed : level.sessions)
    {
      auto session = toSession(parsed.counts);
      session.start = parsed.start;
      // lastPlayed is -1 or -2 until it was played
      session.end = parsed.lastPlayed > parsed.start
                        ? parsed.lastPlayed
                        : parsed.start + static_cast<int64_t>(session.activeSeconds + session.pausedSeconds);
      session.importID = fmt::format("{}/{}", level.dtKey, parsed.start);

      firstStart = std::min(firstStart, parsed.start);
      sessions.push_back(std::move(session));
    }

    auto rest = remainder(level);
    if (!rest.empty())
    {
      auto session = toSession(rest);
      auto length = static_cast<int64_t>(session.activeSeconds + session.pausedSeconds);

      // Undated: it ends where the first session starts, or when general.dt was written
      session.end = firstStart != INT64_MAX ? firstStart - 1 : (level.generalWritten > 0 ? level.generalWritten : data::now());
      session.start = session.end - length;
      session.importID = fmt::format("{}/earlier", level.dtKey);
      sessions.push_back(std::move(session));
    }

    return sessions;
  }

  static bool overlaps(data::Session const &a, data::Session const &b)
  {
    return a.start <= b.end + OVERLAP_SLACK && a.end >= b.start - OVERLAP_SLACK;
  }

  static ImportResult apply(std::vector<ParsedLevel> const &levels)
  {
    auto &storage = data::Storage::get();
    ImportResult result;
    std::map<std::string, std::string> keys;

    for (auto const &parsed : levels)
    {
      keys[parsed.dtKey] = parsed.key;

      auto sessions = toSessions(parsed);
      if (sessions.empty())
        continue;

      auto &level = storage.ensure(parsed.key, parsed.name, parsed.levelID, parsed.local);
      bool changed = false;

      for (auto &session : sessions)
      {
        bool recordedHere = std::ranges::any_of(level.sessions, [&](data::Session const &own)
                                                { return own.source.empty() && overlaps(own, session); });
        if (recordedHere)
        {
          result.overlapping++;
          continue;
        }

        // Sessions are only replaced or added, never moved: the tracker holds an index into the list
        auto existing = std::ranges::find_if(level.sessions, [&](data::Session const &own)
                                             { return own.importID == session.importID; });
        if (existing != level.sessions.end())
          *existing = std::move(session);
        else
          level.sessions.push_back(std::move(session));

        result.sessions++;
        changed = true;
      }

      if (changed)
      {
        storage.save(parsed.key);
        result.levels++;
      }
    }

    for (auto const &parsed : levels)
    {
      for (auto const &link : parsed.links)
      {
        auto other = keys.find(link);
        if (other != keys.end() && !other->second.empty() && storage.linkOnce(parsed.key, other->second))
          result.links++;
      }
    }

    return result;
  }

  // ! --- Running it --- !

  bool importing()
  {
    return s_importing;
  }

  void importAll(geode::Function<void(ImportResult)> done)
  {
    auto dir = levelsDir();
    if (!dir || s_importing.exchange(true))
    {
      done({});
      return;
    }

    std::thread([dir = *dir, done = std::move(done)]() mutable
                {
                  auto levels = readAll(dir);
                  log::info("Read {} levels from Death Tracker", levels.size());

                  Loader::get()->queueInMainThread([levels = std::move(levels), done = std::move(done)]() mutable
                                                   {
                                                     auto result = apply(levels);
                                                     s_importing = false;
                                                     log::info("Imported {} sessions on {} levels, {} links, {} already here",
                                                               result.sessions, result.levels, result.links, result.overlapping);
                                                     done(result); }); })
        .detach();
  }
}
