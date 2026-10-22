#include "MetaTable.hpp"

#include "RichLabel.hpp"

static constexpr float ROW_GAP = 2.5f;
static constexpr float LINE_GAP = 5.f;

MetaTable *MetaTable::create(std::vector<MetaRow> const &rows, float width, float padding, float fontSize)
{
  auto ret = new MetaTable();

  if (ret->init(rows, width, padding, fontSize))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool MetaTable::init(std::vector<MetaRow> const &rows, float width, float padding, float fontSize)
{
  if (!CCNode::init())
    return false;

  float inner = width - padding * 2;
  float y = padding;

  // Bottom up, so the first row ends on top
  for (auto it = rows.rbegin(); it != rows.rend(); ++it)
  {
    auto label = RichLabel::create(it->label, "gjFont17.fnt", fontSize);
    auto value = RichLabel::create(it->value, "gjFont17.fnt", fontSize);
    label->setVariant(RichLabel::Variant::Dim);

    // A long value takes what it needs, the label gives way
    value->limitWidth(inner * .7f);
    label->limitWidth(inner - value->getScaledContentWidth() - LINE_GAP * 2);

    float height = std::max(label->getScaledContentHeight(), value->getScaledContentHeight());
    float center = y + height / 2;

    label->setAnchorPoint({0.f, .5f});
    label->setPosition({padding, center});
    addChild(label);

    value->setAnchorPoint({1.f, .5f});
    value->setPosition({width - padding, center});
    addChild(value);

    float lineStart = padding + label->getScaledContentWidth() + LINE_GAP;
    float lineWidth = width - padding - value->getScaledContentWidth() - LINE_GAP - lineStart;
    if (lineWidth > 0.f)
    {
      auto line = CCLayerColor::create({255, 255, 255, 10}, lineWidth, 1.f);
      line->setPosition({lineStart, center - .5f});
      addChild(line);
    }

    y += height + ROW_GAP;
  }

  setContentSize({width, y - ROW_GAP + padding});
  return true;
}
