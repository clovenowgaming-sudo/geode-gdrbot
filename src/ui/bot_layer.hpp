#pragma once

#include <Geode/Geode.hpp>

#include <filesystem>
#include <string>
#include <vector>

// In-game bot menu. Opened from the pause menu. Uses only plain cocos2d nodes
// so it behaves identically on Android and desktop.
class BotLayer : public cocos2d::CCLayerColor {
protected:
    bool init(cocos2d::CCSize const& size);

    void buildButtons();
    void buildFileList();
    void refreshFileList();
    void refresh();

    void onRecord(CCObject*);
    void onPlay(CCObject*);
    void onStop(CCObject*);
    void onSave(CCObject*);
    void onReload(CCObject*);
    void onStep(CCObject*);
    void onClose(CCObject*);
    void onPage(CCObject*);
    void onFile(CCObject* sender);

    CCLabelBMFont* m_statusLabel = nullptr;
    CCLabelBMFont* m_fileLabel = nullptr;

    cocos2d::CCMenu* m_menu = nullptr;
    cocos2d::CCMenu* m_fileMenu = nullptr;

    cocos2d::CCSize m_size;

    std::vector<std::filesystem::path> m_files;
    int m_page = 0;
    int m_perPage = 6;

public:
    static BotLayer* create(cocos2d::CCSize const& size);
    static void open(cocos2d::CCNode* parent);

    void keyBackClicked() override;
};
