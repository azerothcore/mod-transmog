/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Bag.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "Transmogrification.h"

using namespace Acore::ChatCommands;

class transmog_commandscript : public CommandScript
{
public:
    transmog_commandscript() : CommandScript("transmog_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable addCollectionTable =
        {
            { "set", HandleAddTransmogItemSet,    SEC_MODERATOR, Console::Yes },
            { "",    HandleAddTransmogItem,       SEC_MODERATOR, Console::Yes },
        };

        static ChatCommandTable transmogTable =
        {
            { "add",        addCollectionTable                                              },
            { "check",      HandleCheckTransmog,           SEC_GAMEMASTER,    Console::Yes },
            { "",           HandleDisableTransMogVisual,   SEC_PLAYER,        Console::No  },
            { "sync",       HandleSyncTransMogCommand,     SEC_PLAYER,        Console::No  },
            { "portable",   HandleTransmogPortableCommand, SEC_PLAYER,        Console::No  },
            { "claim",      HandleClaimCommand,            SEC_PLAYER,        Console::No  },
            { "interface",  HandleInterfaceOption,         SEC_PLAYER,        Console::No  },
            { "disclaimer", HandleDisclaimerOption,        SEC_PLAYER,        Console::No  },
            { "reload",     HandleReloadTransmogConfig,    SEC_ADMINISTRATOR, Console::Yes }
        };

        static ChatCommandTable commandTable =
        {
            { "transmog", transmogTable },
        };

        return commandTable;
    }

    static bool HandleSyncTransMogCommand(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        uint32 accountId = player->GetSession()->GetAccountId();
        handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_BEGIN_SYNC);

        for (uint32 itemId : sTransmogrification->collectionCache[accountId])
            handler->PSendSysMessage("TRANSMOG_SYNC:{}", itemId);

        handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_COMPLETE_SYNC);
        return true;
    }

    static bool HandleDisableTransMogVisual(ChatHandler* handler, bool hide)
    {
        Player* player = handler->GetPlayer();

        if (hide)
        {
            player->UpdatePlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG, 0);
            handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_SHOW);
        }
        else
        {
            player->UpdatePlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG, 1);
            handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_HIDE);
        }

        player->UpdateObjectVisibility();
        return true;
    }

    static bool HandleAddTransmogItem(ChatHandler* handler, Optional<PlayerIdentifier> player, ItemTemplate const* itemTemplate)
    {
        if (!sTransmogrification->GetUseCollectionSystem())
            return true;

        if (!sObjectMgr->GetItemTemplate(itemTemplate->ItemId))
        {
            handler->PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, itemTemplate->ItemId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!player)
            player = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!player)
            return false;

        Player* target = player->GetConnectedPlayer();
        bool isNotConsole = handler->GetSession();
        bool suitableForTransmog;

        if (target)
            suitableForTransmog = sTransmogrification->SuitableForTransmogrification(target, itemTemplate);
        else
            suitableForTransmog = sTransmogrification->SuitableForTransmogrification(player->GetGUID(), itemTemplate);

        if (!sTransmogrification->GetTrackUnusableItems() && !suitableForTransmog)
        {
            handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_ADD_UNSUITABLE);
            handler->SetSentErrorMessage(true);
            return true;
        }

        if (itemTemplate->Class != ITEM_CLASS_ARMOR && itemTemplate->Class != ITEM_CLASS_WEAPON)
        {
            handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_ADD_FORBIDDEN);
            handler->SetSentErrorMessage(true);
            return true;
        }

        auto guid = player->GetGUID();
        uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(guid);
        uint32 itemId = itemTemplate->ItemId;

        std::stringstream tempStream;
        tempStream << std::hex << ItemQualityColors[itemTemplate->Quality];
        std::string itemQuality = tempStream.str();
        std::string itemName = itemTemplate->Name1;

        if (target) {
            // get locale item name
            int locIndex = target->GetSession()->GetSessionDbLocaleIndex();
            if (ItemLocale const* il = sObjectMgr->GetItemLocale(itemId))
                ObjectMgr::GetLocaleString(il->Name, locIndex, itemName);
        }

        std::string playerName = player->GetName();
        std::string nameLink = handler->playerLink(playerName);

        if (sTransmogrification->AddCollectedAppearance(accountId, itemId))
        {
            // Notify target of new item in appearance collection
            if (target && !(target->GetPlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG).value) && !sTransmogrification->CanNeverTransmog(itemTemplate))
                ChatHandler(target->GetSession()).PSendSysMessage(R"(|c{}|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r has been added to your appearance collection.)", itemQuality.c_str(), itemId, itemName.c_str());

            // Feedback of successful command execution to GM
            if (isNotConsole && target != handler->GetPlayer())
                handler->PSendSysMessage(R"(|c{}|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r has been added to the appearance collection of Player {}.)", itemQuality.c_str(), itemId, itemName.c_str(), nameLink);

            CharacterDatabase.Execute("INSERT INTO custom_unlocked_appearances (account_id, item_template_id) VALUES ({}, {})", accountId, itemId);
        }
        else
        {
            // Feedback of failed command execution to GM
            if (isNotConsole)
            {
                handler->PSendSysMessage(R"(Player {} already has item |c{}|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r in the appearance collection.)", nameLink, itemQuality.c_str(), itemId, itemName.c_str());
                handler->SetSentErrorMessage(true);
            }
        }

        return true;
    }

    static bool HandleAddTransmogItemSet(ChatHandler* handler, Optional<PlayerIdentifier> player, Variant<Hyperlink<itemset>, uint32> itemSetId)
    {
        if (!sTransmogrification->GetUseCollectionSystem())
            return true;

        if (!*itemSetId)
        {
            handler->PSendSysMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, uint32(itemSetId));
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!player)
            player = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!player)
            return false;

        Player* target = player->GetConnectedPlayer();
        ItemSetEntry const* set = sItemSetStore.LookupEntry(uint32(itemSetId));
        bool isNotConsole = handler->GetSession();

        if (!set)
        {
            handler->PSendSysMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, uint32(itemSetId));
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto guid = player->GetGUID();
        CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(guid);
        if (!playerData)
            return false;

        bool added = false;
        uint32 error = 0; // holds a TransmogStrings id, 0 = no error
        uint32 itemId;
        uint32 accountId = playerData->AccountId;

        for (uint32 i = 0; i < MAX_ITEM_SET_ITEMS; ++i)
        {
            itemId = set->itemId[i];
            if (itemId)
            {
                ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
                if (itemTemplate)
                {
                    if (!sTransmogrification->GetTrackUnusableItems() && (
                            (target && !sTransmogrification->SuitableForTransmogrification(target, itemTemplate)) ||
                            !sTransmogrification->SuitableForTransmogrification(guid, itemTemplate)
                            ))
                    {
                        error = LANG_TRANSMOG_CMD_ADD_UNSUITABLE;
                        continue;
                    }
                    if (itemTemplate->Class != ITEM_CLASS_ARMOR && itemTemplate->Class != ITEM_CLASS_WEAPON)
                    {
                        error = LANG_TRANSMOG_CMD_ADD_FORBIDDEN;
                        continue;
                    }

                    if (sTransmogrification->AddCollectedAppearance(accountId, itemId))
                    {
                        CharacterDatabase.Execute("INSERT INTO custom_unlocked_appearances (account_id, item_template_id) VALUES ({}, {})", accountId, itemId);
                        added = true;
                    }
                }
            }
        }

        if (!added && error > 0)
        {
            handler->PSendModuleSysMessage("mod-transmog", error);
            handler->SetSentErrorMessage(true);
            return true;
        }

        int locale = handler->GetSessionDbcLocale();
        std::string setName = set->name[locale];
        std::string nameLink = handler->playerLink(player->GetName());

        // Feedback of command execution to GM
        if (isNotConsole)
        {
            // Failed command execution
            if (!added)
            {
                handler->PSendSysMessage("Player {} already has ItemSet |cffffffff|Hitemset:{}|h[{} {}]|h|r in the appearance collection.", nameLink, uint32(itemSetId), setName.c_str(), localeNames[locale]);
                handler->SetSentErrorMessage(true);
                return true;
            }

            // Successful command execution
            if (target != handler->GetPlayer())
                handler->PSendSysMessage("ItemSet |cffffffff|Hitemset:{}|h[{} {}]|h|r has been added to the appearance collection of Player {}.", uint32(itemSetId), setName.c_str(), localeNames[locale], nameLink);
        }

        // Notify target of new item in appearance collection
        if (target && !(target->GetPlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG).value))
            ChatHandler(target->GetSession()).PSendSysMessage("ItemSet |cffffffff|Hitemset:%d|h[{} {}]|h|r has been added to your appearance collection.", uint32(itemSetId), setName.c_str(), localeNames[locale]);

        return true;
    }

    static bool HandleTransmogPortableCommand(ChatHandler* handler)
    {
        if (!sTransmogrification->IsPortableNPCEnabled)
        {
            handler->SendErrorMessage("The portable transmogrification NPC is disabled.");
            return true;
        }

        if (!sTransmogrification->IsTransmogPlusEnabled)
        {
            handler->SendErrorMessage("The portable transmogrification NPC is a plus feature. Plus features are currently disabled.");
            return true;
        }

        Player* player = PlayerIdentifier::FromSelf(handler)->GetConnectedPlayer();

        if (!sTransmogrification->IsPlusFeatureEligible(player->GetGUID(), PLUS_FEATURE_PET))
        {
            handler->SendErrorMessage("You are not eligible for the portable transmogrification NPC. Please check your subscription level.");
            return true;
        }

        if (!sSpellMgr->GetSpellInfo(sTransmogrification->PetSpellId))
        {
            handler->SendErrorMessage("The portable transmogrification NPC spell is not available.");
            return true;
        }

        player->CastSpell((Unit*)nullptr, sTransmogrification->PetSpellId, true);
        return true;
    };

    enum class ClaimResult
    {
        Claimed,
        AlreadyOwned,
        Unsuitable
    };

    // Binds the item to the player, strips its refundable/BoP-tradeable flags and unlocks its
    // appearance in the account collection. Sends no feedback of its own; the caller decides what to
    // report. AddToDatabase shows the per-item "new appearance" message when the appearance is new.
    static ClaimResult TryClaimItem(Player* player, Item* item)
    {
        ItemTemplate const* itemTemplate = item->GetTemplate();

        // Only allow claiming if the character could actually equip the item.
        if (!sTransmogrification->SuitableForTransmogrification(player, itemTemplate))
            return ClaimResult::Unsuitable;

        uint32 accountId = player->GetSession()->GetAccountId();
        auto accIt = sTransmogrification->collectionCache.find(accountId);
        if (accIt != sTransmogrification->collectionCache.end() && accIt->second.contains(itemTemplate->ItemId))
            return ClaimResult::AlreadyOwned;

        // Claim the physical item: bind it to the player and drop the refundable/BoP-tradeable flags
        // so the appearance can't be collected and then refunded to a vendor or traded away.
        item->SetOwnerGUID(player->GetGUID());
        item->SetNotRefundable(player);
        if (!sTransmogrification->GetAllowTradeable())
            item->ClearSoulboundTradeable(player);
        item->SetBinding(true);
        item->SetState(ITEM_CHANGED, player);

        // Unlock the appearance and show the "added to your appearance collection" message (LANG_TRANSMOG_ADDED_APPEARANCE).
        sTransmogrification->AddToDatabase(player, itemTemplate);
        return ClaimResult::Claimed;
    }

    // Claims item appearances held in the player's bags without having to equip them. Forms:
    //   .transmog claim [item link]  - first instance of that item in the player's bags
    //   .transmog claim <bag> <slot> - WoW client numbering: bag 0 = backpack, 1-4 = equipped bags,
    //                                  slots start at 1 (matches GetContainerItemInfo).
    //   .transmog claim all          - every unknown, claimable appearance in the player's bags
    static bool HandleClaimCommand(ChatHandler* handler, Variant<Hyperlink<Acore::Hyperlinks::LinkTags::item>, EXACT_SEQUENCE("all"), uint8> claimArg, Optional<uint8> slotArg)
    {
        if (!sTransmogrification->GetUseCollectionSystem())
        {
            handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_DISABLED);
            handler->SetSentErrorMessage(true);
            return true;
        }

        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        // .transmog claim all - sweep every item in the backpack and equipped bags.
        if (claimArg.holds_alternative<EXACT_SEQUENCE("all")>())
        {
            uint32 claimed = 0;

            auto claimSlot = [&](Item* bagItem)
            {
                if (bagItem && TryClaimItem(player, bagItem) == ClaimResult::Claimed)
                    ++claimed;
            };

            for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
                claimSlot(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));

            for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
            {
                Bag* pBag = player->GetBagByPos(bag);
                if (!pBag)
                    continue;
                for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
                    claimSlot(player->GetItemByPos(bag, slot));
            }

            if (claimed)
            {
                handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_ALL_RESULT, claimed);
            }
            else
            {
                handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_ALL_NONE);
                handler->SetSentErrorMessage(true);
            }
            return true;
        }

        // Otherwise resolve a single item, either from a shift-clicked link or a bag/slot position.
        Item* item = nullptr;

        if (claimArg.holds_alternative<Hyperlink<Acore::Hyperlinks::LinkTags::item>>())
        {
            // Item-link form: claim the first instance of this item held in the player's bags.
            uint32 itemId = claimArg.get<Hyperlink<Acore::Hyperlinks::LinkTags::item>>()->Item->ItemId;

            // Backpack
            for (uint8 slot = INVENTORY_SLOT_ITEM_START; !item && slot < INVENTORY_SLOT_ITEM_END; ++slot)
                if (Item* bagItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    if (bagItem->GetEntry() == itemId)
                        item = bagItem;

            // Equipped bags
            for (uint8 bag = INVENTORY_SLOT_BAG_START; !item && bag < INVENTORY_SLOT_BAG_END; ++bag)
            {
                Bag* pBag = player->GetBagByPos(bag);
                if (!pBag)
                    continue;
                for (uint32 slot = 0; !item && slot < pBag->GetBagSize(); ++slot)
                    if (Item* bagItem = player->GetItemByPos(bag, slot))
                        if (bagItem->GetEntry() == itemId)
                            item = bagItem;
            }

            if (!item)
            {
                handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_NOT_IN_BAGS);
                handler->SetSentErrorMessage(true);
                return true;
            }
        }
        else
        {
            // Bag/slot form: WoW client numbering (bag 0 = backpack, 1-4 = equipped bags; 1-based slot).
            uint8 bag = claimArg.get<uint8>();
            if (!slotArg)
            {
                handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_USAGE);
                handler->SetSentErrorMessage(true);
                return true;
            }

            uint8 wowSlot = *slotArg;
            if (bag > 4 || wowSlot == 0)
            {
                handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_INVALID_SLOT);
                handler->SetSentErrorMessage(true);
                return true;
            }

            if (bag == 0)
            {
                // Backpack: WoW slot 1 maps to INVENTORY_SLOT_ITEM_START.
                uint8 maxSlots = INVENTORY_SLOT_ITEM_END - INVENTORY_SLOT_ITEM_START;
                if (wowSlot > maxSlots)
                {
                    handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_INVALID_SLOT);
                    handler->SetSentErrorMessage(true);
                    return true;
                }

                item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START + wowSlot - 1);
            }
            else
            {
                // Equipped bag: WoW bag 1-4 maps to INVENTORY_SLOT_BAG_START + (bag - 1).
                uint8 bagPos = INVENTORY_SLOT_BAG_START + (bag - 1);
                Bag* pBag = player->GetBagByPos(bagPos);
                if (!pBag || wowSlot > pBag->GetBagSize())
                {
                    handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_INVALID_SLOT);
                    handler->SetSentErrorMessage(true);
                    return true;
                }

                item = player->GetItemByPos(bagPos, wowSlot - 1);
            }

            if (!item)
            {
                handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_EMPTY_SLOT);
                handler->SetSentErrorMessage(true);
                return true;
            }
        }

        switch (TryClaimItem(player, item))
        {
            case ClaimResult::Unsuitable:
                handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_ADD_UNSUITABLE);
                handler->SetSentErrorMessage(true);
                break;
            case ClaimResult::AlreadyOwned:
                handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_CLAIM_ALREADY);
                handler->SetSentErrorMessage(true);
                break;
            case ClaimResult::Claimed:
                break; // AddToDatabase already announced the new appearance.
        }
        return true;
    }

    static bool HandleInterfaceOption(ChatHandler* handler, bool enable)
    {
        handler->GetPlayer()->UpdatePlayerSetting("mod-transmog", SETTING_VENDOR_INTERFACE, enable);
        handler->PSendModuleSysMessage("mod-transmog", enable ? LANG_TRANSMOG_CMD_VENDOR_INTERFACE_ENABLE : LANG_TRANSMOG_CMD_VENDOR_INTERFACE_DISABLE);
        return true;
    }

    static bool HandleDisclaimerOption(ChatHandler* handler, bool enable)
    {
        Player* player = handler->GetPlayer();
        if (enable)
        {
            player->UpdatePlayerSetting("mod-transmog", SETTING_HIDE_SET_DISCLAIMER, 0);
            handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_DISCLAIMER_ON);
        }
        else
        {
            player->UpdatePlayerSetting("mod-transmog", SETTING_HIDE_SET_DISCLAIMER, 1);
            handler->PSendModuleSysMessage("mod-transmog", LANG_TRANSMOG_CMD_DISCLAIMER_OFF);
        }
        return true;
    }

    static bool HandleCheckTransmog(ChatHandler* handler, PlayerIdentifier playerIdent, ItemTemplate const* destItem, ItemTemplate const* srcItem)
    {
        static constexpr char const* MOD = "mod-transmog";

        WorldSession* gSession = handler->GetSession();
        auto itemDisplay = [&](uint32 itemId) -> std::string
        {
            if (gSession)
                return sTransmogrification->GetItemLink(itemId, gSession);
            ItemTemplate const* tpl = sObjectMgr->GetItemTemplate(itemId);
            return tpl ? Acore::StringFormat("{} (ID: {})", tpl->Name1, itemId)
                       : Acore::StringFormat("ID: {}", itemId);
        };

        // Header
        handler->PSendModuleSysMessage(MOD, LANG_TRANSMOG_CHECK_HEADER);
        handler->PSendModuleSysMessage(MOD, LANG_TRANSMOG_CHECK_DEST,   itemDisplay(destItem->ItemId));
        handler->PSendModuleSysMessage(MOD, LANG_TRANSMOG_CHECK_SRC,    itemDisplay(srcItem->ItemId));
        handler->PSendModuleSysMessage(MOD, LANG_TRANSMOG_CHECK_PLAYER, playerIdent.GetName());

        // Resolve player
        ObjectGuid playerGuid = playerIdent.GetGUID();
        Player*    player     = playerIdent.GetConnectedPlayer();

        CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(playerGuid);
        if (!cache)
        {
            handler->PSendModuleSysMessage(MOD, LANG_TRANSMOG_CHECK_PLAYER_NOT_FOUND, playerIdent.GetName());
            return true;
        }

        uint32 rawGuid     = static_cast<uint32>(playerGuid.GetCounter());
        uint8  playerRace  = cache->Race;
        uint32 playerLevel = cache->Level;
        uint32 raceMask    = 1u << (playerRace - 1);
        uint32 classMask   = 1u << (cache->Class - 1);
        TeamId teamId      = Player::TeamIdForRace(playerRace);

        std::unordered_map<uint32, uint32> offlineSkills;
        if (!player)
        {
            if (QueryResult res = CharacterDatabase.Query(
                    "SELECT `skill`, `value` FROM `character_skills` WHERE `guid` = {}", rawGuid))
            {
                do {
                    Field* f = res->Fetch();
                    offlineSkills[f[0].Get<uint16>()] = f[1].Get<uint16>();
                } while (res->NextRow());
            }
        }

        auto skillValue = [&](uint32 skillId) -> uint32
        {
            if (player) return player->GetSkillValue(skillId);
            auto it = offlineSkills.find(skillId);
            return it != offlineSkills.end() ? it->second : 0u;
        };

        auto hasSpellFn = [&](uint32 spellId) -> bool
        {
            if (player) return player->HasSpell(spellId);
            return static_cast<bool>(CharacterDatabase.Query(
                "SELECT `spell` FROM `character_spell` WHERE `guid` = {} AND `spell` = {}",
                rawGuid, spellId));
        };

        // Section: collects failure messages; skips are silently ignored
        struct Section {
            bool ok          = true;
            bool whitelisted = false;
            std::vector<std::string> fails;

            void check(bool passed, std::string failMsg)
            {
                if (!passed) { ok = false; fails.push_back(std::move(failMsg)); }
            }
        };

        // Render one section line: "label  [+] All checks passed" or "label  [-] fail1; fail2"
        std::string okMsg = handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_SECTION_OK);
        auto printSection = [&](std::string const& label, Section const& sec, std::string const& whitelistMsg = "")
        {
            std::string line = label + "  ";
            if (sec.whitelisted)
            {
                line += "[~] " + whitelistMsg;
            }
            else if (sec.ok)
            {
                line += "[+] " + okMsg;
            }
            else
            {
                line += "[-] ";
                for (size_t i = 0; i < sec.fails.size(); ++i)
                {
                    if (i) line += "; ";
                    line += sec.fails[i];
                }
            }
            handler->SendSysMessage(line);
        };

        // ----------------------------------------------------------------
        // Pair checks
        // ----------------------------------------------------------------
        Section pair;

        pair.check(srcItem->ItemId != destItem->ItemId,
            handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_PAIR_IDS_SAME));
        pair.check(srcItem->DisplayInfoID != destItem->DisplayInfoID,
            handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_PAIR_DISP_SAME));
        pair.check(srcItem->Class == destItem->Class,
            handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_PAIR_CLASS_FAIL));

        auto isForbiddenType = [](uint32 t) -> bool
        {
            return t == INVTYPE_BAG    || t == INVTYPE_RELIC   || t == INVTYPE_FINGER ||
                   t == INVTYPE_TRINKET || t == INVTYPE_AMMO  || t == INVTYPE_QUIVER;
        };

        pair.check(!isForbiddenType(destItem->InventoryType),
            handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_PAIR_DEST_TYPE_FAIL));
        pair.check(!isForbiddenType(srcItem->InventoryType),
            handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_PAIR_SRC_TYPE_FAIL));

        {
            bool srcRanged  = sTransmogrification->IsRangedWeapon(srcItem->Class,  srcItem->SubClass);
            bool destRanged = sTransmogrification->IsRangedWeapon(destItem->Class, destItem->SubClass);
            pair.check(srcRanged == destRanged,
                handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_PAIR_RANGED_FAIL));
        }

        if (srcItem->SubClass != destItem->SubClass)
            pair.check(sTransmogrification->IsSubclassMismatchAllowed(player, srcItem, destItem),
                handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_PAIR_SUB_DENIED));

        if (srcItem->InventoryType != destItem->InventoryType)
            pair.check(sTransmogrification->IsInvTypeMismatchAllowed(srcItem, destItem),
                handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_PAIR_INV_DENIED));

        // ----------------------------------------------------------------
        // Per-item checks  (collect into a section, print later)
        // ----------------------------------------------------------------
        auto gatherItemChecks = [&](ItemTemplate const* proto, Section& sec)
        {
            if (sTransmogrification->IsAllowed(proto->ItemId))
            {
                sec.whitelisted = true;
                return;
            }

            {
                bool ok = proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON;
                sec.check(ok, handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_CLASS_FAIL));
                if (!ok)
                    return;
            }

            sec.check(!sTransmogrification->IsNotAllowed(proto->ItemId),
                handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_BLACKLISTED));

            sec.check(sTransmogrification->IsAllowedQuality(proto->Quality, playerGuid),
                handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_QUALITY_FAIL));

            if (proto->Class == ITEM_CLASS_WEAPON && proto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
                sec.check(sTransmogrification->AllowFishingPoles,
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_POLE_FAIL));

            if (proto->HolidayId)
                sec.check(sTransmogrification->IgnoreReqEvent || IsHolidayActive((HolidayIds)proto->HolidayId),
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_EVENT_FAIL));

            if (!sTransmogrification->IgnoreReqStats && !proto->RandomProperty && !proto->RandomSuffix && proto->StatsCount > 0)
            {
                bool hasStats = false;
                for (uint8 i = 0; i < proto->StatsCount; ++i)
                    if (proto->ItemStat[i].ItemStatValue != 0) { hasStats = true; break; }
                sec.check(hasStats, handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_STAT_FAIL));
            }

            {
                uint32 subclassSkill = proto->GetSkill();
                if (proto->SubClass > 0 && subclassSkill)
                {
                    uint32 sv = skillValue(subclassSkill);
                    bool ok;
                    if (proto->Class == ITEM_CLASS_ARMOR)
                        ok = sv > 0 || sTransmogrification->AllowMixedArmorTypes;
                    else
                        ok = sv > 0 || sTransmogrification->AllowMixedWeaponTypes == MIXED_WEAPONS_LOOSE;
                    sec.check(ok, handler->PGetParseModuleString(MOD,
                        LANG_TRANSMOG_CHECK_ITEM_PROF_FAIL, subclassSkill));
                }
            }

            if (proto->HasFlag2(ITEM_FLAG2_FACTION_HORDE))
                sec.check(teamId == TEAM_HORDE,
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_FACTION_FAIL));
            else if (proto->HasFlag2(ITEM_FLAG2_FACTION_ALLIANCE))
                sec.check(teamId == TEAM_ALLIANCE,
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_FACTION_FAIL));

            if (!sTransmogrification->IgnoreReqClass)
                sec.check((proto->AllowableClass & classMask) != 0,
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_CLASS_REQ_FAIL));

            if (!sTransmogrification->IgnoreReqRace)
                sec.check((proto->AllowableRace & raceMask) != 0,
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_RACE_REQ_FAIL));

            if (!sTransmogrification->IgnoreReqSkill && proto->RequiredSkill != 0)
            {
                uint32 sv = skillValue(proto->RequiredSkill);
                sec.check(sv >= proto->RequiredSkillRank,
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_SKILL_FAIL,
                        proto->RequiredSkill, proto->RequiredSkillRank, sv));
            }

            if (!sTransmogrification->IgnoreLevelRequirement(playerGuid))
                sec.check(playerLevel >= proto->RequiredLevel,
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_LEVEL_FAIL,
                        proto->RequiredLevel, playerLevel));

            bool skipSpell = sTransmogrification->AllowLowerTiers &&
                             sTransmogrification->TierAvailable(player, rawGuid, proto->SubClass);
            if (!sTransmogrification->IgnoreReqSpell && proto->RequiredSpell != 0 && !skipSpell)
                sec.check(hasSpellFn(proto->RequiredSpell),
                    handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_SPELL_FAIL, proto->RequiredSpell));
        };

        Section destSec, srcSec;
        gatherItemChecks(destItem, destSec);
        gatherItemChecks(srcItem, srcSec);

        // ----------------------------------------------------------------
        // Collection check
        // ----------------------------------------------------------------
        bool collOk = true;
        std::string collLine = handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_SECTION_COLL) + "  ";
        if (!sTransmogrification->GetUseCollectionSystem())
        {
            collLine += "[~] " + handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_COLL_DISABLED);
        }
        else
        {
            uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(playerGuid);
            auto const& collCache = sTransmogrification->collectionCache[accountId];
            bool inCollection = collCache.find(srcItem->ItemId) != collCache.end();
            collOk = inCollection;
            collLine += inCollection
                ? "[+] " + okMsg
                : "[-] " + handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_COLL_NOT_FOUND);
        }

        // ----------------------------------------------------------------
        // Print all section results, then final verdict
        // ----------------------------------------------------------------
        std::string whitelistMsg = handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_ITEM_WHITELISTED);
        printSection(handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_SECTION_PAIR), pair);
        printSection(handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_SECTION_ITEM, itemDisplay(destItem->ItemId)), destSec, whitelistMsg);
        printSection(handler->PGetParseModuleString(MOD, LANG_TRANSMOG_CHECK_SECTION_ITEM, itemDisplay(srcItem->ItemId)),  srcSec,  whitelistMsg);
        handler->SendSysMessage(collLine);

        bool overallOk = pair.ok && destSec.ok && srcSec.ok && collOk;
        handler->PSendModuleSysMessage(MOD, overallOk ? LANG_TRANSMOG_CHECK_RESULT_OK : LANG_TRANSMOG_CHECK_RESULT_FAIL);

        return true;
    }

    static bool HandleReloadTransmogConfig(ChatHandler* handler)
    {
        sTransmogrification->LoadConfig(true);
        handler->SendSysMessage("Transmog configs reloaded.");
        sTransmogrification->LoadCollections();
        handler->SendSysMessage("Transmog collections reloaded.");
        return true;
    }
};

void AddSC_transmog_commandscript()
{
    new transmog_commandscript();
}
