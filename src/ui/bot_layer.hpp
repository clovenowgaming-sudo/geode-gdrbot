#pragma once

#include <Geode/Geode.hpp>

#include <filesystem>
#include <string>
#include <vector>

// In-game bot menu. Opened from the pause menu. Uses only plain cocos2d nodes
// so it behaves identically on Android and desktop.
//
// Layout: title + status, a macro selector (< name >), LOAD / RELOAD, then
// REC / PLAY / STOP / SAVE.
class BotLayer : public cocos2d::CCLayerColor {
protected:
    bool init(cocos2d::CCSize const& size);

    void buildButtons();
    void refreshFileList();
    void selectPath(std::filesystem::path const& path);
    void updateSelector();
    void refresh();
    void onTick(float dt);

    bool hasSelection() const;
    std::filesystem::path selectedPath() const;
    bool loadSelected(bool silent);

    void onRecord(CCObject*);
    void onPlay(CCObject*);
    void onStop(CCObject*);
    void onSave(CCObject*);
    void onLoad(CCObject*);
    void onReload(CCObject*);
    void onPrev(CCObject*);
    void onNext(CCObject*);
    void onClose(CCObject*);

    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCLabelBMFont* m_nameLabel = nullptr;
    cocos2d::CCLabelBMFont* m_infoLabel = nullptr;
    cocos2d::CCLabelBMFont* m_footerLabel = nullptr;

    cocos2d::CCMenu* m_menu = nullptr;

    cocos2d::CCSize m_size;

    std::vector<std::filesystem::path> m_files;
    int m_index = 0;

public:
    static BotLayer* create(cocos2d::CCSize const& size);
    static void open(cocos2d::CCNode* parent);

    void keyBackClicked() override;
};
