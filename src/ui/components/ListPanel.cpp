#include "ListPanel.hpp"

#include "RectNode.hpp"

static constexpr float PADDING = 5.f;

ListPanel *ListPanel::create(CCSize size, float gap)
{
  auto ret = new ListPanel();

  if (ret->init(size, gap))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool ListPanel::init(CCSize size, float gap)
{
  if (!CCNode::init())
    return false;

  setContentSize(size);
  setAnchorPoint({.5f, .5f});

  auto bg = RectNode::create(size, ccColor4B{30, 30, 30, 255}, 8.f);
  addChild(bg, -1);

  m_scroll = ScrollLayer::create({size.width - PADDING * 2, size.height - PADDING * 2});
  m_scroll->setPosition({PADDING, PADDING});
  m_scroll->m_contentLayer->setLayout(ColumnLayout::create()
                                          ->setGap(gap)
                                          ->setAxisReverse(true)
                                          ->setAxisAlignment(AxisAlignment::End)
                                          ->setAutoGrowAxis(m_scroll->getContentHeight()));
  addChild(m_scroll);

  auto borders = ListBorders::create();
  borders->setSpriteFrames("list-top.png"_spr, "list-side.png"_spr, 2.f);
  borders->setContentSize({size.width, size.height - 3.f});
  borders->setAnchorPoint({.5f, .5f});
  borders->setPosition({size.width / 2, size.height / 2 - .5f});
  for (auto child : CCArrayExt<CCNodeRGBA *>(borders->getChildren()))
    child->setColor({50, 50, 50});
  addChild(borders, 1);

  m_empty = CCLabelBMFont::create("", "bigFont.fnt");
  m_empty->setScale(.45f);
  m_empty->setOpacity(150);
  m_empty->setVisible(false);
  addChildAtPosition(m_empty, Anchor::Center);

  return true;
}

float ListPanel::innerWidth() const
{
  return m_scroll->getContentWidth();
}

void ListPanel::clear()
{
  m_scroll->m_contentLayer->removeAllChildren();
}

void ListPanel::add(CCNode *node)
{
  m_scroll->m_contentLayer->addChild(node);
}

void ListPanel::finish(bool toTop)
{
  auto content = m_scroll->m_contentLayer;
  float fromTop = content->getPositionY() + content->getContentHeight();

  content->updateLayout();
  m_empty->setVisible(content->getChildrenCount() == 0 && !std::string_view(m_empty->getString()).empty());

  if (toTop)
  {
    m_scroll->scrollToTop();
    return;
  }

  float minY = m_scroll->getContentHeight() - content->getContentHeight();
  content->setPositionY(std::clamp(fromTop - content->getContentHeight(), minY, 0.f));
}

void ListPanel::setEmptyText(std::string const &text)
{
  m_empty->setString(text.c_str());
  m_empty->limitLabelWidth(getContentWidth() - 30.f, .45f, .1f);
}
