#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <random>

using namespace geode::prelude;

// Custom layer handling the game engine logic, textures, and interface
class CosmicAlleyLayer : public CCLayer {
protected:
    // Game Object Node pointers
    CCSprite* m_ball = nullptr;
    CCArray* m_pins = nullptr;
    
    // UI Label Element pointers
    CCLabelBMFont* m_scoreLabel = nullptr;
    CCLabelBMFont* m_pinsDownLabel = nullptr;
    CCLabelBMFont* m_streakLabel = nullptr;

    // Internal Game State tracking systems
    bool m_isRolling = false;
    int m_score = 0;
    int m_pinsDownCount = 0;
    int m_streakCount = 0;
    
    // Original spatial coordinates for game state resets
    CCPoint m_ballStartPos;

public:
    // Standard Cocos2d-x Layer Memory Allocator
    static CosmicAlleyLayer* create() {
        auto ret = new CosmicAlleyLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    // Secondary setup layer initialization routing
    bool init() override {
        if (!CCLayer::init()) return false;

        // Schedule frames to monitor real-time physics simulation loops
        this->scheduleUpdate();

        // Safe heap initialization for object pointer arrays
        m_pins = CCArray::create();
        m_pins->retain();

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        m_ballStartPos = ccp(120, winSize.height / 2);

        // ==========================================
        // 1. VISUAL LAYER INTERFACE GENERATION
        // ==========================================

        // Cosmic Alley Background Vector Setup (Z-Order 0)
        auto bg = CCSprite::createWithSpriteFrameName("yourname.bowling_mod/alley_bg.png");
        if (bg) {
            bg->setPosition({winSize.width / 2, winSize.height / 2});
            bg->setScaleX(winSize.width / bg->getContentSize().width);
            bg->setScaleY(winSize.height / bg->getContentSize().height);
            this->addChild(bg, 0);
        }

        // Wooden Lane Floor Vector Setup (Z-Order 1)
        auto lane = CCSprite::createWithSpriteFrameName("yourname.bowling_mod/alley_floor.png");
        if (lane) {
            lane->setPosition({winSize.width / 2, winSize.height / 2});
            lane->setRotation(90.0f); // Rotates horizontal to span the side-view lane
            
            float scaleX = winSize.width / lane->getContentSize().height;
            float scaleY = (winSize.height * 0.6f) / lane->getContentSize().width;
            lane->setScaleX(scaleX);
            lane->setScaleY(scaleY);
            this->addChild(lane, 1);
        }

        // Cyan Bowling Ball Vector Setup (Z-Order 2)
        m_ball = CCSprite::createWithSpriteFrameName("yourname.bowling_mod/alley_ball.png");
        if (m_ball) {
            m_ball->setScale(0.35f);
            m_ball->setPosition(m_ballStartPos);
            this->addChild(m_ball, 2);
        }

        // Generate the 10-pin triangle layout grid configuration
        setupBowlingPins();

        // ==========================================
        // 2. USER INTERFACE & LABELS LAYOUT
        // ==========================================

        auto uiMenu = CCMenu::create();
        uiMenu->setPosition({0, 0});
        this->addChild(uiMenu, 3);

        // Header Title Label (Matching your original screen design specifications)
        auto titleText = CCLabelBMFont::create("COSMIC ALLEY  ROOM: 84374", "goldFont.fnt");
        titleText->setPosition({winSize.width / 2, winSize.height - 35});
        titleText->setScale(0.75f);
        this->addChild(titleText, 3);

        // Real-time Pins Down Display text indicator
        m_pinsDownLabel = CCLabelBMFont::create("PINS DOWN: 0", "bigFont.fnt");
        m_pinsDownLabel->setPosition({winSize.width / 2, winSize.height - 75});
        m_pinsDownLabel->setScale(0.6f);
        this->addChild(m_pinsDownLabel, 3);

        // Cumulative Mod Score tracking status board
        m_scoreLabel = CCLabelBMFont::create("SCORE: 0000", "goldFont.fnt");
        m_scoreLabel->setPosition({80, winSize.height - 35});
        m_scoreLabel->setScale(0.55f);
        this->addChild(m_scoreLabel, 3);

        // Consecutive Win/Strike streak display text counter
        m_streakLabel = CCLabelBMFont::create("STREAK: 0", "bigFont.fnt");
        m_streakLabel->setPosition({winSize.width - 80, winSize.height - 35});
        m_streakLabel->setScale(0.45f);
        m_streakLabel->setColor({255, 0, 255}); // Magenta/Pink neon coloring
        this->addChild(m_streakLabel, 3);

        // ROLL BALL Action Frame Button (Bottom Left Corner)
        auto rollBtnSprite = ButtonSprite::create("ROLL BALL", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        auto rollBtn = CCMenuItemSpriteExtra::create(
            rollBtnSprite,
            this,
            menu_selector(CosmicAlleyLayer::onRollBall)
        );
        rollBtn->setPosition({winSize.width * 0.32f, 45});
        uiMenu->addChild(rollBtn);

        // EXIT TO MENU Navigation Frame Button (Bottom Right Corner)
        auto exitBtnSprite = ButtonSprite::create("EXIT TO MENU", "goldFont.fnt", "GJ_button_02.png", 0.8f);
        auto exitBtn = CCMenuItemSpriteExtra::create(
            exitBtnSprite,
            this,
            menu_selector(CosmicAlleyLayer::onExitToMenu)
        );
        exitBtn->setPosition({winSize.width * 0.68f, 45});
        uiMenu->addChild(exitBtn);

        return true;
    }

    // Array Generation Logic mapping specific side-view coordinate blocks
    void setupBowlingPins() {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        
        // Pin layout entry coordinates
        float tipX = winSize.width - 220; 
        float centerY = winSize.height / 2;

        // Custom structural spacing grids
        float rowSpacingX = 35.0f; 
        float pinSpacingY = 40.0f; 
        int targetTagId = 1;

        // Nested loops allocating 4 columns to create the 10-pin block (1+2+3+4)
        for (int col = 0; col < 4; ++col) {
            float columnStartY = centerY + (col * pinSpacingY / 2.0f);
            
            for (int row = 0; row <= col; ++row) {
                auto pin = CCSprite::createWithSpriteFrameName("yourname.bowling_mod/alley_pins.png");
                if (pin) {
                    pin->setScale(0.35f); 
                    
                    // Apply programmatic mapping algorithm
                    float currentX = tipX + (col * rowSpacingX);
                    float currentY = columnStartY - (row * pinSpacingY);
                    
                    pin->setPosition({currentX, currentY});
                    pin->setUserData(reinterpret_cast<void*>(uintptr_t(currentX))); // Save origin X coordinate safely
                    pin->setTag(targetTagId); // Keep standard tag identities clean
                    
                    this->addChild(pin, 2);
                    m_pins->addObject(pin);
                    targetTagId++;
                }
            }
        }
    }

    // Handles executing physics translation transformations on launch click
    void onRollBall(CCObject* sender) {
        if (m_isRolling) return; 
        m_isRolling = true;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Trigger original Geometry Dash sound effect managers
        FMODAudioEngine::sharedEngine()->playEffect("playTarget.ogg", 1.0f, 0.0f, 1.0f);

        // Movement action maps spanning the full floor length out-of-bounds
        auto moveAction = CCMoveTo::create(1.5f, ccp(winSize.width + 120, winSize.height / 2));
        auto rotateAction = CCRotateBy::create(1.5f, 1080.0f); // Fast linear spin rate
        
        auto resetCall = CCCallFunc::create(this, callfunc_selector(CosmicAlleyLayer::evaluateFrameResults));
        auto sequence = CCSequence::create(moveAction, resetCall, nullptr);

        m_ball->runAction(CCSpawn::create(sequence, rotateAction, nullptr));
    }

    // Handles the screen transition sequence safely popping back out to GD menus
    void onExitToMenu(CCObject* sender) {
        FMODAudioEngine::sharedEngine()->playEffect("quitSound.ogg", 1.0f, 0.0f, 1.0f);
        CCDirector::sharedDirector()->popSceneWithTransition(0.5f, kPopTransitionFade);
    }

    // Real-Time Frame Engine checking structural bounds intersection overlaps
    void update(float delta) override {
        if (!m_isRolling || !m_ball) return;

        // Extract and trim the collision bounding container area mapping
        CCRect ballBox = m_ball->boundingBox();
        ballBox.size.width *= 0.75f;
        ballBox.size.height *= 0.75f;

        // High precision pseudo-randomization vectors
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> angleDist(-45.0f, 45.0f);
        std::uniform_real_distribution<float> forceDist(60.0f, 130.0f);

        CCObject* itemObj = nullptr;
        CCARRAY_FOREACH(m_pins, itemObj) {
            auto pin = draw_cast<CCSprite*>(itemObj);
            
            // Filter configuration bypass flags (Tag ID 999 isolates down state)
            if (!pin || pin->getTag() == 999) continue;

            if (ballBox.intersectsRect(pin->boundingBox())) {
                pin->setTag(999); // Instantly flag state to prevent duplicate calls
                m_pinsDownCount++;

                // Update text container strings
                m_pinsDownLabel->setString(fmt::format("PINS DOWN: {}", m_pinsDownCount).c_str());

                // Trigger impactful crash impact sounds using original game asset libraries
                FMODAudioEngine::sharedEngine()->playEffect("hitSpikes.ogg", 0.8f, 0.0f, 1.0f);

                // Advanced trajectory math algorithms
float randomAngle = angleDist(gen);
float randomForce = forceDist(gen);

// Create a simulated gravity arc using physics vectors
auto launchBack = CCMoveBy::create(0.5f, ccp(randomForce, randomAngle));
auto rotationSpin = CCRotateBy::create(0.5f, randomForce * 3.0f);
auto fadeOutEffect = CCFadeOut::create(0.5f);

// Package actions concurrently inside spawn sequences
pin->runAction(CCSpawn::create(launchBack, rotationSpin, fadeOutEffect, nullptr));
}
}
}

// Compiles framework scores before generating scene layout reloads
void evaluateFrameResults() {
if (m_pinsDownCount >= 10) {
// Absolute strike accomplishment configuration routing
m_score += 500;
m_streakCount++;
FMODAudioEngine::sharedEngine()->playEffect("achievement_01.ogg", 1.0f, 0.0f, 1.0f);

auto strikeNotice = CCLabelBMFont::create("STRIKE !!", "goldFont.fnt");
strikeNotice->setPosition(CCDirector::sharedDirector()->getWinSize() / 2);
strikeNotice->setScale(1.5f);
this->addChild(strikeNotice, 4);

strikeNotice->runAction(CCSequence::create(
CCScaleTo::create(0.3f, 2.0f),
CCFadeOut::create(0.5f),
CCRemoveSelf::create(),
nullptr
));
} else if (m_pinsDownCount > 0) {
// Standard point conversion distributions
m_score += (m_pinsDownCount * 30);
m_streakCount = 0; // Reset consecutive combo chain
} else {
// Gutter execution exception handling
m_streakCount = 0;
FMODAudioEngine::sharedEngine()->playEffect("explode_11.ogg", 0.7f, 0.0f, 1.0f);
}

// Apply updated values across operational labels
m_scoreLabel->setString(fmt::format("SCORE: {:04d}", m_score).c_str());
m_streakLabel->setString(fmt::format("STREAK: {}", m_streakCount).c_str());

// Process final room variable system reset
this->runAction(CCSequence::create(
CCDelayTime::create(0.6f),
CCCallFunc::create(this, callfunc_selector(CosmicAlleyLayer::resetGame)),
nullptr
));
}

// Flushes animation instances and resets positions
void resetGame() {
m_isRolling = false;
m_pinsDownCount = 0;

// Direct assignment targeting original configuration positions
m_pinsDownLabel->setString("PINS DOWN: 0");

m_ball->stopAllActions();
m_ball->setPosition(m_ballStartPos);
m_ball->setRotation(0);
m_ball->setOpacity(255);

// Cycle full array tree converting attributes back to visible defaults
CCObject* itemObj = nullptr;
int sequentialIndex = 1;

CCARRAY_FOREACH(m_pins, itemObj) {
auto pin = draw_cast<CCSprite*>(itemObj);
if (pin) {
pin->stopAllActions();
pin->setTag(sequentialIndex);
pin->setOpacity(255);
pin->setRotation(0);

// Recover layout properties saved inside custom pointer structures
float originalX = static_cast(reinterpret_cast<uintptr_t>(pin->getUserData()));
auto winSize = CCDirector::sharedDirector()->getWinSize();

// Reposition using the internal column configurations
float centerY = winSize.height / 2;
float pinSpacingY = 40.0f;

// Reverse engineering column indices based on location
int calculatedCol = static_cast((originalX - (winSize.width - 220)) / 35.0f);
float columnStartY = centerY + (calculatedCol * pinSpacingY / 2.0f);

// Gather contextual offset identities
int pinIndexInCol = 0;
CCObject* internalScan = nullptr;
CCARRAY_FOREACH(m_pins, internalScan) {
auto pScan = draw_cast<CCSprite*>(internalScan);
if(pScan && pScan != pin && pScan->getUserData() == pin->getUserData() && pScan->getTag() < pin->getTag()) {
pinIndexInCol++;
}
}

float restoredY = columnStartY - (pinIndexInCol * pinSpacingY);
pin->setPosition({originalX, restoredY});
sequentialIndex++;
}
}
}

// Clear tracking references out of heap scopes to avoid crash leaks
~CosmicAlleyLayer() override {
CC_SAFE_RELEASE(m_pins);
}
};

// ==========================================
// 3. MAIN GEOMETRY DASH MEMU LAYER INJECTION
// ==========================================

class $modify(MyMenuLayer, MenuLayer) {
bool init() {
if (!MenuLayer::init()) return false;

// Safely extract main UI menu structure arrays from memory trees
auto menu = this->getChildByID("main-menu");
if (!menu) return true;

// Custom main menu entry button instance setup configuration
auto btnSprite = CCSprite::createWithSpriteFrameName("GJ_playBtn_001.png");
if (btnSprite) {
btnSprite->setScale(0.45f);
btnSprite->setColor({0, 240, 255}); // Stylize launcher to match cyan colors

auto btn = CCMenuItemSpriteExtra::create(
btnSprite,
this,
menu_selector(MyMenuLayer::onCosmicAlleyClick)
);

// Safely push layout injection items into Geometry Dash nodes
if (btn) {
btn->setID("cosmic-alley-launcher");
menu->addChild(btn);
menu->updateLayout();
}
}

return true;
}

// Direct scene navigation routing logic
void onCosmicAlleyClick(CCObject* sender) {
// Trigger default menu navigation sound profile ticks
FMODAudioEngine::sharedEngine()->playEffect("GJ_select_01.ogg", 1.0f, 0.0f, 1.0f);

auto scene = CCScene::create();
auto layer = CosmicAlleyLayer::create();

if (scene && layer) {
scene->addChild(layer);
// Push scene via a sleek 0.5-second crossfade transition animation
CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}
}
};


