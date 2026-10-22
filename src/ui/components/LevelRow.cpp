#include "LevelRow.hpp"

#include "RichLabel.hpp"

static constexpr float HEIGHT = 36.f;
static constexpr float BUTTON_SCALE = .6f;

LevelRow *LevelRow::create(float width, std::string const &name, std::string const &details, ccColor3B nameColor,
                           std::vector<RowButton> buttons)
{
  auto ret = new LevelRow();

  if (ret->init(width, name, details, nameColor, std::move(buttons)))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool LevelRow::init(float width, std::string const &name, std::string const &details, ccColor3B nameColor, std::vector<RowButton> buttons)
{
  if (!CCNode::init())
    return false;

  setContentSize({width, HEIGHT});
  setAnchorPoint({.5f, .5f});

  auto bg = CCScale9Sprite::create("square02b_small.png");
  bg->setContentSize(getContentSize());
  bg->setColor({0, 0, 0});
  bg->setOpacity(77);
  addChildAtPosition(bg, Anchor::Center);

  auto menu = CCMenu::create();
  menu->setAnchorPoint({1.f, .5f});
  menu->setContentSize({0.f, HEIGHT});
  menu->setLayout(RowLayout::create()
                      ->setGap(2.5f)
                      ->setAutoScale(false)
                      ->setAutoGrowAxis(0.f)
                      ->setAxisAlignment(AxisAlignment::End));

  for (auto &button : buttons)
  {
    auto item = CCMenuItemExt::createSpriteExtraWithFrameName(button.frame, 1.f, [callback = std::move(button.callback)](auto) mutable
                                                              { callback(); });
    item->setScale(BUTTON_SCALE);
    item->m_baseScale = BUTTON_SCALE;
    menu->addChild(item);
  }

  menu->updateLayout();
  addChildAtPosition(menu, Anchor::Right, {-5.f, 0.f});

  float textWidth = width - 20.f - menu->getContentWidth();

  auto nameLabel = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
  nameLabel->setColor(nameColor);
  nameLabel->setAnchorPoint({0.f, .5f});
  nameLabel->limitLabelWidth(textWidth, .5f, .1f);
  nameLabel->setPosition({10.f, 24.f});
  addChild(nameLabel);

  auto detailsLabel = RichLabel::create(details, "bigFont.fnt", .25f);
  detailsLabel->setVariant(RichLabel::Variant::Dim);
  detailsLabel->setAnchorPoint({0.f, .5f});
  detailsLabel->limitWidth(textWidth);
  detailsLabel->setPosition({10.f, 10.f});
  addChild(detailsLabel);

  return true;
}
