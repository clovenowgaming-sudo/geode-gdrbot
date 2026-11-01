#include "bot_layer.hpp"

#include "../bot/bot.hpp"

#include <algorithm>
#include <system_error>

using namespace geode::prelude;

namespace {

CCMenuItemSpriteExtra* makeButton(char const* text, CCObject* target, SEL_MenuHandler handler) {
    auto sprite = ButtonSprite::create(text, "bigFont.fnt", "GJ_button_01.png", .8f);
    auto item = CCMenuItemSpriteExtra::create(sprite, target, handler);
    if (text) item->setID(text);
    return item;
}

constexpr float kPanelWidth = 300.f;
constexpr float kPanelHeight = 220.f;

} // namespace

BotLayer* BotLayer::create(cocos2d::CCSize const& size) {
    auto ret = new BotLayer();
    if (ret) {
        ret->m_size = size;
        if (ret->init(size)) {
            ret->autorelease();
            return ret;
        }
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void BotLayer::open(cocos2d::CCNode* parent) {
    if (!parent) return;

    auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

    auto layer = BotLayer::create(winSize);
    if (layer) parent->addChild(layer, 1000);
}

bool BotLayer::init(cocos2d::CCSize const& size) {
    if (!CCLayerColor::initWithColor({0, 0, 0, 150}, size.width, size.height)) {
        return false;
    }

    m_size = size;

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);

    auto center = cocos2d::CCPoint{size.width / 2.f, size.height / 2.f};

    auto panel = cocos2d::extension::CCScale9Sprite::create("GJ_square01.png");
    panel->setContentSize({kPanelWidth, kPanelHeight});
    panel->setPosition(center);
    this->addChild(panel);

    auto title = CCLabelBMFont::create("GDRBot", "bigFont.fnt");
    title->setScale(.7f);
    title->setPosition({center.x, center.y + kPanelHeight / 2.f - 18.f});
    this->addChild(title);

    m_statusLabel = CCLabelBMFont::create("Idle", "goldFont.fnt");
    m_statusLabel->setScale(.45f);
    m_statusLabel->setPosition({center.x, center.y + kPanelHeight / 2.f - 38.f});
    this->addChild(m_statusLabel);

    m_fileLabel = CCLabelBMFont::create("files", "chatFont.fnt");
    m_fileLabel->setScale(.5f);
    m_fileLabel->setAnchorPoint({0.f, .5f});
    m_fileLabel->setPosition({center.x - kPanelWidth / 2.f + 14.f, center.y + kPanelHeight / 2.f - 54.f});
    this->addChild(m_fileLabel);

    m_menu = CCMenu::create();
    m_menu->setPosition({0.f, 0.f});
    this->addChild(m_menu);

    m_fileMenu = CCMenu::create();
    m_fileMenu->setPosition({0.f, 0.f});
    this->addChild(m_fileMenu);

    buildButtons();
    refreshFileList();
    refresh();

    return true;
}

void BotLayer::buildButtons() {
    auto center = cocos2d::CCPoint{m_size.width / 2.f, m_size.height / 2.f};

    struct Entry {
        char const* text;
        SEL_MenuHandler handler;
        float dx;
        float dy;
    };

    std::vector<Entry> entries = {
        {"REC", menu_selector(BotLayer::onRecord), -105.f, 46.f},
        {"PLAY", menu_selector(BotLayer::onPlay), -35.f, 46.f},
        {"STOP", menu_selector(BotLayer::onStop), 35.f, 46.f},
        {"SAVE", menu_selector(BotLayer::onSave), 105.f, 46.f},
        {"RELOAD", menu_selector(BotLayer::onReload), -105.f, -kPanelHeight / 2.f + 22.f},
        {"STEP", menu_selector(BotLayer::onStep), -35.f, -kPanelHeight / 2.f + 22.f},
        {"<", menu_selector(BotLayer::onPage), 35.f, -kPanelHeight / 2.f + 22.f},
        {">", menu_selector(BotLayer::onPage), 105.f, -kPanelHeight / 2.f + 22.f},
        {"X", menu_selector(BotLayer::onClose), kPanelWidth / 2.f - 16.f, kPanelHeight / 2.f - 16.f},
    };

    for (auto& entry : entries) {
        auto button = makeButton(entry.text, this, entry.handler);
        button->setPosition({center.x + entry.dx, center.y + entry.dy});
        m_menu->addChild(button);
    }
}

void BotLayer::buildFileList() {
    m_fileMenu->removeAllChildren();

    auto center = cocos2d::CCPoint{m_size.width / 2.f, m_size.height / 2.f};
    float top = center.y + kPanelHeight / 2.f - 72.f;

    for (int i = 0; i < m_perPage; i++) {
        int index = m_page * m_perPage + i;
        if (index < 0 || index >= static_cast<int>(m_files.size())) break;

        auto label = m_files[index].stem().string();
        if (label.size() > 24) label = label.substr(0, 24) + "..";

        auto button = makeButton(label.c_str(), this, menu_selector(BotLayer::onFile));
        button->setPosition({center.x, top - i * 20.f});
        button->setScale(.75f);
        m_fileMenu->addChild(button);
    }

    int pages = std::max(1, (static_cast<int>(m_files.size()) + m_perPage - 1) / m_perPage);
    auto text = fmt::format("{} file(s) - page {}/{}", m_files.size(), m_page + 1, pages);
    m_fileLabel->setString(text.c_str());
}

void BotLayer::refreshFileList() {
    m_files.clear();

    auto dir = Bot::get().macrosDir();
    std::error_code ec;

    if (std::filesystem::exists(dir, ec)) {
        for (auto const& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file()) continue;
            auto name = entry.path().filename().string();
            if (name.find(".gdr2") != std::string::npos || name.find(".gdr") != std::string::npos) {
                m_files.push_back(entry.path());
            }
        }
    }

    std::sort(m_files.begin(), m_files.end());

    if (m_page * m_perPage >= static_cast<int>(m_files.size())) {
        m_page = 0;
    }

    buildFileList();
}

void BotLayer::refresh() {
    auto& bot = Bot::get();

    char const* state = "Idle";
    if (bot.state == BotState::Recording) state = "Recording";
    else if (bot.state == BotState::Playing) state = "Playing";

    m_statusLabel->setString(fmt::format("{} | {}", state, bot.statusText).c_str());
}

void BotLayer::onRecord(CCObject*) {
    auto& bot = Bot::get();
    auto pl = PlayLayer::get();

    if (!pl) {
        FLAlertLayer::create("GDRBot", "Start a level first.", "OK")->show();
        return;
    }

    if (bot.state == BotState::Playing) bot.stopPlaying(pl);

    if (bot.startRecording(pl) && bot.autoRestart()) {
        pl->resetLevelFromStart();
    }

    refresh();
}

void BotLayer::onStop(CCObject*) {
    auto& bot = Bot::get();
    auto pl = PlayLayer::get();

    if (bot.state == BotState::Recording) {
        bot.stopRecording(pl);
    } else if (bot.state == BotState::Playing) {
        bot.stopPlaying(pl);
    }

    refresh();
}

void BotLayer::onSave(CCObject*) {
    auto& bot = Bot::get();

    std::string error;
    auto name = bot.macroName;

    if (name.empty() || name == "macro") {
        name = botutil::levelName(PlayLayer::get()) + " macro";
    }

    if (!bot.save(name, &error)) {
        FLAlertLayer::create("GDRBot", error.empty() ? "Failed to save." : error, "OK")->show();
    } else {
        refreshFileList();
    }

    refresh();
}

void BotLayer::onReload(CCObject*) {
    refreshFileList();
    refresh();
}

void BotLayer::onStep(CCObject*) {
    auto& bot = Bot::get();
    bot.setFrozen(!bot.frozen);
    refresh();
}

void BotLayer::onClose(CCObject*) {
    this->removeFromParentAndCleanup(true);
}

void BotLayer::keyBackClicked() {
    this->removeFromParentAndCleanup(true);
}

void BotLayer::onPage(CCObject* sender) {
    auto* node = typeinfo_cast<CCNode*>(sender);
    std::string id = node ? node->getID() : std::string();

    int pages = std::max(1, (static_cast<int>(m_files.size()) + m_perPage - 1) / m_perPage);

    if (id == ">") {
        m_page = (m_page + 1) % pages;
    } else {
        m_page = (m_page - 1 + pages) % pages;
    }

    buildFileList();
}

void BotLayer::onFile(CCObject* sender) {
    auto& bot = Bot::get();
    auto* node = typeinfo_cast<CCNode*>(sender);
    if (!node) return;

    std::string id = node->getID();

    auto it = std::find_if(m_files.begin(), m_files.end(), [&id](std::filesystem::path const& path) {
        return path.stem().string() == id;
    });

    if (it == m_files.end()) return;

    std::string error;
    if (!bot.load(*it, &error)) {
        FLAlertLayer::create("GDRBot", error.empty() ? "Failed to load." : error, "OK")->show();
        return;
    }

    refresh();
}

void BotLayer::onPlay(CCObject*) {
    auto& bot = Bot::get();
    auto pl = PlayLayer::get();

    if (!pl) {
        FLAlertLayer::create("GDRBot", "Start a level first.", "OK")->show();
        return;
    }

    if (!bot.pendingReplay) {
        FLAlertLayer::create("GDRBot", "Pick a .gdr2 file from the list first.", "OK")->show();
        return;
    }

    if (bot.startPlaying(pl) && bot.autoRestart()) {
        pl->resetLevelFromStart();
    }

    refresh();
}
