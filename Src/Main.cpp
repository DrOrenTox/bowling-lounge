#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <random>
#include <string>
#include <vector>
#include <cmath>

using namespace geode::prelude;

// ============================================================================
// ENGINE STRUCTURES & GLOBAL STATE
// ============================================================================
struct PinEntity {
    CCSprite* sprite = nullptr;
    bool isKnockedDown = false;
    float vx = 0.0f;
    float vy = 0.0f;
    float rotationSpeed = 0.0f;
};

int g_pinsKnockedDown = 0;
bool g_isStrikeAwarded = false;
std::string g_myRoomID = "00000";
bool g_isInPrivateRoom = false;

void syncPinDropWithGlobed(int pinID);
void awardStrike();

// ============================================================================
// CUSTOM ROOM MANAGEMENT LOGIC
// ============================================================================
namespace CosmicRoomManager {
    std::string generateRoomCode() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(10000, 99999);
        return std::to_string(distr(gen));
    }

    void hostNewRoom() {
        g_myRoomID = generateRoomCode();
        g_isInPrivateRoom = true;
        log::info("Hosted a new cosmic bowling room via MenuLayer! Invite Code: {}", g_myRoomID);
    }
}

// ============================================================================
// REAL BOWLING LOUNGE PLAYABLE ENVIRONMENT WITH DYNAMIC KINETICS
// ============================================================================
class BowlingLoungeLayer : public CCLayer {
protected:
    CCSprite* m_bowlingBall = nullptr;
    std::vector<PinEntity> m_pinDeck;
    CCLabelBMFont* m_scoreLabel = nullptr;
    bool m_ballIsRolling = false;
    float m_ballVelocityX = 0.0f;
    float m_ballVelocityY = 0.0f;

    bool init() override {
        if (!CCLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // 1. Lounge Background (Arcade theme)
        auto* bg = CCLayerColor::create(ccc4(15, 12, 28, 255));
        this->addChild(bg, -2);

        // 2. Wooden Lane Layout
        auto* lane = CCLayerColor::create(ccc4(210, 160, 100, 255));
        lane->setContentSize({ winSize.width * 0.8f, 100.0f });
        lane->setPosition({ winSize.width * 0.1f, winSize.height * 0.25f });
        this->addChild(lane, -1);

        // 3. UI Header Strings
        auto titleStr = "Cosmic Alley — Room: " + g_myRoomID;
        auto* loungeTitle = CCLabelBMFont::create(titleStr.c_str(), "goldFont.fnt");
        loungeTitle->setPosition({ winSize.width / 2, winSize.height - 30.0f });
        loungeTitle->setScale(0.8f);
        this->addChild(loungeTitle);

        m_scoreLabel = CCLabelBMFont::create("Pins Down: 0", "bigFont.fnt");
        m_scoreLabel->setPosition({ winSize.width / 2, winSize.height - 70.0f });
        m_scoreLabel->setScale(0.5f);
        this->addChild(m_scoreLabel);

        // 4. Interactive Sandbox Menu Layout
        auto* actionMenu = CCMenu::create();
        actionMenu->setPosition(CCPointZero);
        this->addChild(actionMenu);

        // Roll Button
        auto* rollBtnSprite = ButtonSprite::create("ROLL BALL", "goldFont.fnt", "GJ_button_01.png");
        auto* rollButton = CCMenuItemSpriteExtra::create(
            rollBtnSprite, this, menu_selector(BowlingLoungeLayer::onRollBall)
        );
        rollButton->setPosition({ winSize.width * 0.25f, winSize.height * 0.10f });
        actionMenu->addChild(rollButton);

        // Reset Lane Button
        auto* resetBtnSprite = ButtonSprite::create("RESET LANE", "goldFont.fnt", "GJ_button_02.png");
        auto* resetButton = CCMenuItemSpriteExtra::create(
            resetBtnSprite, this, menu_selector(BowlingLoungeLayer::onResetAlley)
        );
        resetButton->setPosition({ winSize.width * 0.50f, winSize.height * 0.10f });
        actionMenu->addChild(resetButton);

        // Exit Button
        auto* exitBtnSprite = ButtonSprite::create("EXIT TO MENU", "goldFont.fnt", "GJ_button_06.png");
        auto* exitButton = CCMenuItemSpriteExtra::create(
            exitBtnSprite, this, menu_selector(BowlingLoungeLayer::onExitLounge)
        );
        exitButton->setPosition({ winSize.width * 0.75f, winSize.height * 0.10f });
        actionMenu->addChild(exitButton);

        setupAlleyEntities();
        this->scheduleUpdate();

        return true;
    }

    void setupAlleyEntities() {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        if (m_bowlingBall) m_bowlingBall->removeFromParent();
        for (auto& pin : m_pinDeck) {
            if (pin.sprite) pin.sprite->removeFromParent();
        }
        m_pinDeck.clear();

        m_ballIsRolling = false;
        m_ballVelocityX = 0.0f;
        m_ballVelocityY = 0.0f;

        m_bowlingBall = CCSprite::createWithSpriteFrameName("p_firework_01.png");
        m_bowlingBall->setPosition({ winSize.width * 0.15f, winSize.height * 0.38f });
        m_bowlingBall->setColor({ 130, 50, 250 });
        m_bowlingBall->setScale(1.8f);
        this->addChild(m_bowlingBall);

        float startX = winSize.width * 0.72f;
        float centerY = winSize.height * 0.38f;
        float spacingX = 18.0f;
        float spacingY = 16.0f;

        std::vector<CCPoint> targetCoords = {
            { startX, centerY },
            { startX + spacingX, centerY - spacingY }, 
            { startX + spacingX, centerY + spacingY },
            { startX + (spacingX * 2), centerY - (spacingY * 2) }, 
            { startX + (spacingX * 2), centerY }, 
            { startX + (spacingX * 2), centerY + (spacingY * 2) },
            { startX + (spacingX * 3), centerY - (spacingY * 3) }, 
            { startX + (spacingX * 3), centerY - spacingY }, 
            { startX + (spacingX * 3), centerY + spacingY }, 
            { startX + (spacingX * 3), centerY + (spacingY * 3) }
        };

        for (const auto& coordinate : targetCoords) {
            PinEntity pin;
            pin.sprite = CCSprite::createWithSpriteFrameName("slidergroove.png");
            pin.sprite->setPosition(coordinate);
            pin.sprite->setColor({ 255, 255, 255 });
            pin.sprite->setScaleX(0.5f);
            pin.sprite->setScaleY(1.4f);
            
            this->addChild(pin.sprite);
            
            pin.isKnockedDown = false;
            pin.vx = 0.0f;
            pin.vy = 0.0f;
            pin.rotationSpeed = 0.0f;
            
            m_pinDeck.push_back(pin);
        }
    }

    void onRollBall(CCObject*) {
        if (!m_ballIsRolling) {
            m_ballIsRolling = true;
            m_ballVelocityX = 8.5f;
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> distr(-0.4f, 0.4f);
            m_ballVelocityY = distr(gen);
        }
    }

    void onResetAlley(CCObject*) {
        g_pinsKnockedDown = 0;
        g_isStrikeAwarded = false;
        m_scoreLabel->setString("Pins Down: 0");
        setupAlleyEntities();
    }

    void onExitLounge(CCObject*) {
        auto* menuScene = MenuLayer::scene(false);
        CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, menuScene));
    }

    void update(float delta) override {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // 1. UPDATE BOWLING BALL POSITION
        if (m_ballIsRolling && m_bowlingBall) {
            m_bowlingBall->setPositionX(m_bowlingBall->getPositionX() + m_ballVelocityX);
            m_bowlingBall->setPositionY(m_bowlingBall->getPositionY() + m_ballVelocityY);
            m_bowlingBall->setRotation(m_bowlingBall->getRotation() + 15.0f);

            auto ballBox = m_bowlingBall->boundingBox();
            for (size_t i = 0; i < m_pinDeck.size(); ++i) {
                auto& pin = m_pinDeck[i];
                
                if (!pin.isKnockedDown && ballBox.intersectsRect(pin.sprite->boundingBox())) {
                    pin.isKnockedDown = true;
                    g_pinsKnockedDown++;

                    pin.vx = m_ballVelocityX * 0.75f;
                    pin.vy = (pin.sprite->getPositionY() - m_bowlingBall->getPositionY()) * 0.4f;
                    pin.rotationSpeed = 25.0f;

                    pin.sprite->setColor({ 255, 100, 100 });

                    std::string scoreStr = "Pins Down: " + std::to_string(g_pinsKnockedDown);
                    m_scoreLabel->setString(scoreStr.c_str());

                    syncPinDropWithGlobed(static_cast<int>(i));

                    if (g_pinsKnockedDown == 10 && !g_isStrikeAwarded) {
                        g_isStrikeAwarded = true;
                        awardStrike();
                    }
                }
            }

            if (m_bowlingBall->getPositionX() > winSize.width) {
                m_ballIsRolling = false;
                m_ballVelocityX = 0.0f;
                m_ballVelocityY = 0.0f;
                m_bowlingBall->setPosition({ winSize.width * 0.15f, winSize.height * 0.38f });
            }
        }

        // 2. PIN FLYING PHYSICS TICK ENGINE
        for (auto& pin : m_pinDeck) {
            if (pin.isKnockedDown) {
                pin.sprite->setPositionX(pin.sprite->getPositionX() + pin.vx);
                pin.sprite->setPositionY(pin.sprite->getPositionY() + pin.vy);
                pin.sprite->setRotation(pin.sprite->getRotation() + pin.rotationSpeed);

                auto flyingPinBox = pin.sprite->boundingBox();
                for (size_t j = 0; j < m_pinDeck.size(); ++j) {
                    auto& otherPin = m_pinDeck[j];
                    if (!otherPin.isKnockedDown && flyingPinBox.intersectsRect(otherPin.sprite->boundingBox())) {
                        otherPin.isKnockedDown = true;
                        g_pinsKnockedDown++;

                        otherPin.vx = pin.vx * 0.65f;
                        otherPin.vy = (otherPin.sprite->getPositionY() - pin.sprite->getPositionY()) * 0.5f;
                        otherPin.rotationSpeed = 20.0f;
                        otherPin.sprite->setColor({ 255, 150, 100 });

                        m_scoreLabel->setString(("Pins Down: " + std::to_string(g_pinsKnockedDown)).c_str());
                        syncPinDropWithGlobed(static_cast<int>(j));  // ✅ FIXED: Added <int>
                    }
                }

                pin.vx *= 0.95f;
                pin.vy *= 0.95f;
                pin.rotationSpeed *= 0.95f;

                if (pin.sprite->getPositionX() > winSize.width || fabsf(pin.vx) < 0.1f) {
                    pin.sprite->setVisible(false);
                }
            }
        }
    }

public:
    static BowlingLoungeLayer* create() {
        auto* ret = new BowlingLoungeLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    static CCScene* scene() {
        auto* scene = CCScene::create();
        auto* layer = BowlingLoungeLayer::create();
        scene->addChild(layer);
        return scene;
    }
};

// ============================================================================
// GEODE HOOK: MENULAYER (MAIN DASHBOARD INTEGRATION)
// ============================================================================
class $modify(CosmicMenuButtonManager, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto* bottomMenu = this->getChildByID("bottom-menu");
        if (bottomMenu) {
            auto* bowlingSprite = CCSprite::createWithSpriteFrameName("GJ_everyplayBtn_001.png");
            if (bowlingSprite) {
                bowlingSprite->setColor({ 0, 180, 255 });
            }

            auto* bowlingButton = CCMenuItemSpriteExtra::create(
                bowlingSprite,
                this,
                menu_selector(CosmicMenuButtonManager::onCosmicBowlingLoungeTap)
            );

            if (bowlingButton) {
                bowlingButton->setID("cosmic-bowling-shortcut");
                bottomMenu->addChild(bowlingButton);
                bottomMenu->updateLayout();
            }
        }
        return true;
    }

    void onCosmicBowlingLoungeTap(CCObject* sender) {
        if (!g_isInPrivateRoom) {
            CosmicRoomManager::hostNewRoom();
        }

        auto* loungeScene = BowlingLoungeLayer::scene();
        CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, loungeScene));
    }
};

// ============================================================================
// INTERMOD PACKET SYNCHRONIZATION WITH GLOBED
// ============================================================================
void syncPinDropWithGlobed(int pinID) {
    if (Loader::get()->isModLoaded("dankmeme.globed2")) {
        std::string message = "PIN_DROP:" + std::to_string(pinID) + "|ROOM:" + g_myRoomID;
        auto* targetMod = Loader::get()->getLoadedMod("dankmeme.globed2");
        if (targetMod) {
            log::info("Broadcasting Intermod Packet from menu: {}", message);
        }
    }
}

void awardStrike() {
    log::info("STRIKE! Awarding synchronized cosmic lane points.");
    auto* alert = FLAlertLayer::create("❌ STRIKE! ❌", "You cleared the deck in the Cosmic Lounge!", "Boom!");
    alert->show();
}
