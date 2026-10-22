#include "BarChart.hpp"

#include "RichLabel.hpp"
#include "RectNode.hpp"

static constexpr float PADDING = 8.f;
static constexpr float TITLE_HEIGHT = 14.f;
static constexpr float TICKS_HEIGHT = 9.f;
static constexpr float GRID_LINES = 4;

BarChart *BarChart::create(CCSize size, std::string const &title, std::string const &legend, std::vector<ChartBar> const &bars,
                           std::vector<ChartTick> const &ticks, std::optional<ChartTick> marker, std::string const &emptyText)
{
  auto ret = new BarChart();

  if (ret->init(size, title, legend, bars, ticks, std::move(marker), emptyText))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool BarChart::init(CCSize size, std::string const &title, std::string const &legend, std::vector<ChartBar> const &bars,
                    std::vector<ChartTick> const &ticks, std::optional<ChartTick> marker, std::string const &emptyText)
{
  if (!CCNode::init())
    return false;

  setContentSize(size);
  setAnchorPoint({.5f, .5f});

  addChild(RectNode::create(size, ccColor4B{30, 30, 30, 255}, 8.f), -1);

  auto titleLabel = RichLabel::create(title, "gjFont17.fnt", .3f);
  titleLabel->setAnchorPoint({0.f, .5f});
  titleLabel->setPosition({PADDING, size.height - PADDING - 2.f});
  addChild(titleLabel);

  if (!legend.empty())
  {
    auto legendLabel = RichLabel::create(legend, "gjFont17.fnt", .25f);
    legendLabel->setVariant(RichLabel::Variant::Dim);
    legendLabel->setAnchorPoint({1.f, .5f});
    legendLabel->setPosition({size.width - PADDING, size.height - PADDING - 2.f});
    addChild(legendLabel);
  }

  // The plot area
  CCRect plot = {PADDING, PADDING + TICKS_HEIGHT, size.width - PADDING * 2, size.height - PADDING * 2 - TITLE_HEIGHT - TICKS_HEIGHT};
  float max = 0.f;
  for (auto const &bar : bars)
    max = std::max(max, bar.value);

  auto grid = CCDrawNode::create();
  addChild(grid);

  for (int i = 0; i <= GRID_LINES; i++)
  {
    float y = plot.origin.y + plot.size.height * i / GRID_LINES;
    grid->drawSegment({plot.origin.x, y}, {plot.getMaxX(), y}, .3f, ccc4FFromccc4B({50, 50, 50, 255}));
  }

  for (auto const &tick : ticks)
  {
    auto label = CCLabelBMFont::create(tick.text.c_str(), "chatFont.fnt");
    label->setScale(.4f);
    label->setColor({200, 200, 200});
    label->setOpacity(160);
    label->setPosition({plot.origin.x + plot.size.width * tick.at, plot.origin.y - TICKS_HEIGHT / 2 - 1.f});
    addChild(label);
  }

  if (max <= 0.f)
  {
    auto empty = CCLabelBMFont::create(emptyText.c_str(), "bigFont.fnt");
    empty->setScale(.35f);
    empty->setOpacity(120);
    empty->setPosition({plot.getMidX(), plot.getMidY()});
    addChild(empty);
    return true;
  }

  // The biggest value, so the bars can be read off
  auto maxLabel = CCLabelBMFont::create(fmt::format("{:g}", max).c_str(), "chatFont.fnt");
  maxLabel->setScale(.35f);
  maxLabel->setOpacity(140);
  maxLabel->setAnchorPoint({0.f, 0.f});
  maxLabel->setPosition({plot.origin.x + 1.f, plot.getMaxY() + .5f});
  addChild(maxLabel);

  auto barsNode = CCDrawNode::create();
  addChild(barsNode);

  float slot = plot.size.width / static_cast<float>(bars.size());
  float barWidth = std::max(.6f, slot * .7f);

  for (size_t i = 0; i < bars.size(); i++)
  {
    if (bars[i].value <= 0.f)
      continue;

    float height = std::max(1.f, plot.size.height * bars[i].value / max);
    float x = plot.origin.x + slot * i + (slot - barWidth) / 2;
    CCPoint vertices[] = {{x, plot.origin.y}, {x + barWidth, plot.origin.y}, {x + barWidth, plot.origin.y + height}, {x, plot.origin.y + height}};
    barsNode->drawPolygon(vertices, 4, ccc4FFromccc4B(bars[i].color), 0.f, {0.f, 0.f, 0.f, 0.f});
  }

  if (marker)
  {
    float x = plot.origin.x + plot.size.width * marker->at;
    barsNode->drawSegment({x, plot.origin.y}, {x, plot.getMaxY()}, .5f, ccc4FFromccc4B({99, 224, 110, 200}));

    auto label = RichLabel::create(marker->text, "gjFont17.fnt", .25f);
    label->setVariant(RichLabel::Variant::Green);
    // Kept inside the panel near the right edge
    bool flip = marker->at > .85f;
    label->setAnchorPoint({flip ? 1.f : 0.f, .5f});
    label->setPosition({x + (flip ? -2.f : 2.f), plot.getMaxY() - 3.f});
    addChild(label);
  }

  return true;
}
