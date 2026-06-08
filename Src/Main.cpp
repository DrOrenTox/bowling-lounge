#include <Geode/Bindings.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <random>
#include <string>

using namespace geode::prelude;

namespace CosmicRoomManager {
    std::string generateAnyFiveDigitCode() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(10000, 99999);
        return std::to_string(distr(gen));
    }
}

class $modify(CosmicMenuButtonManager, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto* bottomMenu = this->getChildByID("bottom-menu");
        if (bottomMenu) {
            
            auto* bowlingSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
            if (bowlingSprite) {
                bowlingSprite->setScale(0.45f);
                bowlingSprite->setColor({ 0, 220, 255 });
            }

            auto* bowlingButton = CCMenuItemSpriteExtra::create(
                bowlingSprite,
                this,
                menu_selector(CosmicMenuButtonManager::onLaunchPublicLoungeTap)
            );

            if (bowlingButton) {
                bowlingButton->setID("public-bowling-shortcut");
                bottomMenu->addChild(bowlingButton);
                bottomMenu->updateLayout();
            }
        }
        return true;
    }

    void onLaunchPublicLoungeTap(CCObject* sender) {
        std::string dynamicFiveDigitCode = CosmicRoomManager::generateAnyFiveDigitCode();
        std::string interfaceText = "Welcome to the Public Bowling Lounge!\n\nActive Server Room: " + dynamicFiveDigitCode;
        
        auto* layerAlert = FLAlertLayer::create(
            "Bowling Lounge Online", 
            interfaceText.c_str(), 
            "Enter Lane!"
        );
        layerAlert->show();
    }
};

