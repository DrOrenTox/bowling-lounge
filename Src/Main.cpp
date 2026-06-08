#include <Geode/Bindings.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <random>
#include <string>

using namespace geode::prelude;

// ============================================================================
// GLOBAL STATE & CORE GAMEPLAY VARIABLES
// ============================================================================
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
// GLOBED API PLACEHOLDERS
// ============================================================================
namespace GlobedAPI {
    size_t getRoomPlayerCount() {
        return 0; // Simulated connection count
    }
}

// ============================================================================
// GEODE HOOK: MENULAYER (MAIN DASHBOARD INTEGRATION)
// ============================================================================
class $modify(CosmicMenuButtonManager, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        // 1. Target the exact bottom layout row where the Account & Garage buttons live
        auto* bottomMenu = this->getChildByID("bottom-menu");
        if (bottomMenu) {
            
            // 2. Create the physical visual icon for your mod 
            auto* bowlingSprite = CCSprite::createWithSpriteFrameName("square01_001.png");
            if (bowlingSprite) {
                // Give it a bright cosmic cyan-blue theme color to stand out in the row layout
                bowlingSprite->setColor({ 0, 180, 255 }); 
            }

            // 3. Turn the visual icon asset into an interactive menu selection item
            auto* bowlingButton = CCMenuItemSpriteExtra::create(
                bowlingSprite,
                this,
                menu_selector(CosmicMenuButtonManager::onCosmicBowlingLoungeTap)
            );

            if (bowlingButton) {
                // 4. Register a unique tracking tracking ID for layout compatibility
                bowlingButton->setID("cosmic-bowling-shortcut");

                // 5. Inject the item into the container and force the row to resize seamlessly
                bottomMenu->addChild(bowlingButton);
                bottomMenu->updateLayout();
            }
        }
        return true;
    }

    // This function automatically executes whenever a user clicks your menu item
    void onCosmicBowlingLoungeTap(CCObject* sender) {
        // Initialize room configurations directly from the title screen interface
        if (!g_isInPrivateRoom) {
            CosmicRoomManager::hostNewRoom();
        }

        size_t currentLoungePlayers = GlobedAPI::getRoomPlayerCount();
        
        if (currentLoungePlayers > 8) {
            auto* alert = FLAlertLayer::create(
                "Lounge Full", 
                "This cosmic bowling lane is limited to 8 players!", 
                "Okey"
            );
            alert->show();
            return;
        }

        // Display interface panel on successful launch execution
        std::string alertMessage = "Connecting to private lounge deck...\nROOM CODE: " + g_myRoomID;
        auto* layerAlert = FLAlertLayer::create(
            "Cosmic Bowling", 
            alertMessage.c_str(), 
            "Launch!"
        );
        layerAlert->show();
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

