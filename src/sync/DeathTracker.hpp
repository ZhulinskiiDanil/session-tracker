#pragma once

#include <Geode/Geode.hpp>

#include "../data/index.hpp"

using namespace geode::prelude;

// Brings the history of Death Tracker (elohmrow.death_tracker) in: its sessions as sessions here, its linked
// levels as groups. Its files are only ever read
namespace death_tracker
{
  struct ImportResult
  {
    size_t levels = 0;
    size_t sessions = 0;
    // Recorded here too, by both mods at once: the session here is kept
    size_t overlapping = 0;
    size_t links = 0;
  };

  // Death Tracker's levels folder, when it has one
  std::optional<std::filesystem::path> levelsDir();

  bool importing();
  // Reads on a thread, merges on the main thread, then calls `done` there
  void importAll(geode::Function<void(ImportResult)> done);
}
