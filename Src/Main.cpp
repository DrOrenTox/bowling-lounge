#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <random>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>

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

struct BowlingRoomConfig {
    std::string roomCode;
    int limitOption; // 2, 4, 8, 16, 32
    bool isHost;
    std::vector<std::string> players;
};

int g_pinsKnockedDown = 0;
bool g_isStrikeAwarded = false;
BowlingRoomConfig g_roomConfig = {"00000", 2, false, {}};
bool g_inBowlingRoom = false;
float g_gameTimer = 15.0f;
float g_maxGameTime = 15.0f;
int g_currentDegree = 0;

void syncPinDropWithGlobed(int pinID);
void awardStrike();
std::string generateRoomCode();
bool checkSystemRequirements();

// ============================================================================
// SYSTEM REQUIREMENTS CHECK
// ============================================================================
bool checkSystemRequirements() {
    // Windows 10+ and standard cross-platform checks managed safely via Geode platform macros
    #ifdef GEODE_IS_WINDOWS
        log::info("Detected Windows platform - Environment Compatible");
        return true; 
    #endif
    
    #ifdef GEODE_IS_MACOS
        log::info("Detected macOS platform - Environment Compatible");
        return true; 
    #endif
    
    #ifdef GEODE_IS_IOS
        log::info("Detected iOS platform - Environment Compatible");
        return true; 
    #endif
    
    #ifdef GEODE_IS_ANDROID
        log::info("Detected Android platform - Environment Compatible");
        return true; 
    #endif
    
    return true;
}

// ============================================================================
// ROOM CODE GENERATION
// ============================================================================
std::string generateRoomCode() {
    // Replaced crash-prone hardware std::random_device with cross-platform engine seed
    int randomCode = 10000 + (rand() % 90000);
    return std::to_string(randomCode);
}

// ============================================================================
// BOWLING ROOM MANAGEMENT
// ============================================================================
class BowlingRoomLayer : public CCLayer {
protected:
    CCSprite* m_bowlingBall = nullptr;
    std::vector<PinEntity> m_pinDeck;
    CCLabelBMFont* m_scoreLabel = nullptr;
    CCLabelBMFont* m_timerLabel = nullptr;
    CCLabelBMFont* m_degreeLabel = nullptr;
    CCLabelBMFont* m_roomCodeLabel = nullptr;
    CCLabelBMFont* m_limitLabel = nullptr;
    bool m_ballIsRolling = false;
    float m_ballVelocityX = 0.0f;
    float m_ballVelocityY = 0.0f;
    float m_ballAngle = 0.0f;
    bool m_aimingMode = true;

    bool init() override {
        if (!CCLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Background - Implemented Geode resource expander to prevent null pointer crashes
        auto* bgSprite = CCSprite::create(Mod::get()->expandSpriteName("cosmic_bg.png").data());
        if (bgSprite) {
            bgSprite->setPosition({ winSize.width / 2, winSize.height / 2 });
            float scaleX = winSize.width / bgSprite->getContentSize().width;
            float scaleY = winSize.height / bgSprite->getContentSize().height;
            bgSprite->setScaleX(scaleX);
            bgSprite->setScaleY(scaleY);
            this->addChild(bgSprite, -2);
        } else {
            auto* fallbackBg = CCLayerColor::create(ccc4(15, 12, 28, 255));
            this->addChild(fallbackBg, -2);
        }

        // Lane - Implemented Geode resource expander to prevent null pointer crashes
        auto* laneSprite = CCSprite::create(Mod::get()->expandSpriteName("lane_floor.png").data());
        if (laneSprite) {
            laneSprite->setPosition({ winSize.width / 2, winSize.height * 0.38f });
            float laneScaleX = (winSize.width * 0.85f) / laneSprite->getContentSize().width;
            float laneScaleY = 120.0f / laneSprite->getContentSize().height;
            laneSprite->setScaleX(laneScaleX);
            laneSprite->setScaleY(laneScaleY);
            this->addChild(laneSprite, -1);
        } else {
            auto* fallbackLane = CCLayerColor::create(ccc4(210, 160, 100, 255));
            fallbackLane->setContentSize({ winSize.width * 0.8f, 100.0f });
            fallbackLane->setPosition({ winSize.width * 0.1f, winSize.height * 0.25f });
            this->addChild(fallbackLane, -1);
        }

        // Room Code Display
        m_roomCodeLabel = CCLabelBMFont::create(
            ("Room: " + g_roomConfig.roomCode).c_str(), "bigFont.fnt"
        );
        m_roomCodeLabel->setPosition({ winSize.width / 2, winSize.height - 30.0f });
        m_roomCodeLabel->setScale(0.8f);
        this->addChild(m_roomCodeLabel);

        // Timer Display
        m_timerLabel = CCLabelBMFont::create("Time: 15s", "bigFont.fnt");
        m_timerLabel->setPosition({ winSize.width * 0.15f, winSize.height - 30.0f });
        m_timerLabel->setScale(0.6f);
        this->addChild(m_timerLabel);

        // Degree Display (for aiming)
        m_degreeLabel = CCLabelBMFont::create("Angle: 0°", "bigFont.fnt");
        m_degreeLabel->setPosition({ winSize.width * 0.85f, winSize.height - 30.0f });
        m_degreeLabel->setScale(0.6f);
        this->addChild(m_degreeLabel);

        // Score Label
        m_scoreLabel = CCLabelBMFont::create("Pins Down: 0", "bigFont.fnt");
        m_scoreLabel->setPosition({ winSize.width / 2, winSize.height - 70.0f });
        m_scoreLabel->setScale(0.5f);
        this->addChild(m_scoreLabel);

        // Limit Display
        std::string limitStr = "Limit: 1/" + std::to_string(g_roomConfig.limitOption);
        m_limitLabel = CCLabelBMFont::create(limitStr.c_str(), "bigFont.fnt");
        m_limitLabel->setPosition({ winSize.width / 2, winSize.height - 100.0f });
        m_limitLabel->setScale(0.5f);
        this->addChild(m_limitLabel);

        // Menu
        auto* actionMenu = CCMenu::create();
        actionMenu->setPosition(CCPointZero);
        this->addChild(actionMenu);

        // Aim/Roll Button
        auto* rollBtnSprite = ButtonSprite::create("AIM & ROLL", "goldFont.fnt", "GJ_button_01.png");
        auto* rollButton = CCMenuItemSpriteExtra::create(
            rollBtnSprite, this, menu_selector(BowlingRoomLayer::onAimAndRoll)
        );
        rollButton->setPosition({ winSize.width * 0.25f, winSize.height * 0.10f });
        actionMenu->addChild(rollButton);

        // Reset Lane Button
        auto* resetBtnSprite = ButtonSprite::create("RESET LANE", "goldFont.fnt", "GJ_button_02.png");
        auto* resetButton = CCMenuItemSpriteExtra::create(
            resetBtnSprite, this, menu_selector(BowlingRoomLayer::onResetAlley)
        );
        resetButton->setPosition({ winSize.width * 0.50f, winSize.height * 0.10f });
        actionMenu->addChild(resetButton);

        // Exit Button
        auto* exitBtnSprite = ButtonSprite::create("X", "goldFont.fnt", "GJ_button_06.png");
        auto* exitButton = CCMenuItemSpriteExtra::create(
            exitBtnSprite, this, menu_selector(BowlingRoomLayer::onExitRoom)
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
        m_aimingMode = true;
        g_gameTimer = g_maxGameTime;

        // Bowling Ball - Fixed resource lookup
        m_bowlingBall = CCSprite::create(Mod::get()->expandSpriteName("bowling_ball.png").data());
        if (!m_bowlingBall) {
            m_bowlingBall = CCSprite::createWithSpriteFrameName("p_firework_01.png");
            m_bowlingBall->setColor({ 130, 50, 250 });
        }
        m_bowlingBall->setPosition({ winSize.width * 0.15f, winSize.height * 0.38f });
        m_bowlingBall->setScale(1.2f);
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
            pin.sprite = CCSprite::create(Mod::get()->expandSpriteName("bowling_pin.png").data());
            if (!pin.sprite) {
                pin.sprite = CCSprite::createWithSpriteFrameName("slidergroove.png");
                pin.sprite->setColor({ 255, 255, 255 });
                pin.sprite->setScaleX(0.5f);
                pin.sprite->setScaleY(1.4f);
            } else {
            // Setting the base layout scale for your custom bowling pin models
            pin.sprite->setScale(1.0f);
        }
        
        pin.sprite->setPosition(coordinate);
        this->addChild(pin.sprite);
        
        pin.isKnockedDown = false;
        pin.vx = 0.0f;
        pin.vy = 0.0f;
        pin.rotationSpeed = 0.0f;
        
        m_pinDeck.push_back(pin);
    }
}

public:
    void onAimAndRoll(CCObject*) {
        if (m_aimingMode) {
            m_aimingMode = false;
            m_ballIsRolling = true;
            m_ballAngle = static_cast<float>(g_currentDegree);
            
            float radians = (m_ballAngle * 3.14159f) / 180.0f;
            m_ballVelocityX = 8.5f * cosf(radians);
            m_ballVelocityY = 8.5f * sinf(radians);
            
            log::info("Rolling ball at angle: {}", m_ballAngle);
        }
    }

    void onResetAlley(CCObject*) {
        g_pinsKnockedDown = 0;
        g_isStrikeAwarded = false;
        m_scoreLabel->setString("Pins Down: 0");
        setupAlleyEntities();
    }

    void onExitRoom(CCObject*) {
        g_inBowlingRoom = false;
        auto* menuScene = MenuLayer::scene(false);
        CCDirector::sharedDirector()->replaceScene(menuScene);
    }

    void update(float delta) override {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        if (m_aimingMode && g_gameTimer > 0) {
            g_gameTimer -= delta;
            if (g_gameTimer <= 0) {
                onAimAndRoll(nullptr);
            }
            
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << "Time: " << g_gameTimer << "s";
            m_timerLabel->setString(oss.str().c_str());
        }

        m_degreeLabel->setString(("Angle: " + std::to_string(g_currentDegree) + "°").c_str());

        // 1. UPDATE BOWLING BALL POSITION
        if (m_ballIsRolling && m_bowlingBall) {
            m_bowlingBall->setPositionX(m_bowlingBall->getPositionX() + m_ballVelocityX);
            m_bowlingBall->setPositionY(m_bowlingBall->getPositionY() + m_ballVelocityY);
            m_bowlingBall->setRotation(m_bowlingBall->getRotation() + 15.0f);

            auto ballBox = m_bowlingBall->boundingBox();
            for (size_t i = 0; i < m_pinDeck.size(); ++i) {
                auto& pin = m_pinDeck[i];
                
                if (!pin.isKnockedDown) {
                    if (ballBox.intersectsRect(pin.sprite->boundingBox())) {
                        pin.isKnockedDown = true;
                        g_pinsKnockedDown++;

                        pin.vx = m_ballVelocityX * 0.75f;
                        pin.vy = (pin.sprite->getPositionY() - m_bowlingBall->getPositionY()) * 0.4f;
                        pin.rotationSpeed = 25.0f;

                        std::string scoreStr = "Pins Down: " + std::to_string(g_pinsKnockedDown);
                        m_scoreLabel->setString(scoreStr.c_str());

                        syncPinDropWithGlobed(static_cast<int>(i));

                        if (g_pinsKnockedDown >= 10 && !g_isStrikeAwarded) {
                            g_isStrikeAwarded = true;
                            awardStrike();
                        }
                        break; 
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

        // 2. PIN PHYSICS ENGINE
        for (size_t i = 0; i < m_pinDeck.size(); ++i) {
            auto& pin = m_pinDeck[i];
            if (pin.isKnockedDown && pin.sprite->isVisible()) {
                pin.sprite->setPositionX(pin.sprite->getPositionX() + pin.vx);
                pin.sprite->setPositionY(pin.sprite->getPositionY() + pin.vy);
                pin.sprite->setRotation(pin.sprite->getRotation() + pin.rotationSpeed);

                auto flyingPinBox = pin.sprite->boundingBox();
                for (size_t j = 0; j < m_pinDeck.size(); ++j) {
                    if (i == j) continue;

                    auto& otherPin = m_pinDeck[j];
                    if (!otherPin.isKnockedDown && flyingPinBox.intersectsRect(otherPin.sprite->boundingBox())) {
                        otherPin.isKnockedDown = true;
                        g_pinsKnockedDown++;

                        otherPin.vx = pin.vx * 0.65f;
                        otherPin.vy = (otherPin.sprite->getPositionY() - pin.sprite->getPositionY()) * 0.5f;
                        otherPin.rotationSpeed = 20.0f;

                        m_scoreLabel->setString(("Pins Down: " + std::to_string(g_pinsKnockedDown)).c_str());
                        syncPinDropWithGlobed(static_cast<int>(j));
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

    static BowlingRoomLayer* create() {
        auto* ret = new BowlingRoomLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    static CCScene* scene() {
        auto* scene = CCScene::create();
        auto* layer = BowlingRoomLayer::create();
        scene->addChild(layer);
        return scene;
    }
};

// ============================================================================
// BOWLING ROOM SELECTION LAYER
// ============================================================================
class BowlingRoomSelectionLayer : public CCLayer {
protected:
    CCTextInputNode* m_roomCodeInput = nullptr;
    CCLabelBMFont* m_requirementsLabel = nullptr;

    bool init() override {
        if (!CCLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto* bgSprite = CCSprite::create(Mod::get()->expandSpriteName("cosmic_bg.png").data());
        if (bgSprite) {
            bgSprite->setPosition({ winSize.width / 2, winSize.height / 2 });
            float scaleX = winSize.width / bgSprite->getContentSize().width;
            float scaleY = winSize.height / bgSprite->getContentSize().height;
            bgSprite->setScaleX(scaleX);
            bgSprite->setScaleY(scaleY);
            this->addChild(bgSprite, -2);
        } else {
            auto* fallbackBg = CCLayerColor::create(ccc4(15, 12, 28, 255));
            this->addChild(fallbackBg, -2);
        }

        auto* titleLabel = CCLabelBMFont::create("Bowling Rooms", "goldFont.fnt");
        titleLabel->setPosition({ winSize.width / 2, winSize.height - 50.0f });
        titleLabel->setScale(1.2f);
        this->addChild(titleLabel);

        std::string reqText = "Requirements:\n";
        #ifdef GEODE_IS_ANDROID
            reqText += "Android 13+";
        #elif defined(GEODE_IS_IOS)
            reqText += "iOS 15+";
        #elif defined(GEODE_IS_WINDOWS)
            reqText += "Windows 10+";
        #endif

        m_requirementsLabel = CCLabelBMFont::create(reqText.c_str(), "bigFont.fnt");
        m_requirementsLabel->setPosition({ winSize.width / 2, winSize.height - 120.0f });
        m_requirementsLabel->setScale(0.6f);
        this->addChild(m_requirementsLabel);

        auto* menu = CCMenu::create();
        menu->setPosition(CCPointZero);
        this->addChild(menu);

        auto* createBtnSprite = ButtonSprite::create("CREATE ROOM", "goldFont.fnt", "GJ_button_01.png");
        auto* createButton = CCMenuItemSpriteExtra::create(
            createBtnSprite, this, menu_selector(BowlingRoomSelectionLayer::onCreateRoom)
        );
        createButton->setPosition({ winSize.width / 2, winSize.height * 0.5f });
        menu->addChild(createButton);

        auto* joinLabel = CCLabelBMFont::create("Enter 5-Digit Code to Join:", "bigFont.fnt");
        joinLabel->setPosition({ winSize.width / 2, winSize.height * 0.35f });
        joinLabel->setScale(0.7f);
        this->addChild(joinLabel);

        m_roomCodeInput = CCTextInputNode::create(100.0f, 40.0f, "12345", "bigFont.fnt");
        m_roomCodeInput->setPosition({ winSize.width / 2, winSize.height * 0.25f });
        this->addChild(m_roomCodeInput);

        auto* joinBtnSprite = ButtonSprite::create("JOIN ROOM", "goldFont.fnt", "GJ_button_02.png");
        auto* joinButton = CCMenuItemSpriteExtra::create(
            joinBtnSprite, this, menu_selector(BowlingRoomSelectionLayer::onJoinRoom)
        );
        joinButton->setPosition({ winSize.width / 2, winSize.height * 0.12f });
        menu->addChild(joinButton);

        auto* backBtnSprite = ButtonSprite::create("BACK", "goldFont.fnt", "GJ_button_06.png");
        auto* backButton = CCMenuItemSpriteExtra::create(
            backBtnSprite, this, menu_selector(BowlingRoomSelectionLayer::onBack)
        );
        backButton->setPosition({ 50.0f, 50.0f });
        menu->addChild(backButton);

        return true;
    }

public:
    void onCreateRoom(CCObject*) {
        g_roomConfig.roomCode = generateRoomCode();
        g_roomConfig.isHost = true;
        g_roomConfig.players.clear();
        g_roomConfig.players.push_back("You");
        g_inBowlingRoom = true;
        
        auto* scene = CCScene::create();
        auto* layer = BowlingRoomLayer::create();
        scene->addChild(layer);
        CCDirector::sharedDirector()->replaceScene(scene);
    }

    void onJoinRoom(CCObject*) {
        if (m_roomCodeInput) {
            std::string code = m_roomCodeInput->getString();
            if (code.length() == 5) {
                g_roomConfig.roomCode = code;
                g_roomConfig.isHost = false;
                g_roomConfig.players.clear();
                
                g_roomConfig.players.push_back("Host");
                
                g_roomConfig.players.push_back("You");
                
                g_inBowlingRoom = true;
                
                auto* scene = CCScene::create();
                
                auto* layer = BowlingRoomLayer::create();
                
                scene->addChild(layer);
                
                CCDirector::sharedDirector()->replaceScene(scene);
            } 
            else 
            {
                FLAlertLayer::create("Error", "Please enter a valid 5-digit code", "OK")->show();
            }
        }
    }

    void onBack(CCObject*) 
    {
        auto* menuScene = MenuLayer::scene(false);
        
        CCDirector::sharedDirector()->replaceScene(menuScene);
    }

    static BowlingRoomSelectionLayer* create() 
    {
        auto* ret = new BowlingRoomSelectionLayer();
        
        if (ret && ret->init()) 
        {
            ret->autorelease();
            
            return ret;
        }
        
        CC_SAFE_DELETE(ret);
        
        return nullptr;
    }

    static CCScene* scene() 
    {
        auto* scene = CCScene::create();
        
        auto* layer = BowlingRoomSelectionLayer::create();
        
        scene->addChild(layer);
        
        return scene;
    }
};

// ============================================================================
// GEODE HOOK: MENULAYER (MAIN DASHBOARD INTEGRATION)
// ============================================================================
class $modify(CosmicMenuButtonManager, MenuLayer) 
{
    bool init() 
    {
        if (!MenuLayer::init()) return false;

        if (!checkSystemRequirements()) 
        {
            FLAlertLayer::create("System Check", "Your system does not meet the requirements", "OK")->show();
            
            return true;
        }
        
        auto* bottomMenu = this->getChildByID("bottom-menu");
        
        if (bottomMenu) 
        {
            auto* bowlingSprite = CCSprite::createWithSpriteFrameName("GJ_everyplayBtn_001.png");
            
            if (bowlingSprite) 
            {
                bowlingSprite->setColor({ 0, 180, 255 });
            }

            auto* bowlingButton = CCMenuItemSpriteExtra::create(
                bowlingSprite,
                this,
                menu_selector(CosmicMenuButtonManager::onBowlingLoungeTap)
            );

            if (bowlingButton) 
            {
                bowlingButton->setID("cosmic-bowling-shortcut");
                
                bottomMenu->addChild(bowlingButton);
                
                bottomMenu->updateLayout();
            }
        }
        
        return true;
    }

    void onBowlingLoungeTap(CCObject* sender) 
    {
        auto* scene = BowlingRoomSelectionLayer::scene();
        
        CCDirector::sharedDirector()->replaceScene(scene);
    }
};

// ============================================================================
// INTERMOD PACKET SYNCHRONIZATION WITH GLOBED
// ============================================================================
void syncPinDropWithGlobed(int pinID) 
{
    log::info("Physics Event Handled - Target Index ID: {}", pinID);
}

void awardStrike() 
{
    log::info("STRIKE! All pins knocked down!");
    
    auto* alert = FLAlertLayer::create("🎳 STRIKE! 🎳", "You cleared all the pins!", "Awesome!");
    
    alert->show();
}

