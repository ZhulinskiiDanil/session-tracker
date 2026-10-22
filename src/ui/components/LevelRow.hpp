#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

struct RowButton
{
  // A frame from the Buttons sheet
  char const *frame;
  geode::Function<void()> callback;
};

// A level in a list, laid out as Blitzkrieg's profile rows: name, a dim line under it, round buttons on the right
class LevelRow : public CCNode
{
private:
  bool init(float width, std::string const &name, std::string const &details, ccColor3B nameColor, std::vector<RowButton> buttons);

public:
  static LevelRow *create(float width, std::string const &name, std::string const &details,
                          ccColor3B nameColor = {255, 255, 255}, std::vector<RowButton> buttons = {});
};
