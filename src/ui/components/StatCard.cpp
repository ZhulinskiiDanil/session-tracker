#include "StatCard.hpp"

char const *cellBackground(StatCard::Style style)
{
  switch (style)
  {
  case StatCard::Style::Current:
    return "range-current-bg.png"_spr;
  case StatCard::Style::Completed:
    return "range-completed-bg.png"_spr;
  default:
    return "range-default-bg.png"_spr;
  }
}

StatCard *StatCard::create(CCSize size, std::string const &value, std::string const &caption, Style style, RichLabel::Variant variant)
{
  auto ret = new StatCard();

  if (ret->init(size, value, caption, style, variant))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool StatCard::init(CCSize size, std::string const &value, std::string const &caption, Style style, RichLabel::Variant variant)
{
  if (!CCNode::init())
    return false;

  setContentSize(size);
  setAnchorPoint({.5f, .5f});

  auto bg = CCScale9Sprite::create(cellBackground(style));
  bg->setContentSize(size);
  addChildAtPosition(bg, Anchor::Center);

  auto valueLabel = RichLabel::create(value, "gjFont17.fnt", .45f);
  valueLabel->setVariant(variant);
  valueLabel->setAnchorPoint({.5f, .5f});
  valueLabel->limitWidth(size.width - 10.f);
  addChildAtPosition(valueLabel, Anchor::Center, {0.f, 4.f});

  auto captionLabel = CCLabelBMFont::create(caption.c_str(), "bigFont.fnt");
  captionLabel->setOpacity(128);
  captionLabel->limitLabelWidth(size.width - 10.f, .22f, .05f);
  addChildAtPosition(captionLabel, Anchor::Bottom, {0.f, 7.f});

  return true;
}
