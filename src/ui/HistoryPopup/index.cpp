#include "index.hpp"

#include <Geode/ui/GeodeUI.hpp>

#include "../../sync/DeathTracker.hpp"
#include "../../tracker/index.hpp"
#include "../MergePopup/index.hpp"
#include "../components/BarChart.hpp"
#include "../components/RichLabel.hpp"
#include "../components/LevelRow.hpp"
#include "../components/MetaTable.hpp"
#include "../components/RectNode.hpp"
#include "../components/SessionCell.hpp"
#include "../components/StatCard.hpp"
#include "../format.hpp"

using namespace data;

static constexpr float WIDTH = 420.f;
static constexpr float HEIGHT = 260.f;
static constexpr float PADDING = 10.f;
// Where the tab's content starts, under its header
static constexpr float CONTENT_TOP = HEIGHT - 50.f;
static constexpr float CARD_HEIGHT = 40.f;
static constexpr float GAP = 6.f;
static constexpr float BOTTOM_BAR = 30.f;
// Older sessions are left out of the list, they still count in the totals and the export
static constexpr size_t MAX_SESSION_ROWS = 300;
static constexpr size_t MAX_CHART_SESSIONS = 40;

HistoryPopup *HistoryPopup::create(std::string key, Tab tab)
{
  auto ret = new HistoryPopup();

  if (ret->init(std::move(key), tab))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool HistoryPopup::init(std::string key, Tab tab)
{
  if (!Popup::init(WIDTH, HEIGHT, "GJ_square01_custom.png"_spr))
    return false;

  m_key = std::move(key);

  // The session being played shows in the lists too
  tracker::Tracker::get().flush();

  m_tab = tab == Tab::Session && !liveHere() ? Tab::Overview : tab;

  drawTabs();
  drawContent();

  // Settings in the corner, as in Blitzkrieg
  auto settingsSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
  settingsSpr->setScale(.8f);
  auto settingsBtn = CCMenuItemExt::createSpriteExtra(settingsSpr, [](auto)
                                                      { openSettingsPopup(Mod::get(), false); });
  settingsBtn->setPosition({m_bgSprite->getContentWidth() - 3.f, 3.f});
  m_closeBtn->getParent()->addChild(settingsBtn);

  return true;
}

// ! --- Data --- !

std::string HistoryPopup::root()
{
  return Storage::get().rootOf(m_key);
}

bool HistoryPopup::grouped()
{
  auto groupRoot = root();
  auto members = Storage::get().members(groupRoot);
  return members.size() > 1 || (members.size() == 1 && members.front()->key != groupRoot);
}

bool HistoryPopup::liveHere()
{
  auto &tracker = tracker::Tracker::get();
  return tracker.active() && Storage::get().rootOf(tracker.key()) == root();
}

std::vector<Level const *> HistoryPopup::shownLevels()
{
  if (m_wholeGroup)
    return Storage::get().members(root());

  auto level = Storage::get().find(m_key);
  return level ? std::vector<Level const *>{level} : std::vector<Level const *>{};
}

std::string HistoryPopup::tagFor(Level const *level)
{
  auto groupRoot = root();

  if (level->key == groupRoot)
    return "Main";

  size_t number = 0;
  for (auto member : Storage::get().members(groupRoot))
  {
    if (member->key == groupRoot)
      continue;
    number++;
    if (member == level)
      break;
  }

  return fmt::format("{} #{}", level->originalID > 0 ? "Copy" : "Linked", number);
}

// ! --- Tabs --- !

void HistoryPopup::drawTabs()
{
  std::vector<std::pair<Tab, char const *>> tabs;
  if (liveHere())
    tabs.push_back({Tab::Session, "Session"});
  tabs.push_back({Tab::Overview, "Overview"});
  tabs.push_back({Tab::Sessions, "Sessions"});
  tabs.push_back({Tab::Levels, "Levels"});

  auto tabsNode = CCNode::create();
  tabsNode->setID("tabs"_spr);
  m_mainLayer->addChild(tabsNode, 1);

  auto menu = CCMenu::create();
  menu->setPosition({m_size.width / 2, m_size.height - 3.5f});
  tabsNode->addChild(menu, 1);

  for (auto const &[tab, name] : tabs)
  {
    auto button = TabButton::create(name, this, menu_selector(HistoryPopup::onTab));
    button->setAnchorPoint({.5f, 0.f});
    button->setTag(static_cast<int>(tab));
    button->toggle(tab == m_tab);

    menu->addChild(button);
    m_tabs.push_back({tab, button});
  }

  menu->alignItemsHorizontallyWithPadding(2.f);

  for (auto const &[tab, button] : m_tabs)
  {
    auto gradient = CCSprite::create("tab-gradient-mask.png"_spr);
    gradient->setAnchorPoint({.5f, 0.f});
    gradient->setPosition(menu->getPosition() + button->getPosition());
    gradient->setColor({82, 82, 82});
    tabsNode->addChild(gradient, 0);
  }
}

void HistoryPopup::onTab(CCObject *sender)
{
  auto tab = static_cast<Tab>(sender->getTag());

  // Selected after the toggler is done flipping itself
  Loader::get()->queueInMainThread([self = Ref(this), tab]
                                   { self->selectTab(tab); });
}

void HistoryPopup::selectTab(Tab tab)
{
  m_tab = tab;

  for (auto const &[other, button] : m_tabs)
    button->toggle(other == tab);

  drawContent();
}

void HistoryPopup::drawContent()
{
  if (m_content)
    m_content->removeFromParent();

  m_content = CCNode::create();
  m_content->setID("content"_spr);
  m_mainLayer->addChild(m_content);
  m_list = nullptr;

  switch (m_tab)
  {
  case Tab::Session:
    drawSession();
    break;
  case Tab::Overview:
    drawOverview();
    break;
  case Tab::Sessions:
    drawSessions();
    break;
  case Tab::Levels:
    drawLevels();
    break;
  }
}

// ! --- Shared pieces --- !

void HistoryPopup::drawHeader(std::string const &title, std::string const &subtitle)
{
  auto titleLabel = CCLabelBMFont::create(title.c_str(), "goldFont.fnt");
  titleLabel->setAnchorPoint({0.f, .5f});
  titleLabel->limitLabelWidth(m_size.width * .6f, .75f, .2f);
  titleLabel->setPosition({PADDING + 5.f, m_size.height - 22.f});
  m_content->addChild(titleLabel);

  auto subtitleLabel = RichLabel::create(subtitle, "bigFont.fnt", .4f);
  subtitleLabel->setAnchorPoint({0.f, .5f});
  subtitleLabel->limitWidth(m_size.width * .62f);
  subtitleLabel->setPosition({PADDING + 6.f, m_size.height - 38.f});
  m_content->addChild(subtitleLabel);
}

void HistoryPopup::drawFilters(bool sort)
{
  auto menu = CCMenu::create();
  menu->ignoreAnchorPointForPosition(false);
  menu->setAnchorPoint({1.f, .5f});
  menu->setContentSize({0.f, 30.f});
  menu->setLayout(RowLayout::create()
                      ->setGap(5.f)
                      ->setAutoScale(false)
                      ->setAutoGrowAxis(0.f)
                      ->setAxisAlignment(AxisAlignment::End));
  menu->setPosition({m_size.width - PADDING, m_size.height - 30.f});
  m_content->addChild(menu);

  if (grouped())
  {
    auto label = CCLabelBMFont::create("Whole group", "bigFont.fnt");
    label->setScale(.3f);
    label->setOpacity(150);
    menu->addChild(label);

    auto scope = CCMenuItemExt::createToggler(CCSprite::createWithSpriteFrameName("check-mark-square-btn.png"_spr),
                                              CCSprite::createWithSpriteFrameName("check-mark-gray-square-btn.png"_spr),
                                              [this](auto)
                                              {
                                                m_wholeGroup = !m_wholeGroup;
                                                Loader::get()->queueInMainThread([self = Ref(this)]
                                                                                 { self->drawContent(); }); });
    scope->toggle(m_wholeGroup);
    scope->setScale(.6f);
    menu->addChild(scope);
  }

  if (sort)
  {
    auto order = CCMenuItemExt::createToggler(CCSprite::createWithSpriteFrameName("sort-down-square-btn.png"_spr),
                                              CCSprite::createWithSpriteFrameName("sort-up-square-btn.png"_spr),
                                              [this](auto)
                                              {
                                                m_newestFirst = !m_newestFirst;
                                                fillSessions(); });
    order->toggle(m_newestFirst);
    order->setScale(.6f);
    menu->addChild(order);
  }

  menu->updateLayout();
}

void HistoryPopup::drawCards(std::vector<CCNode *> const &cards, float top)
{
  float width = m_size.width - PADDING * 2;
  float cardWidth = (width - GAP * (cards.size() - 1)) / cards.size();

  for (size_t i = 0; i < cards.size(); i++)
  {
    cards[i]->setPosition({PADDING + cardWidth / 2 + (cardWidth + GAP) * i, top - CARD_HEIGHT / 2});
    m_content->addChild(cards[i]);
  }
}

static CCNode *card(float width, std::string const &value, std::string const &caption,
                    StatCard::Style style = StatCard::Style::Default, RichLabel::Variant variant = RichLabel::Variant::Default)
{
  return StatCard::create({width, CARD_HEIGHT}, value, caption, style, variant);
}

static float cardWidth(size_t count)
{
  return (WIDTH - PADDING * 2 - GAP * (count - 1)) / count;
}

CCNode *HistoryPopup::createDeathsChart(Totals const &totals, CCSize size, std::string const &title)
{
  std::vector<ChartBar> bars(101);

  int total = 0;
  for (auto const &[percent, count] : totals.deaths)
  {
    bars[std::clamp(percent, 0, 100)].value = static_cast<float>(count);
    total += count;
  }

  for (auto &bar : bars)
    bar.color = {170, 170, 170, 255};

  // The hardest spots in Blitzkrieg's red
  for (auto const &[percent, count] : totals.hardest(3))
    bars[std::clamp(percent, 0, 100)].color = {218, 80, 80, 255};

  std::vector<ChartTick> ticks;
  for (int percent = 0; percent <= 100; percent += 25)
    ticks.push_back({(percent + .5f) / 101.f, fmt::format("{}%", percent)});

  std::optional<ChartTick> marker;
  int best = totals.completions > 0 ? 100 : totals.best;
  if (best > 0)
    marker = ChartTick{(best + .5f) / 101.f, fmt::format("best {}<small>%</small>", best)};

  return BarChart::create(size, title, fmt_time::count(total, "death", "deaths"), bars, ticks, marker, "No deaths yet");
}

static std::string shortDate(int64_t unix)
{
  static constexpr char const *MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  auto tm = fmt_time::local(unix);
  return fmt::format("{} {}", tm.tm_mday, MONTHS[tm.tm_mon]);
}

CCNode *HistoryPopup::createTimeChart(std::vector<Session const *> const &sessions, CCSize size)
{
  std::vector<Session const *> recent = sessions;
  std::ranges::sort(recent, [](auto a, auto b)
                    { return a->start < b->start; });
  if (recent.size() > MAX_CHART_SESSIONS)
    recent.erase(recent.begin(), recent.end() - MAX_CHART_SESSIONS);

  std::vector<ChartBar> bars;
  for (auto session : recent)
  {
    ChartBar bar;
    bar.value = std::round(static_cast<float>(session->activeSeconds) / 60.f * 10.f) / 10.f;
    bar.color = session->inProgress ? ccColor4B{158, 145, 77, 255} : session->completions > 0 ? ccColor4B{98, 240, 70, 255} : ccColor4B{170, 170, 170, 255};
    bars.push_back(bar);
  }

  std::vector<ChartTick> ticks;
  if (!recent.empty())
  {
    float slot = 1.f / recent.size();
    ticks.push_back({slot / 2, shortDate(recent.front()->start)});
    if (recent.size() > 1)
      ticks.push_back({1.f - slot / 2, shortDate(recent.back()->start)});
  }

  auto legend = recent.size() < sessions.size() ? fmt::format("last {} <small>sessions</small>", recent.size()) : std::string("<small>minutes</small>");
  return BarChart::create(size, "Time <small>per session</small>", legend, bars, ticks, std::nullopt, "No sessions yet");
}

CCMenuItemSpriteExtra *HistoryPopup::drawBottomButton(char const *text, geode::Function<void()> callback)
{
  // One row for all of a tab's buttons
  auto menu = m_content->getChildByID("bottom-menu"_spr);
  if (!menu)
  {
    menu = CCMenu::create();
    menu->setID("bottom-menu"_spr);
    menu->ignoreAnchorPointForPosition(false);
    menu->setAnchorPoint({0.f, .5f});
    menu->setContentSize({0.f, BOTTOM_BAR});
    menu->setLayout(RowLayout::create()
                        ->setGap(5.f)
                        ->setAutoScale(false)
                        ->setAutoGrowAxis(0.f)
                        ->setAxisAlignment(AxisAlignment::Start));
    menu->setPosition({PADDING, PADDING + BOTTOM_BAR / 2 - 4.f});
    m_content->addChild(menu);
  }

  auto button = CCMenuItemExt::createSpriteExtra(ButtonSprite::create(text), [callback = std::move(callback)](auto) mutable
                                                 { callback(); });
  button->setScale(.6f);
  button->m_baseScale = .6f;
  menu->addChild(button);

  menu->updateLayout();
  return button;
}

// ! --- Session --- !

void HistoryPopup::drawSession()
{
  auto &tracker = tracker::Tracker::get();

  if (!liveHere())
  {
    drawHeader("Current Session", "<small>Not playing this level right now</small>");
    return;
  }

  auto const &session = tracker.session();
  auto totals = tracker.groupTotals();
  auto level = Storage::get().find(tracker.key());
  bool platformer = level && level->platformer;

  auto start = fmt_time::local(session.start);
  auto subtitle = fmt::format("<small>Started at</small> {:02}:{:02}", start.tm_hour, start.tm_min);
  if (level && grouped())
    subtitle += fmt::format(" <small>on</small> {} <small>({})</small>", level->name, tagFor(level));
  drawHeader("Current Session", subtitle);

  float width = cardWidth(5);
  bool completed = session.completions > 0;

  drawCards({card(width, fmt_time::rich(session.activeSeconds), "Played", StatCard::Style::Current, RichLabel::Variant::Orange),
             card(width, std::to_string(session.attempts), "Attempts"),
             platformer ? card(width, std::to_string(session.completions), "Completions")
                        : card(width, completed ? "100<small>%</small>" : fmt::format("{}<small>%</small>", session.best), "Best Run",
                               completed ? StatCard::Style::Completed : StatCard::Style::Default, completed ? RichLabel::Variant::Green : RichLabel::Variant::Default),
             card(width, std::to_string(session.practiceAttempts), "Practice"),
             card(width, fmt_time::rich(session.pausedSeconds), "Paused")},
            CONTENT_TOP);

  float chartTop = CONTENT_TOP - CARD_HEIGHT - GAP;
  float chartHeight = 96.f;
  CCSize chartSize = {m_size.width - PADDING * 2, chartHeight};

  Totals sessionTotals;
  sessionTotals.add(session);

  CCNode *chart;
  if (platformer)
  {
    std::vector<Session const *> sessions;
    for (auto shown : Storage::get().members(root()))
      for (auto const &other : shown->sessions)
        sessions.push_back(&other);
    chart = createTimeChart(sessions, chartSize);
  }
  else
    chart = createDeathsChart(sessionTotals, chartSize, "Deaths <small>this session</small>");

  chart->setPosition({m_size.width / 2, chartTop - chartHeight / 2});
  m_content->addChild(chart);

  std::vector<MetaRow> rows;

  auto total = fmt::format("{} / {}", fmt_time::rich(totals.activeSeconds), fmt_time::count(totals.shownAttempts(), "attempt", "attempts"));
  if (!platformer)
    total += fmt::format(" / <small>best</small> {}<small>%</small>", totals.best);
  rows.push_back({grouped() ? "Group Total:" : "Level Total:", total});
  rows.push_back({"This Launch:", fmt::format("{} <small>in</small> {}", fmt_time::rich(tracker.launchSeconds()),
                                              fmt_time::count(static_cast<int>(tracker.launchLevels()), "level", "levels"))});

  float tableWidth = m_size.width - PADDING * 2;
  auto table = MetaTable::create(rows, tableWidth, 8.f, .3f);
  auto tableBg = RectNode::create(table->getContentSize(), ccColor4B{30, 30, 30, 255}, 8.f);

  float tableTop = chartTop - chartHeight - GAP;
  tableBg->setPosition({PADDING, tableTop - table->getContentHeight()});
  table->setPosition(tableBg->getPosition());
  m_content->addChild(tableBg);
  m_content->addChild(table);
}

// ! --- Overview --- !

void HistoryPopup::drawOverview()
{
  auto &storage = Storage::get();
  auto levels = shownLevels();

  Totals totals;
  std::vector<Session const *> sessions;
  bool platformer = false;

  for (auto level : levels)
  {
    platformer = platformer || level->platformer;
    for (auto const &session : level->sessions)
    {
      totals.add(session);
      sessions.push_back(&session);
    }
  }

  auto groupRoot = root();
  auto own = storage.find(m_key);
  auto title = m_wholeGroup ? storage.groupName(groupRoot) : (own ? own->name : storage.groupName(groupRoot));

  std::string subtitle;
  if (m_wholeGroup && grouped())
  {
    auto count = storage.members(groupRoot).size();
    subtitle = fmt::format("{} <small>levels counted as one</small>", count);
  }
  else if (own && own->levelID > 0)
    subtitle = fmt::format("<small>ID</small> {}", own->levelID);
  else if (own)
    subtitle = "<small>Local level</small>";
  else
    subtitle = "<small>Not played yet</small>";

  if (totals.sessions > 0)
    subtitle += fmt::format(" <small>/ since</small> {}", shortDate(totals.first));

  drawHeader(title, subtitle);
  drawFilters(false);

  float width = cardWidth(5);
  bool completed = totals.completions > 0;

  drawCards({card(width, fmt_time::rich(totals.activeSeconds), "Play Time"),
             card(width, std::to_string(totals.sessions), "Sessions"),
             card(width, std::to_string(totals.shownAttempts()), "Attempts"),
             platformer ? card(width, fmt_time::rich(totals.pausedSeconds), "Paused")
                        : card(width, completed ? "100<small>%</small>" : fmt::format("{}<small>%</small>", totals.best), "Best Run",
                               completed ? StatCard::Style::Completed : StatCard::Style::Default, completed ? RichLabel::Variant::Green : RichLabel::Variant::Default),
             card(width, std::to_string(totals.completions), "Completions", completed ? StatCard::Style::Completed : StatCard::Style::Default,
                  completed ? RichLabel::Variant::Green : RichLabel::Variant::Default)},
            CONTENT_TOP);

  float chartTop = CONTENT_TOP - CARD_HEIGHT - GAP;
  float chartHeight = chartTop - PADDING;
  float fullWidth = m_size.width - PADDING * 2;

  if (platformer)
  {
    auto chart = createTimeChart(sessions, {fullWidth, chartHeight});
    chart->setPosition({m_size.width / 2, chartTop - chartHeight / 2});
    m_content->addChild(chart);
    return;
  }

  float deathsWidth = std::round((fullWidth - GAP) * .62f);
  float timeWidth = fullWidth - GAP - deathsWidth;

  auto deaths = createDeathsChart(totals, {deathsWidth, chartHeight}, "Deaths <small>by percent</small>");
  deaths->setPosition({PADDING + deathsWidth / 2, chartTop - chartHeight / 2});
  m_content->addChild(deaths);

  auto time = createTimeChart(sessions, {timeWidth, chartHeight});
  time->setPosition({m_size.width - PADDING - timeWidth / 2, chartTop - chartHeight / 2});
  m_content->addChild(time);
}

// ! --- Sessions --- !

void HistoryPopup::drawSessions()
{
  Totals totals;
  for (auto level : shownLevels())
    for (auto const &session : level->sessions)
      totals.add(session);

  drawHeader(fmt::format("Sessions: {}", totals.sessions),
             fmt::format("{}  {}", fmt_time::count(totals.shownAttempts(), "Attempt", "Attempts"), fmt_time::rich(totals.activeSeconds)));
  drawFilters(true);

  float listHeight = CONTENT_TOP - PADDING - BOTTOM_BAR;
  m_list = ListPanel::create({m_size.width - PADDING * 2, listHeight}, 4.f);
  m_list->setPosition({m_size.width / 2, PADDING + BOTTOM_BAR + listHeight / 2});
  m_list->setEmptyText("No sessions yet");
  m_content->addChild(m_list);

  drawBottomButton("Export CSV", [this]
                      { onExport(); });

  fillSessions();
}

void HistoryPopup::fillSessions()
{
  if (!m_list)
    return;

  auto &tracker = tracker::Tracker::get();
  bool showLevel = m_wholeGroup && grouped();

  std::vector<std::pair<Level const *, Session const *>> sessions;
  for (auto level : shownLevels())
    for (auto const &session : level->sessions)
      sessions.push_back({level, &session});

  std::ranges::sort(sessions, [this](auto const &a, auto const &b)
                    { return m_newestFirst ? a.second->start > b.second->start : a.second->start < b.second->start; });

  m_list->clear();
  float width = m_list->innerWidth();

  for (size_t i = 0; i < sessions.size() && i < MAX_SESSION_ROWS; i++)
  {
    auto [level, session] = sessions[i];
    bool live = session->inProgress && tracker.active() && level->key == tracker.key();
    auto levelText = showLevel ? fmt::format("{} <small>({})</small>", level->name, tagFor(level)) : std::string();

    m_list->add(SessionCell::create(width, *session, level->platformer, levelText, live, [this]
                                    {
                                      if (m_list)
                                        m_list->finish(false); }));
  }

  m_list->finish(true);
}

// ! --- Levels --- !

void HistoryPopup::drawLevels()
{
  drawHeader("Levels", grouped() ? fmt::format("{} <small>levels count as one</small>", Storage::get().members(root()).size())
                                 : std::string("<small>Copies join their original by themselves</small>"));

  float listHeight = CONTENT_TOP - PADDING - BOTTOM_BAR;
  m_list = ListPanel::create({m_size.width - PADDING * 2, listHeight}, 2.5f);
  m_list->setPosition({m_size.width / 2, PADDING + BOTTOM_BAR + listHeight / 2});
  m_list->setEmptyText("Play this level to see it here");
  m_content->addChild(m_list);

  drawBottomButton("Merge with...", [this]
                      { onMerge(); });

  // Only where Death Tracker has kept anything
  if (death_tracker::levelsDir())
  {
    auto button = std::make_shared<CCMenuItemSpriteExtra *>(nullptr);
    *button = drawBottomButton(death_tracker::importing() ? "Importing..." : "Import from Death Tracker", [this, button]
                               { onImport(*button); });
  }

  fillLevels();
}

void HistoryPopup::onImport(CCMenuItemSpriteExtra *button)
{
  if (death_tracker::importing())
    return;

  if (button)
  {
    static_cast<ButtonSprite *>(button->getNormalImage())->setString("Importing...");
    button->setEnabled(false);
    button->getParent()->updateLayout();
  }

  death_tracker::importAll([self = Ref(this)](death_tracker::ImportResult result)
                           {
                             auto text = fmt::format("Imported <cg>{}</c> sessions on <cg>{}</c> levels and <cg>{}</c> links between levels.",
                                                     result.sessions, result.levels, result.links);
                             if (result.overlapping > 0)
                               text += fmt::format("\n<cy>{}</c> sessions were recorded here too and kept as they are.", result.overlapping);
                             text += "\n\nImporting again only brings in what's new.";

                             FLAlertLayer::create("Death Tracker", text, "OK")->show();
                             self->drawContent(); });
}

void HistoryPopup::fillLevels()
{
  auto &storage = Storage::get();
  auto groupRoot = root();
  auto members = storage.members(groupRoot);
  bool isGrouped = grouped();
  float width = m_list->innerWidth();

  m_list->clear();

  // The original that copies point to may not be played yet
  if (isGrouped && !storage.find(groupRoot))
  {
    auto id = Storage::onlineID(groupRoot);
    auto details = id ? fmt::format("<small>Main /</small> ID {} <small>/ not played yet</small>", *id) : std::string("<small>Main / not played yet</small>");
    m_list->add(LevelRow::create(width, "Original level", details, {150, 150, 150}));
  }

  for (auto level : members)
  {
    Totals totals;
    for (auto const &session : level->sessions)
      totals.add(session);

    auto details = fmt::format("<small>{} /</small> {} <small>/</small> {} <small>/</small> {}", tagFor(level),
                               level->levelID > 0 ? fmt::format("ID {}", level->levelID) : std::string("Local"),
                               fmt_time::rich(totals.activeSeconds), fmt_time::count(totals.shownAttempts(), "attempt", "attempts"));
    if (totals.last > 0)
      details += fmt::format(" <small>/ last</small> {}", shortDate(totals.last));

    std::vector<RowButton> buttons;
    if (isGrouped)
    {
      auto key = level->key;
      auto name = level->name;
      buttons.push_back({"unlink-profile-btn.png"_spr, [this, key, name]
                         { onSplit(key, name); }});
    }

    bool current = level->key == m_key;
    m_list->add(LevelRow::create(width, level->name, details, current ? ccColor3B{99, 224, 110} : ccColor3B{255, 255, 255}, std::move(buttons)));
  }

  m_list->finish(true);
}

void HistoryPopup::onSplit(std::string const &key, std::string const &name)
{
  createQuickPopup("Split Level", fmt::format("Take <cy>{}</c> out of this group? Its sessions stay, they just count apart again.", name),
                   "Cancel", "Split", [self = Ref(this), key](auto, bool split)
                   {
                     if (!split)
                       return;
                     Storage::get().split(key);
                     self->drawContent(); });
}

void HistoryPopup::onMerge()
{
  MergePopup::create(m_key, [self = Ref(this)]
                     { self->drawContent(); })
      ->show();
}

// ! --- CSV --- !

static std::string csvField(std::string const &text)
{
  if (text.find_first_of(",\"\n") == std::string::npos)
    return text;

  std::string quoted = "\"";
  for (char c : text)
  {
    if (c == '"')
      quoted += '"';
    quoted += c;
  }
  return quoted + "\"";
}

void HistoryPopup::onExport()
{
  std::string csv = "level,level_id,tag,start,end,active_seconds,paused_seconds,attempts,start_pos_attempts,practice_attempts,completions,best,source\n";

  for (auto level : shownLevels())
  {
    auto tag = tagFor(level);
    for (auto const &session : level->sessions)
    {
      csv += fmt::format("{},{},{},{},{},{:.0f},{:.0f},{},{},{},{},{},{}\n", csvField(level->name), level->levelID, tag,
                         fmt_time::date(session.start), fmt_time::date(session.end), session.activeSeconds, session.pausedSeconds,
                         session.attempts, session.startPosAttempts, session.practiceAttempts, session.completions, session.best,
                         session.source.empty() ? "session-tracker" : session.source);
    }
  }

  auto dir = Mod::get()->getSaveDir() / "exports";
  (void)file::createDirectoryAll(dir);

  // Names can be anything, the date keeps exports apart
  auto stamp = fmt_time::date(data::now());
  std::ranges::replace(stamp, ':', '-');
  std::ranges::replace(stamp, ' ', '_');
  auto path = dir / fmt::format("sessions_{}.csv", stamp);

  if (auto result = file::writeString(path, csv); !result)
  {
    Notification::create(fmt::format("Export failed: {}", result.unwrapErr()), NotificationIcon::Error)->show();
    return;
  }

  Notification::create("Exported, opening the folder", NotificationIcon::Success)->show();
  file::openFolder(dir);
}
