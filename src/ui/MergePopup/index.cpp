#include "index.hpp"

#include "../components/RichLabel.hpp"
#include "../components/LevelRow.hpp"
#include "../format.hpp"

using namespace data;

static constexpr float WIDTH = 360.f;
static constexpr float HEIGHT = 260.f;
static constexpr float PADDING = 10.f;

MergePopup *MergePopup::create(std::string key, geode::Function<void()> onMerged)
{
  auto ret = new MergePopup();

  if (ret->init(std::move(key), std::move(onMerged)))
  {
    ret->autorelease();
    return ret;
  }

  CC_SAFE_DELETE(ret);
  return nullptr;
}

bool MergePopup::init(std::string key, geode::Function<void()> onMerged)
{
  if (!Popup::init(WIDTH, HEIGHT, "GJ_square01_custom.png"_spr))
    return false;

  m_key = std::move(key);
  m_onMerged = std::move(onMerged);

  auto &storage = Storage::get();

  auto title = CCLabelBMFont::create("Merge With", "goldFont.fnt");
  title->setAnchorPoint({0.f, .5f});
  title->setScale(.75f);
  title->setPosition({PADDING + 5.f, m_size.height - 22.f});
  m_mainLayer->addChild(title);

  auto subtitle = RichLabel::create(fmt::format("<small>Count</small> {} <small>together with...</small>", storage.groupName(storage.rootOf(m_key))),
                                "bigFont.fnt", .4f);
  subtitle->setAnchorPoint({0.f, .5f});
  subtitle->limitWidth(m_size.width - PADDING * 2 - 10.f);
  subtitle->setPosition({PADDING + 6.f, m_size.height - 38.f});
  m_mainLayer->addChild(subtitle);

  float searchHeight = 24.f;
  m_search = TextInput::create(m_size.width - PADDING * 2, "Search by name or ID");
  m_search->setScale(.8f);
  m_search->setContentWidth((m_size.width - PADDING * 2) / .8f);
  m_search->setCallback([this](auto const &)
                        { rebuild(); });
  m_mainLayer->addChildAtPosition(m_search, Anchor::Top, {0.f, -50.f - searchHeight / 2 + 4.f});

  float listTop = m_size.height - 50.f - searchHeight - 2.f;
  float listHeight = listTop - PADDING;
  m_list = ListPanel::create({m_size.width - PADDING * 2, listHeight}, 2.5f);
  m_list->setPosition({m_size.width / 2, PADDING + listHeight / 2});
  m_mainLayer->addChild(m_list);

  rebuild();
  return true;
}

static std::string lower(std::string text)
{
  std::ranges::transform(text, text.begin(), [](unsigned char c)
                         { return static_cast<char>(std::tolower(c)); });
  return text;
}

void MergePopup::rebuild()
{
  auto &storage = Storage::get();
  auto ownRoot = storage.rootOf(m_key);
  auto query = lower(m_search->getString());

  std::vector<Level const *> levels;

  for (auto level : storage.all())
  {
    // Already together
    if (storage.rootOf(level->key) == ownRoot)
      continue;

    bool matches = query.empty() || lower(level->name).find(query) != std::string::npos ||
                   (level->levelID > 0 && std::to_string(level->levelID).find(query) != std::string::npos);
    if (matches)
      levels.push_back(level);
  }

  auto lastPlayed = [](Level const *level)
  { return level->sessions.empty() ? 0 : level->sessions.back().end; };
  std::ranges::sort(levels, [&](auto a, auto b)
                    { return lastPlayed(a) > lastPlayed(b); });

  m_list->clear();
  m_list->setEmptyText(query.empty() ? "No other levels played yet" : "Nothing found");
  float width = m_list->innerWidth();

  for (auto level : levels)
  {
    auto groupSize = storage.members(storage.rootOf(level->key)).size();

    auto details = level->levelID > 0 ? fmt::format("ID {}", level->levelID) : std::string("Local");
    if (!level->creator.empty())
      details += fmt::format(" <small>by</small> {}", level->creator);
    if (groupSize > 1)
      details += fmt::format(" <small>/ group of</small> {}", groupSize);
    if (!level->sessions.empty())
    {
      auto last = fmt_time::local(level->sessions.back().end);
      details += fmt::format(" <small>/ last</small> {:02}.{:02}", last.tm_mday, last.tm_mon + 1);
    }

    auto key = level->key;
    auto name = level->name;
    std::vector<RowButton> buttons;
    buttons.push_back({"link-profile-btn.png"_spr, [this, key, name]
                       { merge(key, name); }});

    m_list->add(LevelRow::create(width, level->name, details, {255, 255, 255}, std::move(buttons)));
  }

  m_list->finish(true);
}

void MergePopup::merge(std::string const &target, std::string const &name)
{
  auto &storage = Storage::get();
  auto ownName = storage.groupName(storage.rootOf(m_key));

  createQuickPopup("Merge Levels",
                   fmt::format("Count <cy>{}</c> together with <cg>{}</c>? Their stats will add up. You can split them again later.", ownName, name),
                   "Cancel", "Merge", [self = Ref(this), target](auto, bool merge)
                   {
                     if (!merge)
                       return;

                     Storage::get().merge(self->m_key, target);
                     Notification::create("Levels merged", NotificationIcon::Success)->show();

                     if (self->m_onMerged)
                       self->m_onMerged();
                     self->onClose(nullptr); });
}
