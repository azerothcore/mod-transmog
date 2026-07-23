#ifndef DEF_TRANSMOGRIFICATION_H
#define DEF_TRANSMOGRIFICATION_H

#include "Player.h"
#include "Config.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "Item.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "ItemTemplate.h"
#include "QuestDef.h"
#include "ItemTemplate.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define PRESETS // comment this line to disable preset feature totally
#define HIDDEN_ITEM_ID 1 // used for hidden transmog - do not use a valid equipment ID
#define MAX_OPTIONS 25 // do not alter
#define MAX_SEARCH_STRING_LENGTH 50

class Item;
class Player;
class WorldSession;
struct ItemTemplate;

enum TransmogSettings
{
    SETTING_HIDE_TRANSMOG             = 0,
    SETTING_RETROACTIVE_CHECK         = 1,
    SETTING_VENDOR_INTERFACE          = 2,
    SETTING_HIDE_SET_DISCLAIMER       = 3,

    // Subscriptions
    SETTING_TRANSMOG_MEMBERSHIP_LEVEL = 0
};

enum MixedWeaponSettings
{
    MIXED_WEAPONS_STRICT = 0,
    MIXED_WEAPONS_MODERN = 1,
    MIXED_WEAPONS_LOOSE  = 2
};

enum TransmogStrings : uint32
{
    // Transmog result strings
    LANG_TRANSMOG_OK                           = 1,
    LANG_TRANSMOG_INVALID_SLOT                 = 2,
    LANG_TRANSMOG_INVALID_SRC_ENTRY            = 3,
    LANG_TRANSMOG_MISSING_SRC_ITEM             = 4,
    LANG_TRANSMOG_MISSING_DEST_ITEM            = 5,
    LANG_TRANSMOG_INVALID_ITEMS                = 6,
    LANG_TRANSMOG_NOT_ENOUGH_MONEY             = 7,
    LANG_TRANSMOG_NOT_ENOUGH_TOKENS            = 8,
    LANG_TRANSMOG_UNTRANSMOG_OK                = 9,
    LANG_TRANSMOG_UNTRANSMOG_NO_TRANSMOGS      = 10,
    LANG_TRANSMOG_PRESET_ERR_INVALID_NAME      = 11,
    // Command strings
    LANG_TRANSMOG_CMD_SHOW                     = 12,
    LANG_TRANSMOG_CMD_HIDE                     = 13,
    LANG_TRANSMOG_CMD_ADD_UNSUITABLE           = 14,
    LANG_TRANSMOG_CMD_ADD_FORBIDDEN            = 15,
    LANG_TRANSMOG_CMD_BEGIN_SYNC               = 16,
    LANG_TRANSMOG_CMD_COMPLETE_SYNC            = 17,
    LANG_TRANSMOG_CMD_VENDOR_INTERFACE_ENABLE  = 18,
    LANG_TRANSMOG_CMD_VENDOR_INTERFACE_DISABLE = 19,
    // Gossip/UI strings
    LANG_TRANSMOG_HOWWORKS                     = 20,
    LANG_TRANSMOG_MANAGESETS                   = 21,
    LANG_TRANSMOG_REMOVETRANSMOG               = 22,
    LANG_TRANSMOG_REMOVETRANSMOG_ASK           = 23,
    LANG_TRANSMOG_UPDATEMENU                   = 24,
    LANG_TRANSMOG_HOWSETSWORK                  = 25,
    LANG_TRANSMOG_SAVESET                      = 26,
    LANG_TRANSMOG_BACK                         = 27,
    LANG_TRANSMOG_USESET                       = 28,
    LANG_TRANSMOG_CONFIRM_USESET               = 29,
    LANG_TRANSMOG_DELETESET                    = 30,
    LANG_TRANSMOG_CONFIRM_DELETESET            = 31,
    LANG_TRANSMOG_INSERTSETNAME                = 32,
    LANG_TRANSMOG_SEARCH                       = 33,
    LANG_TRANSMOG_SEARCHING_FOR                = 34,
    LANG_TRANSMOG_SEARCH_FOR_ITEM              = 35,
    LANG_TRANSMOG_CONFIRM_HIDE_ITEM            = 36,
    LANG_TRANSMOG_HIDESLOT                     = 37,
    LANG_TRANSMOG_REMOVETRANSMOG_SLOT          = 38,
    LANG_TRANSMOG_CONFIRM_USEITEM              = 39,
    LANG_TRANSMOG_PREVIOUS_PAGE                = 40,
    LANG_TRANSMOG_NEXT_PAGE                    = 41,
    LANG_TRANSMOG_ADDED_APPEARANCE             = 42,
    // Disclaimer strings
    LANG_TRANSMOG_SET_DISCLAIMER               = 43,
    LANG_TRANSMOG_CMD_DISCLAIMER_ON            = 44,
    LANG_TRANSMOG_CMD_DISCLAIMER_OFF           = 45,
    // Check command: header / info
    LANG_TRANSMOG_CHECK_HEADER                 = 46,
    LANG_TRANSMOG_CHECK_DEST                   = 47,
    LANG_TRANSMOG_CHECK_SRC                    = 48,
    LANG_TRANSMOG_CHECK_PLAYER                 = 49,
    LANG_TRANSMOG_CHECK_PLAYER_NOT_FOUND       = 50,
    // Check command: section labels
    LANG_TRANSMOG_CHECK_SECTION_PAIR           = 51,
    LANG_TRANSMOG_CHECK_SECTION_ITEM           = 52,
    LANG_TRANSMOG_CHECK_SECTION_COLL           = 53,
    // Check command: verdict
    LANG_TRANSMOG_CHECK_RESULT_OK              = 54,
    LANG_TRANSMOG_CHECK_RESULT_FAIL            = 55,
    // Check command: pair fail messages
    LANG_TRANSMOG_CHECK_PAIR_IDS_SAME          = 56,
    LANG_TRANSMOG_CHECK_PAIR_DISP_SAME         = 57,
    LANG_TRANSMOG_CHECK_PAIR_CLASS_FAIL        = 58,
    LANG_TRANSMOG_CHECK_PAIR_DEST_TYPE_FAIL    = 59,
    LANG_TRANSMOG_CHECK_PAIR_SRC_TYPE_FAIL     = 60,
    LANG_TRANSMOG_CHECK_PAIR_RANGED_FAIL       = 61,
    LANG_TRANSMOG_CHECK_PAIR_SUB_DENIED        = 62,
    LANG_TRANSMOG_CHECK_PAIR_INV_DENIED        = 63,
    // Check command: per-item fail messages
    LANG_TRANSMOG_CHECK_ITEM_WHITELISTED       = 64,
    LANG_TRANSMOG_CHECK_ITEM_CLASS_FAIL         = 65,
    LANG_TRANSMOG_CHECK_ITEM_BLACKLISTED       = 66,
    LANG_TRANSMOG_CHECK_ITEM_QUALITY_FAIL      = 67,
    LANG_TRANSMOG_CHECK_ITEM_POLE_FAIL         = 68,
    LANG_TRANSMOG_CHECK_ITEM_EVENT_FAIL        = 69,
    LANG_TRANSMOG_CHECK_ITEM_STAT_FAIL         = 70,
    LANG_TRANSMOG_CHECK_ITEM_PROF_FAIL         = 71,
    LANG_TRANSMOG_CHECK_ITEM_FACTION_FAIL      = 72,
    LANG_TRANSMOG_CHECK_ITEM_CLASS_REQ_FAIL    = 73,
    LANG_TRANSMOG_CHECK_ITEM_RACE_REQ_FAIL     = 74,
    LANG_TRANSMOG_CHECK_ITEM_SKILL_FAIL        = 75,
    LANG_TRANSMOG_CHECK_ITEM_LEVEL_FAIL        = 76,
    LANG_TRANSMOG_CHECK_ITEM_SPELL_FAIL        = 77,
    // Check command: collection fail messages
    LANG_TRANSMOG_CHECK_COLL_DISABLED          = 78,
    LANG_TRANSMOG_CHECK_COLL_NOT_FOUND         = 79,
    // Check command: section pass message (shared by all sections)
    LANG_TRANSMOG_CHECK_SECTION_OK             = 80,
    // Claim command strings
    LANG_TRANSMOG_CMD_CLAIM_DISABLED           = 81,
    LANG_TRANSMOG_CMD_CLAIM_NOT_IN_BAGS        = 82,
    LANG_TRANSMOG_CMD_CLAIM_USAGE              = 83,
    LANG_TRANSMOG_CMD_CLAIM_INVALID_SLOT       = 84,
    LANG_TRANSMOG_CMD_CLAIM_EMPTY_SLOT         = 85,
    LANG_TRANSMOG_CMD_CLAIM_ALL_RESULT         = 86,
    LANG_TRANSMOG_CMD_CLAIM_ALL_NONE           = 87,
    LANG_TRANSMOG_CMD_CLAIM_ALREADY            = 88,
};

enum ArmorClassSpellIDs
{
    SPELL_PLATE   = 750,
    SPELL_MAIL    = 8737,
    SPELL_LEATHER = 9077,
    SPELL_CLOTH   = 9078
};

const uint32 AllArmorSpellIds[4] =
{
    SPELL_PLATE,
    SPELL_MAIL,
    SPELL_LEATHER,
    SPELL_CLOTH
};

const uint32 AllArmorTiers[4] =
{
    ITEM_SUBCLASS_ARMOR_PLATE,
    ITEM_SUBCLASS_ARMOR_MAIL,
    ITEM_SUBCLASS_ARMOR_LEATHER,
    ITEM_SUBCLASS_ARMOR_CLOTH
};

enum PlusFeatures
{
    PLUS_FEATURE_GREY_ITEMS,
    PLUS_FEATURE_LEGENDARY_ITEMS,
    PLUS_FEATURE_PET,
    PLUS_FEATURE_SKIP_LEVEL_REQ
};

const uint32 TMOG_VENDOR_CREATURE_ID = 190010;

class Transmogrification
{
public:
    static Transmogrification* instance();

    typedef std::unordered_map<ObjectGuid, ObjectGuid> transmogData;
    typedef std::unordered_map<ObjectGuid, uint32> transmog2Data;
    typedef std::unordered_map<ObjectGuid, transmog2Data> transmogMap;
    typedef std::unordered_map<uint32, std::unordered_set<uint32>> collectionCacheMap;
    typedef std::unordered_map<uint32, std::string> searchStringMap;
    typedef std::unordered_map<uint32, std::vector<uint32>> transmogPlusData;
    typedef std::unordered_map<ObjectGuid, uint8> selectedSlotMap;
    
    transmogPlusData plusDataMap;
    transmogMap entryMap; // entryMap[pGUID][iGUID] = entry
    transmogData dataMap; // dataMap[iGUID] = pGUID
    collectionCacheMap collectionCache;
    selectedSlotMap selectionCache;

#ifdef PRESETS
    bool EnableSetInfo;
    uint32 SetNpcText;

    typedef std::map<uint8, uint32> slotMap;
    typedef std::map<uint8, slotMap> presetData;
    typedef std::unordered_map<ObjectGuid, presetData> presetDataMap;
    presetDataMap presetById; // presetById[pGUID][presetID][slot] = entry
    typedef std::map<uint8, std::string> presetIdMap;
    typedef std::unordered_map<ObjectGuid, presetIdMap> presetNameMap;
    presetNameMap presetByName; // presetByName[pGUID][presetID] = presetName
    searchStringMap searchStringByPlayer;

    void PresetTransmog(Player* player, Item* itemTransmogrified, uint32 fakeEntry, uint8 slot);

    bool EnableSets;
    uint8 MaxSets;
    float SetCostModifier;
    int32 SetCopperCost;

    bool GetEnableSets() const;
    uint8 GetMaxSets() const;
    float GetSetCostModifier() const;
    int32 GetSetCopperCost() const;

    void LoadPlayerSets(ObjectGuid pGUID);
    void UnloadPlayerSets(ObjectGuid pGUID);
    void LoadCollections();
#endif

    bool EnableTransmogInfo;
    uint32 TransmogNpcText;

    // Use IsAllowed() and IsNotAllowed()
    // these are thread unsafe, but assumed to be static data so it should be safe
    std::set<uint32> Allowed;
    std::set<uint32> NotAllowed;

    float ScaledCostModifier;
    int32 CopperCost;

    bool RequireToken;
    uint32 TokenEntry;
    uint32 TokenAmount;

    bool AllowPoor;
    bool AllowCommon;
    bool AllowUncommon;
    bool AllowRare;
    bool AllowEpic;
    bool AllowLegendary;
    bool AllowArtifact;
    bool AllowHeirloom;
    bool AllowTradeable;

    bool AllowMixedArmorTypes;
    bool AllowLowerTiers;
    bool AllowMixedOffhandArmorTypes;
    bool AllowMixedWeaponHandedness;
    bool AllowFishingPoles;

    uint8 AllowMixedWeaponTypes;

    bool IgnoreReqRace;
    bool IgnoreReqClass;
    bool IgnoreReqSkill;
    bool IgnoreReqSpell;
    bool IgnoreReqLevel;
    bool IgnoreReqEvent;
    bool IgnoreReqStats;

    bool UseCollectionSystem;
    bool UseVendorInterface;
    
    bool AllowHiddenTransmog;
    bool HiddenTransmogIsFree;
    
    bool TrackUnusableItems;
    bool RetroActiveAppearances;
    bool ResetRetroActiveAppearances;
    bool ShowSetDisclaimer;

    bool IsTransmogEnabled;
    bool IsPortableNPCEnabled;

    bool IsAllowed(uint32 entry) const;
    bool IsNotAllowed(uint32 entry) const;
    bool IsAllowedQuality(uint32 quality, ObjectGuid const & playerGuid) const;
    bool IsRangedWeapon(uint32 Class, uint32 SubClass) const;
    bool CanNeverTransmog(ItemTemplate const* itemTemplate);

    void LoadConfig(bool reload); // thread unsafe

    std::string GetItemIcon(uint32 entry, uint32 width, uint32 height, int x, int y) const;
    std::string GetSlotIcon(uint8 slot, uint32 width, uint32 height, int x, int y) const;
    const char * GetSlotName(uint8 slot, WorldSession* session) const;
    std::string GetItemLink(Item* item, WorldSession* session) const;
    std::string GetItemLink(uint32 entry, WorldSession* session) const;
    uint32 GetFakeEntry(ObjectGuid itemGUID) const;
    void UpdateItem(Player* player, Item* item) const;
    void DeleteFakeEntry(Player* player, uint8 slot, Item* itemTransmogrified, CharacterDatabaseTransaction* trans = nullptr);
    void SetFakeEntry(Player* player, uint32 newEntry, uint8 slot, Item* itemTransmogrified);
    bool AddCollectedAppearance(uint32 accountId, uint32 itemId);
    // Adds the item's appearance to the player's account collection and, if newly unlocked,
    // notifies the player (LANG_TRANSMOG_ADDED_APPEARANCE). Shared by the auto-collect player
    // script and the `.transmog claim` command.
    void AddToDatabase(Player* player, ItemTemplate const* itemTemplate);

    TransmogStrings Transmogrify(Player* player, ObjectGuid itemGUID, uint8 slot, /*uint32 newEntry, */bool no_cost = false);
    TransmogStrings Transmogrify(Player* player, uint32 itemEntry, uint8 slot, /*uint32 newEntry, */bool no_cost = false);
    TransmogStrings Transmogrify(Player* player, Item* itemTransmogrifier, uint8 slot, /*uint32 newEntry, */bool no_cost = false, bool hidden_transmog = false);
    bool CanTransmogrifyItemWithItem(Player* player, ItemTemplate const* destination, ItemTemplate const* source) const;
    bool SuitableForTransmogrification(Player* player, ItemTemplate const* proto) const;
    bool SuitableForTransmogrification(ObjectGuid guid, ItemTemplate const* proto) const;
    bool IsItemTransmogrifiable(ItemTemplate const* proto, ObjectGuid const &playerGuid) const;
    uint32 GetSpecialPrice(ItemTemplate const* proto) const;

    void DeleteFakeFromDB(ObjectGuid::LowType itemLowGuid, CharacterDatabaseTransaction* trans = nullptr);
    float GetScaledCostModifier() const;
    int32 GetCopperCost() const;

    bool GetRequireToken() const;
    uint32 GetTokenEntry() const;
    uint32 GetTokenAmount() const;

    bool GetAllowMixedArmorTypes() const;
    bool GetAllowLowerTiers() const;
    bool GetAllowMixedOffhandArmorTypes() const;
    uint8 GetAllowMixedWeaponTypes() const;

    // Config
    bool GetEnableTransmogInfo() const;
    uint32 GetTransmogNpcText() const;
    bool GetEnableSetInfo() const;
    uint32 GetSetNpcText() const;
    bool GetAllowTradeable() const;

    bool GetUseCollectionSystem() const;
    bool GetUseVendorInterface() const;
    bool GetAllowHiddenTransmog() const;
    bool GetHiddenTransmogIsFree() const;
    bool GetTrackUnusableItems() const;
    bool EnableRetroActiveAppearances() const;
    bool EnableResetRetroActiveAppearances() const;
    [[nodiscard]] bool IsEnabled() const;

    bool IsValidOffhandArmor(uint32 subclass, uint32 invType) const;
    bool IsTieredArmorSubclass(uint32 subclass) const;

    uint32 GetHighestAvailableForPlayer(Player* player) const;
    uint32 GetHighestAvailableForPlayer(int playerGuid) const;

    bool TierAvailable(Player* player, int playerGuid, uint32 tierSpell) const;
    
    bool IsInvTypeMismatchAllowed (const ItemTemplate *source, const ItemTemplate *target) const;
    bool IsSubclassMismatchAllowed (Player *player, const ItemTemplate *source, const ItemTemplate *target) const;

    // Transmog Plus
    bool IsTransmogPlusEnabled;
    [[nodiscard]] bool IsPlusFeatureEligible(ObjectGuid const& playerGuid, uint32 feature) const;
    [[nodiscard]] uint32 GetPlayerMembershipLevel(Player* player) const { return player->GetPlayerSetting("acore_cms_subscriptions", SETTING_TRANSMOG_MEMBERSHIP_LEVEL).value; };
    [[nodiscard]] bool IgnoreLevelRequirement(ObjectGuid const& playerGuid) const { return IgnoreReqLevel || IsPlusFeatureEligible(playerGuid, PLUS_FEATURE_SKIP_LEVEL_REQ); }

    uint32 PetSpellId;
    uint32 PetEntry;
    [[nodiscard]] bool IsTransmogVendor(uint32 entry) const { return entry == TMOG_VENDOR_CREATURE_ID || entry == PetEntry; };
};
#define sTransmogrification Transmogrification::instance()

#endif
