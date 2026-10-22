#include "SessionCell.hpp"

#include "../format.hpp"
#include "RichLabel.hpp"
#include "MetaTable.hpp"
#include "RectNode.hpp"
#include "StatCard.hpp"

static constexpr float HEAD_HEIGHT = 30.f;
static constexpr float LINE_PADDING = 5.f;

SessionCell *SessionCell::create(float width, data::Session const &session, bool platformer, std::string levelText, bool live,
                                 geode::Function<void()> onResize)
{
  auto ret = new SessionCell();

  if (ret->init(width, session, platformer, std::move(levelText), live, std::move(onResize)))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool SessionCell::init(float width, data::Session const &session, bool platformer, std::string levelText, bool live,
                       geode::Function<void()> onResize)
{
  if (!CCNode::init())
    return false;

  m_session = session;
  m_platformer = platformer;
  m_levelText = std::move(levelText);
  m_live = live;
  m_width = width;
  m_onResize = std::move(onResize);

  setAnchorPoint({.5f, .5f});
  build();

  return true;
}

void SessionCell::build()
{
  removeAllChildren();

  auto head = createHead();
  auto details = m_expanded ? createDetails() : nullptr;
  float height = HEAD_HEIGHT + (details ? details->getContentHeight() : 0.f);

  setContentSize({m_width, height});

  auto style = m_live ? StatCard::Style::Current : m_session.completions > 0 ? StatCard::Style::Completed : StatCard::Style::Default;
  auto bg = CCScale9Sprite::create(cellBackground(style));
  bg->setContentSize(getContentSize());
  addChildAtPosition(bg, Anchor::Center);

  head->setPosition({0.f, height - HEAD_HEIGHT});
  addChild(head, 1);

  if (details)
    addChild(details, 1);
}

CCNode *SessionCell::createHead()
{
  auto head = CCNode::create();
  head->setContentSize({m_width, HEAD_HEIGHT});

  auto date = RichLabel::create(fmt_time::richDate(m_session.start), "gjFont17.fnt", .4f);
  date->setVariant(m_live ? RichLabel::Variant::Orange : m_session.completions > 0 ? RichLabel::Variant::Green : RichLabel::Variant::Default);
  date->setPosition({8.f, HEAD_HEIGHT / 2 + 1.f});
  head->addChild(date);

  auto activity = RichLabel::create(fmt::format("{}  {}", fmt_time::count(m_session.attempts, "attempt", "attempts"),
                                            fmt_time::rich(m_session.activeSeconds)),
                                "gjFont17.fnt", .32f);
  activity->setPosition({std::max(88.f, date->getScaledContentWidth() + 16.f), HEAD_HEIGHT / 2 + 1.f});
  head->addChild(activity);

  // Collapsed and expanded, as Blitzkrieg's chevron
  auto chevron = CCMenuItemExt::createSpriteExtraWithFrameName("purple-chevron-up-btn.png"_spr, .5f, [this](auto)
                                                               {
                                                                 m_expanded = !m_expanded;
                                                                 // Deferred: the rebuild removes this very button
                                                                 Loader::get()->queueInMainThread([self = Ref(this)]
                                                                                                  {
                                                                   self->build();
                                                                   if (self->m_onResize)
                                                                     self->m_onResize(); }); });
  chevron->setRotation(m_expanded ? 0.f : 180.f);

  auto menu = CCMenu::create();
  menu->setContentSize(chevron->getScaledContentSize());
  menu->setAnchorPoint({1.f, .5f});
  menu->addChildAtPosition(chevron, Anchor::Center);
  head->addChildAtPosition(menu, Anchor::Right, {-6.f, 1.f});

  float right = m_width - 6.f - menu->getContentWidth() - 6.f;

  if (!m_platformer)
  {
    auto best = RichLabel::create(m_session.completions > 0 ? "100<small>%</small>" : fmt::format("{}<small>%</small>", m_session.best),
                              "gjFont17.fnt", .4f);
    best->setVariant(m_session.completions > 0 ? RichLabel::Variant::Green : RichLabel::Variant::Default);
    best->setAnchorPoint({1.f, .5f});
    best->setPosition({right, HEAD_HEIGHT / 2 + 1.f});
    head->addChild(best);
  }

  if (m_live)
  {
    auto now = RichLabel::create("NOW", "bigFont.fnt", .25f);
    now->setVariant(RichLabel::Variant::Orange);
    now->setAnchorPoint({1.f, .5f});
    now->setPosition({right - (m_platformer ? 0.f : 32.f), HEAD_HEIGHT / 2 + 1.f});
    head->addChild(now);
  }

  // How far the best run got, on the line under the cell's head
  float lineWidth = m_width - LINE_PADDING * 2;
  auto line = RectNode::create({lineWidth, 2.f}, ccColor4B{54, 54, 54, 255}, 1.f);
  line->setPosition({LINE_PADDING, 3.f});
  head->addChild(line);

  int best = m_session.completions > 0 ? 100 : m_session.best;
  if (!m_platformer && best > 0)
  {
    auto color = m_session.completions > 0 ? ccColor4B{98, 240, 70, 255} : m_live ? ccColor4B{158, 145, 77, 255} : ccColor4B{201, 201, 201, 255};
    auto fill = RectNode::create({lineWidth * best / 100.f, 2.f}, color, 1.f);
    fill->setPosition({LINE_PADDING, 3.f});
    head->addChild(fill);
  }

  return head;
}

CCNode *SessionCell::createDetails()
{
  auto const &s = m_session;
  std::vector<MetaRow> rows;

  auto from = fmt_time::local(s.start);
  auto to = fmt_time::local(s.end);
  rows.push_back({"Time:", fmt::format("{:02}:{:02} - {:02}:{:02}", from.tm_hour, from.tm_min, to.tm_hour, to.tm_min)});

  auto played = fmt_time::rich(s.activeSeconds);
  if (s.pausedSeconds >= 1.0)
    played += fmt::format(" <small>+</small> {} <small>paused</small>", fmt_time::rich(s.pausedSeconds));
  rows.push_back({"Played:", played});

  auto attempts = fmt_time::count(s.attempts, "attempt", "attempts");
  if (s.startPosAttempts > 0)
    attempts += fmt::format(" / {} <small>from start pos</small>", s.startPosAttempts);
  if (s.practiceAttempts > 0)
    attempts += fmt::format(" / {} <small>practice</small>", s.practiceAttempts);
  rows.push_back({"Attempts:", attempts});

  if (!m_platformer)
  {
    rows.push_back({"Best Run:", s.completions > 0 ? fmt::format("<small>completed</small> {}<small>x</small>", s.completions)
                                                   : fmt::format("{}<small>%</small>", s.best)});

    data::Totals totals;
    totals.add(s);
    auto hardest = totals.hardest(3);

    std::string deaths = hardest.empty() ? "<small>None</small>" : "";
    for (auto const &[percent, count] : hardest)
      deaths += fmt::format("{}{}<small>% x{}</small>", deaths.empty() ? "" : "  ", percent, count);
    rows.push_back({"Most Deaths:", deaths});
  }

  if (!m_levelText.empty())
    rows.push_back({"Level:", m_levelText});

  return MetaTable::create(rows, m_width, 8.f);
}
