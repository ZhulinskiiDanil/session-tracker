#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Text where <small>parts</small> are drawn smaller and dimmer, as in Blitzkrieg
class RichLabel : public CCNode
{
public:
  enum class Variant
  {
    Default,
    Green,
    Orange,
    Red,
    Dim
  };

private:
  Variant m_variant = Variant::Default;

  std::string m_font;
  std::vector<CCLabelBMFont *> m_parts;
  std::string m_text;
  float m_fontSize = .4f;

  void updateColor();
  void parseAndBuild();

  bool init(std::string const &text, std::string const &font, float fontSize);

public:
  static RichLabel *create(std::string const &text, std::string const &font = "gjFont17.fnt", float fontSize = .4f);

  void setText(std::string const &text);
  void setVariant(Variant variant);
  // Scales down to fit `width` when wider
  void limitWidth(float width);
};
