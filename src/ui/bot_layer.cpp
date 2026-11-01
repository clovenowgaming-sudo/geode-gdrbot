#include "bot_layer.hpp"

#include "../bot/bot.hpp"

#include <algorithm>
#include <system_error>

using namespace geode::prelude;

namespace {

constexpr float kPanelWidth = 360.f;
constexpr float kPanelHeight = 230.f;

CCMenuItemSpriteExtra* makeButton(
    char const* text, int width, char const* texture, CCObject* target, SEL_MenuHandler handler
) {
    auto sprite = ButtonSprite::create(text, width, true, "bigFont.fnt", texture, 30.f, .7f);
    return CCMenuItemSpriteExtra::create(sprite, target, handler);
}

CCMenuItemSpriteExtra* makeArrow(bool right, CCObject* target, SEL_MenuHandler handler) {
    auto sprite = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    sprite->setScale(.9f);
    if (right) sprite->setFlipX(true);
    return CCMenuItemSpriteExtra::create(sprite, target, handler);
}

// Shrinks a label so it never gets wider than maxWidth.
void fitLabel(CCLabelBMFont* label, float maxScale, float maxWidth) {
    label->setScale(1.f);
    float width = label->getContentSize().width;
    float scale = maxScale;
    if (width > 0.f) scale = std::min(maxScale, maxWidth / width);
    label->setScale(scale);
}

void alertMessage(std::string const& text) {
    FLAlertLayer::create("GDRBot", text, "OK")->show();
}

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

    // main panel
    auto panel = cocos2d::extension::CCScale9Sprite::create("GJ_square01.png");
    panel->setContentSize({kPanelWidth, kPanelHeight});
    panel->setPosition(center);
    this->addChild(panel);

    // title
    auto title = CCLabelBMFont::create("GDRBot", "bigFont.fnt");
    title->setScale(.7f);
    title->setPosition({center.x, center.y + 96.f});
    this->addChild(title);

    // status strip
    auto statusBg = cocos2d::extension::CCScale9Sprite::create("square02_001.png", {0.f, 0.f, 80.f, 80.f});
    statusBg->setContentSize({300.f, 22.f});
    statusBg->setColor({0, 0, 0});
    statusBg->setOpacity(110);
    statusBg->setPosition({center.x, center.y + 68.f});
    this->addChild(statusBg);

    m_statusLabel = CCLabelBMFont::create("Idle", "chatFont.fnt");
    m_statusLabel->setScale(.6f);
    m_statusLabel->setPosition({center.x, center.y + 68.f});
    this->addChild(m_statusLabel);

    // macro selector box
    auto selectBg = cocos2d::extension::CCScale9Sprite::create("square02_001.png", {0.f, 0.f, 80.f, 80.f});
    selectBg->setContentSize({220.f, 34.f});
    selectBg->setColor({0, 0, 0});
    selectBg->setOpacity(130);
    selectBg->setPosition({center.x, center.y + 30.f});
    this->addChild(selectBg);

    m_nameLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_nameLabel->setPosition({center.x, center.y + 30.f});
    this->addChild(m_nameLabel);

    m_infoLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_infoLabel->setScale(.55f);
    m_infoLabel->setPosition({center.x, center.y + 4.f});
    this->addChild(m_infoLabel);

    m_footerLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_footerLabel->setScale(.5f);
    m_footerLabel->setColor({170, 170, 170});
    m_footerLabel->setPosition({center.x, center.y - 94.f});
    this->addChild(m_footerLabel);

    m_menu = CCMenu::create();
    m_menu->setPosition({0.f, 0.f});
    this->addChild(m_menu);

    buildButtons();
    refreshFileList();

    // start with the macro that is already loaded, if there is one
    auto& bot = Bot::get();
    if (!bot.macroPath.empty()) selectPath(bot.macroPath);

    updateSelector();
    refresh();

    this->schedule(schedule_selector(BotLayer::onTick), .25f);

    return true;
}

void BotLayer::buildButtons() {
    auto center = cocos2d::CCPoint{m_size.width / 2.f, m_size.height / 2.f};

    auto add = [&](CCMenuItemSpriteExtra* item, float dx, float dy) {
        item->setPosition({center.x + dx, center.y + dy});
        m_menu->addChild(item);
    };

    // close (top left corner, like the vanilla popups)
    auto closeSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    auto closeBtn = CCMenuItemSpriteExtra::create(closeSprite, this, menu_selector(BotLayer::onClose));
    add(closeBtn, -kPanelWidth / 2.f + 6.f, kPanelHeight / 2.f - 6.f);

    // macro selector arrows
    add(makeArrow(false, this, menu_selector(BotLayer::onPrev)), -138.f, 30.f);
    add(makeArrow(true, this, menu_selector(BotLayer::onNext)), 138.f, 30.f);

    // load row
    add(makeButton("LOAD", 100, "GJ_button_02.png", this, menu_selector(BotLayer::onLoad)), -58.f, -22.f);
    add(makeButton("RELOAD", 100, "GJ_button_04.png", this, menu_selector(BotLayer::onReload)), 58.f, -22.f);

    // record / play row
    add(makeButton("REC", 78, "GJ_button_06.png", this, menu_selector(BotLayer::onRecord)), -126.f, -60.f);
    add(makeButton("PLAY", 78, "GJ_button_01.png", this, menu_selector(BotLayer::onPlay)), -42.f, -60.f);
    add(makeButton("STOP", 78, "GJ_button_04.png", this, menu_selector(BotLayer::onStop)), 42.f, -60.f);
    add(makeButton("SAVE", 78, "GJ_button_02.png", this, menu_selector(BotLayer::onSave)), 126.f, -60.f);
}

// ---------------------------------------------------------------- file list

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

    if (m_index >= static_cast<int>(m_files.size())) m_index = 0;
    if (m_index < 0) m_index = 0;
}

void BotLayer::selectPath(std::filesystem::path const& path) {
    auto it = std::find(m_files.begin(), m_files.end(), path);
    if (it != m_files.end()) {
        m_index = static_cast<int>(std::distance(m_files.begin(), it));
    }
}

bool BotLayer::hasSelection() const {
    return m_index >= 0 && m_index < static_cast<int>(m_files.size());
}

std::filesystem::path BotLayer::selectedPath() const {
    if (!hasSelection()) return {};
    return m_files[m_index];
}

void BotLayer::updateSelector() {
    if (!hasSelection()) {
        m_nameLabel->setString("No macros");
        fitLabel(m_nameLabel, .6f, 200.f);
        m_infoLabel->setString("Record a run, then press SAVE");
        m_infoLabel->setColor({170, 170, 170});
        return;
    }

    auto& bot = Bot::get();
    auto path = selectedPath();

    m_nameLabel->setString(path.stem().string().c_str());
    fitLabel(m_nameLabel, .6f, 200.f);

    bool loaded = bot.pendingReplay.has_value() && bot.macroPath == path;

    std::string info = fmt::format("{}/{}   -   ", m_index + 1, m_files.size());
    if (loaded) {
        info += fmt::format("loaded, {} inputs", bot.pendingReplay->inputs.size());
        m_infoLabel->setColor({120, 255, 120});
    } else {
        info += "not loaded";
        m_infoLabel->setColor({190, 190, 190});
    }
    m_infoLabel->setString(info.c_str());
}

void BotLayer::refresh() {
    auto& bot = Bot::get();

    char const* state = "Idle";
    cocos2d::ccColor3B color{215, 215, 215};

    if (bot.state == BotState::Recording) {
        state = "Recording";
        color = {255, 90, 90};
    } else if (bot.state == BotState::Playing) {
        state = "Playing";
        color = {120, 255, 120};
    }

    m_statusLabel->setString(fmt::format("{}  |  {}", state, bot.statusText).c_str());
    m_statusLabel->setColor(color);
    fitLabel(m_statusLabel, .6f, 285.f);

    m_footerLabel->setString(
        fmt::format("Practice fix: {}   |   TPS {}", bot.practiceFix() ? "ON" : "OFF", static_cast<int>(bot.tps())).c_str());
}

void BotLayer::onTick(float) {
    updateSelector();
    refresh();
}

// ------------------------------------------------------------------ actions

bool BotLayer::loadSelected(bool silent) {
    if (!hasSelection()) {
        if (!silent) alertMessage("No macro selected.");
        return false;
    }

    std::string error;
    if (!Bot::get().load(selectedPath(), &error)) {
        if (!silent) alertMessage(error.empty() ? "Failed to load." : error);
        return false;
    }

    return true;
}

void BotLayer::onPrev(CCObject*) {
    if (m_files.empty()) return;
    int count = static_cast<int>(m_files.size());
    m_index = (m_index - 1 + count) % count;
    updateSelector();
}

void BotLayer::onNext(CCObject*) {
    if (m_files.empty()) return;
    int count = static_cast<int>(m_files.size());
    m_index = (m_index + 1) % count;
    updateSelector();
}

void BotLayer::onLoad(CCObject*) {
    if (Bot::get().state == BotState::Recording) {
        alertMessage("Stop recording first.");
        return;
    }

    loadSelected(false);
    updateSelector();
    refresh();
}

void BotLayer::onReload(CCObject*) {
    if (Bot::get().state == BotState::Recording) {
        alertMessage("Stop recording first.");
        return;
    }

    // rescan the folder, keep the same macro selected, and read its inputs again
    auto current = selectedPath();
    refreshFileList();
    if (!current.empty()) selectPath(current);

    if (hasSelection()) loadSelected(false);

    updateSelector();
    refresh();
}

void BotLayer::onRecord(CCObject*) {
    auto& bot = Bot::get();
    auto pl = PlayLayer::get();

    if (!pl) {
        alertMessage("Start a level first.");
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
        alertMessage(error.empty() ? "Failed to save." : error);
    } else {
        refreshFileList();
        selectPath(bot.macroPath);
        updateSelector();
    }

    refresh();
}

void BotLayer::onPlay(CCObject*) {
    auto& bot = Bot::get();
    auto pl = PlayLayer::get();

    if (!pl) {
        alertMessage("Start a level first.");
        return;
    }

    if (bot.state == BotState::Recording) {
        alertMessage("Stop recording first.");
        return;
    }

    // nothing loaded yet: use the macro that is selected in the list
    if (!bot.pendingReplay) {
        if (!hasSelection()) {
            alertMessage("Pick a macro with < and > first.");
            return;
        }
        if (!loadSelected(false)) return;
    }

    if (bot.startPlaying(pl) && bot.autoRestart()) {
        pl->resetLevelFromStart();
    }

    updateSelector();
    refresh();
}

void BotLayer::onClose(CCObject*) {
    this->removeFromParentAndCleanup(true);
}

void BotLayer::keyBackClicked() {
    this->removeFromParentAndCleanup(true);
}
