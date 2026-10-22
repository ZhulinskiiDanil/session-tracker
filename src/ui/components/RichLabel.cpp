#include "RichLabel.hpp"

RichLabel *RichLabel::create(std::string const &text, std::string const &font, float fontSize)
{
  auto ret = new RichLabel();

  if (ret->init(text, font, fontSize))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool RichLabel::init(std::string const &text, std::string const &font, float fontSize)
{
  if (!CCNode::init())
    return false;

  m_fontSize = fontSize;
  m_font = font;
  m_text = text;

  setAnchorPoint({0.f, .5f});
  setLayout(RowLayout::create()
                ->setGap(0.f)
                ->setAxisAlignment(AxisAlignment::Start)
                ->setCrossAxisAlignment(AxisAlignment::Center)
                ->setCrossAxisLineAlignment(AxisAlignment::Start)
                ->setAutoScale(false)
                ->setAutoGrowAxis(true)
                ->setGrowCrossAxis(false));

  parseAndBuild();
  updateColor();

  return true;
}

void RichLabel::setText(std::string const &text)
{
  m_text = text;
  parseAndBuild();
  updateColor();
}

void RichLabel::setVariant(Variant variant)
{
  m_variant = variant;
  updateColor();
}

void RichLabel::limitWidth(float width)
{
  setScale(1.f);
  if (getContentWidth() > width)
    setScale(width / getContentWidth());
}

void RichLabel::updateColor()
{
  ccColor3B color;
  GLubyte opacity = 255;

  switch (m_variant)
  {
  case Variant::Green:
    color = {99, 224, 110};
    break;
  case Variant::Orange:
    color = {255, 196, 157};
    break;
  case Variant::Red:
    color = {253, 106, 106};
    break;
  case Variant::Dim:
    color = {255, 255, 255};
    opacity = 128;
    break;
  default:
    color = {255, 255, 255};
  }

  for (auto part : m_parts)
  {
    part->setColor(color);
    // <small> parts stay dimmer than the rest
    part->setOpacity(static_cast<GLubyte>(part->getTag() == 1 ? opacity * .7f : opacity));
  }
}

void RichLabel::parseAndBuild()
{
  for (auto part : m_parts)
    part->removeFromParent();
  m_parts.clear();

  auto addPart = [this](std::string const &text, bool isSmall)
  {
    if (text.empty())
      return;

    auto label = CCLabelBMFont::create(text.c_str(), m_font.c_str());
    label->setScale(isSmall ? m_fontSize * .75f : m_fontSize);
    label->setAnchorPoint({0.f, .5f});
    label->setTag(isSmall ? 1 : 0);

    addChild(label);
    m_parts.push_back(label);
  };

  size_t pos = 0;
  bool inSmall = false;

  while (pos < m_text.size())
  {
    auto tag = inSmall ? "</small>" : "<small>";
    size_t found = m_text.find(tag, pos);

    if (found == std::string::npos)
    {
      addPart(m_text.substr(pos), inSmall);
      break;
    }

    addPart(m_text.substr(pos, found - pos), inSmall);
    pos = found + std::strlen(tag);
    inSmall = !inSmall;
  }

  updateLayout();
}
