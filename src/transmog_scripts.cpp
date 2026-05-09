/*
5.0
Transmogrification 3.3.5a - Gossip menu
By Rochet2

ScriptName for NPC:
Creature_Transmogrify

TODO:
Make DB saving even better (Deleting)? What about coding?

Fix the cost formula
-- Too much data handling, use default costs

Are the qualities right?
Blizzard might have changed the quality requirements.
(TC handles it with stat checks)

Cant transmogrify rediculus items // Foereaper: would be fun to stab people with a fish
-- Cant think of any good way to handle this easily, could rip flagged items from cata DB
*/
#include "Transmogrification.h"
#include "Chat.h"
#include "ScriptedCreature.h"
#include "ItemTemplate.h"
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"

#define sT sTransmogrification

static inline const std::string& Tstr(WorldSession* session, uint32 id)
{
    return *session->GetModuleString("mod-transmog", id);
}



const uint32 FALLBACK_HIDE_ITEM_VENDOR_ID   = 9172; //Invisibility potion
const uint32 FALLBACK_REMOVE_TMOG_VENDOR_ID = 1049; //Tablet of Purge
const uint32 CUSTOM_HIDE_ITEM_VENDOR_ID     = 57575;//Custom Hide Item item
const uint32 CUSTOM_REMOVE_TMOG_VENDOR_ID   = 57576;//Custom Remove Transmog item


uint32 GetTransmogPrice (ItemTemplate const* targetItem)
{
    uint32 price = sT->GetSpecialPrice(targetItem);
    price *= sT->GetScaledCostModifier();
    price += sT->GetCopperCost();
    return price;
}

bool ValidForTransmog (Player* player, Item* target, Item* source, bool hasSearch, std::string searchTerm)
{
    if (!target || !source || !player) return false;
    ItemTemplate const* targetTemplate = target->GetTemplate();
    ItemTemplate const* sourceTemplate = source->GetTemplate();

    if (!sT->CanTransmogrifyItemWithItem(player, targetTemplate, sourceTemplate))
        return false;
    if (sT->GetFakeEntry(target->GetGUID()) == source->GetEntry())
        return false;
    if (hasSearch && sourceTemplate->Name1.find(searchTerm) == std::string::npos)
        return false;
    return true;
}

bool CmpTmog (Item* i1, Item* i2)
{
    const ItemTemplate* i1t = i1->GetTemplate();
    const ItemTemplate* i2t = i2->GetTemplate();
    const int q1 = 7-i1t->Quality;
    const int q2 = 7-i2t->Quality;
    return std::tie(q1, i1t->Name1) < std::tie(q2, i2t->Name1);
}

std::vector<Item*> GetValidTransmogs (Player* player, Item* target, bool hasSearch, std::string searchTerm)
{
    std::vector<Item*> allowedItems;
    if (!target) return allowedItems;

    if (sT->GetUseCollectionSystem())
    {
        uint32 accountId = player->GetSession()->GetAccountId();
        if (sT->collectionCache.find(accountId) == sT->collectionCache.end())
            return allowedItems;

        for (uint32 itemId : sT->collectionCache[accountId])
        {
            if (!sObjectMgr->GetItemTemplate(itemId))
                continue;
            Item* srcItem = Item::CreateItem(itemId, 1, 0);
            if (ValidForTransmog(player, target, srcItem, hasSearch, searchTerm))
                allowedItems.push_back(srcItem);
        }
    }
    else
    {
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            Item* srcItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (ValidForTransmog(player, target, srcItem, hasSearch, searchTerm))
                allowedItems.push_back(srcItem);
        }
        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            Bag* bag = player->GetBagByPos(i);
            if (!bag)
                continue;
            for (uint32 j = 0; j < bag->GetBagSize(); ++j)
            {
                Item* srcItem = player->GetItemByPos(i, j);
                if (ValidForTransmog(player, target, srcItem, hasSearch, searchTerm))
                    allowedItems.push_back(srcItem);
            }
        }
    }

    if (sConfigMgr->GetOption<bool>("Transmogrification.EnableSortByQualityAndName", true)) {
        sort(allowedItems.begin(), allowedItems.end(), CmpTmog);
    }

    return allowedItems;
}

void PerformTransmogrification (Player* player, uint32 itemEntry, uint32 cost)
{
    uint8 slot = sT->selectionCache[player->GetGUID()];
    WorldSession* session = player->GetSession();
    if (!player->HasEnoughMoney(cost))
    {
        ChatHandler(session).SendNotification(Tstr(session, LANG_TRANSMOG_NOT_ENOUGH_MONEY));
        return;
    }
    TransmogStrings res = sT->Transmogrify(player, itemEntry, slot);
    if (res == LANG_TRANSMOG_OK)
    {
        session->SendAreaTriggerMessage("{}", Tstr(session, LANG_TRANSMOG_OK));

        if (sT->ShowSetDisclaimer &&
            !player->GetPlayerSetting("mod-transmog", SETTING_HIDE_SET_DISCLAIMER).value)
        {
            if (Item* destItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                if (destItem->GetTemplate()->ItemSet)
                    ChatHandler(session).PSendSysMessage("{}", Tstr(session, LANG_TRANSMOG_SET_DISCLAIMER));
            }
        }
    }
    else
        ChatHandler(session).SendNotification(Tstr(session, res));
}

void RemoveTransmogrification (Player* player)
{
    uint8 slot = sT->selectionCache[player->GetGUID()];
    WorldSession* session = player->GetSession();
    if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
    {
        if (sT->GetFakeEntry(newItem->GetGUID()))
        {
            sT->DeleteFakeEntry(player, slot, newItem);
            session->SendAreaTriggerMessage("{}", Tstr(session, LANG_TRANSMOG_UNTRANSMOG_OK));
        }
        else
            ChatHandler(session).SendNotification(Tstr(session, LANG_TRANSMOG_UNTRANSMOG_NO_TRANSMOGS));
    }
}

class npc_transmogrifier : public CreatureScript
{
public:
    npc_transmogrifier() : CreatureScript("npc_transmogrifier") { }

    struct npc_transmogrifierAI : ScriptedAI
    {
        npc_transmogrifierAI(Creature* creature) : ScriptedAI(creature) { };

        bool CanBeSeen(Player const* player) override
        {
            Player* target = ObjectAccessor::FindConnectedPlayer(player->GetGUID());

            if (sT->IsPortableNPCEnabled)
            {
                if (TempSummon* summon = me->ToTempSummon())
                {
                    return summon->GetOwner() == player;
                }
            }

            return sTransmogrification->IsEnabled() && (target && !target->GetPlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG).IsEnabled());
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_transmogrifierAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        WorldSession* session = player->GetSession();

        // Clear the search string for the player
        sT->searchStringByPlayer.erase(player->GetGUID().GetCounter());

        if (sT->GetEnableTransmogInfo())
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Misc_Book_11:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_HOWWORKS), EQUIPMENT_SLOT_END + 9, 0);
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (const char* slotName = sT->GetSlotName(slot, session))
            {
                Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                uint32 entry = newItem ? sT->GetFakeEntry(newItem->GetGUID()) : 0;
                std::string icon = entry ? sT->GetItemIcon(entry, 30, 30, -18, 0) : sT->GetSlotIcon(slot, 30, 30, -18, 0);
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, icon + std::string(slotName), EQUIPMENT_SLOT_END, slot);
            }
        }
#ifdef PRESETS
        if (sT->GetEnableSets())
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/RAIDFRAME/UI-RAIDFRAME-MAINASSIST:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_MANAGESETS), EQUIPMENT_SLOT_END + 4, 0);
#endif
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Enchant_Disenchant:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_REMOVETRANSMOG), EQUIPMENT_SLOT_END + 2, 0, Tstr(session, LANG_TRANSMOG_REMOVETRANSMOG_ASK), 0, false);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_UPDATEMENU), EQUIPMENT_SLOT_END + 1, 0);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        WorldSession* session = player->GetSession();
        // Next page
        if (sender > EQUIPMENT_SLOT_END + 10)
        {
            ShowTransmogItemsInGossipMenu(player, creature, action, sender);
            return true;
        }
        switch (sender)
        {
            case EQUIPMENT_SLOT_END: // Show items you can use
            {
                sT->selectionCache[player->GetGUID()] = action;

                bool useVendorInterface = player->GetPlayerSetting("mod-transmog", SETTING_VENDOR_INTERFACE).IsEnabled();

                if (sT->GetUseVendorInterface() || useVendorInterface)
                    ShowTransmogItemsInFakeVendor(player, creature, action);
                else
                    ShowTransmogItemsInGossipMenu(player, creature, action, sender);

                break;
            }
            case EQUIPMENT_SLOT_END + 1: // Main menu
                OnGossipHello(player, creature);
                break;
            case EQUIPMENT_SLOT_END + 2: // Remove Transmogrifications
            {
                bool removed = false;
                auto trans = CharacterDatabase.BeginTransaction();
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        if (!sT->GetFakeEntry(newItem->GetGUID()))
                            continue;
                        sT->DeleteFakeEntry(player, slot, newItem, &trans);
                        removed = true;
                    }
                }
                if (removed)
                {
                    session->SendAreaTriggerMessage("{}", Tstr(session, LANG_TRANSMOG_UNTRANSMOG_OK));
                    CharacterDatabase.CommitTransaction(trans);
                }
                else
                    ChatHandler(session).SendNotification(Tstr(session, LANG_TRANSMOG_UNTRANSMOG_NO_TRANSMOGS));
                OnGossipHello(player, creature);
            } break;
            case EQUIPMENT_SLOT_END + 3: // Remove Transmogrification from single item
            {
                RemoveTransmogrification(player);
                OnGossipSelect(player, creature, EQUIPMENT_SLOT_END, action);
            } break;
    #ifdef PRESETS
            case EQUIPMENT_SLOT_END + 4: // Presets menu
            {
                if (!sT->GetEnableSets())
                {
                    OnGossipHello(player, creature);
                    return true;
                }
                if (sT->GetEnableSetInfo())
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Misc_Book_11:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_HOWSETSWORK), EQUIPMENT_SLOT_END + 10, 0);
                for (Transmogrification::presetIdMap::const_iterator it = sT->presetByName[player->GetGUID()].begin(); it != sT->presetByName[player->GetGUID()].end(); ++it)
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Misc_Statue_02:30:30:-18:0|t" + it->second, EQUIPMENT_SLOT_END + 6, it->first);

                if (sT->presetByName[player->GetGUID()].size() < sT->GetMaxSets())
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/GuildBankFrame/UI-GuildBankFrame-NewTab:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_SAVESET), EQUIPMENT_SLOT_END + 8, 0);
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Ability_Spy:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_BACK), EQUIPMENT_SLOT_END + 1, 0);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            } break;
            case EQUIPMENT_SLOT_END + 5: // Use preset
            {
                if (!sT->GetEnableSets())
                {
                    OnGossipHello(player, creature);
                    return true;
                }
                // action = presetID
                for (Transmogrification::slotMap::const_iterator it = sT->presetById[player->GetGUID()][action].begin(); it != sT->presetById[player->GetGUID()][action].end(); ++it)
                {
                    if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, it->first))
                        sT->PresetTransmog(player, item, it->second, it->first);
                }
                OnGossipSelect(player, creature, EQUIPMENT_SLOT_END + 6, action);
            } break;
            case EQUIPMENT_SLOT_END + 6: // view preset
            {
                if (!sT->GetEnableSets())
                {
                    OnGossipHello(player, creature);
                    return true;
                }
                // action = presetID
                for (Transmogrification::slotMap::const_iterator it = sT->presetById[player->GetGUID()][action].begin(); it != sT->presetById[player->GetGUID()][action].end(); ++it)
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, sT->GetItemIcon(it->second, 30, 30, -18, 0) + sT->GetItemLink(it->second, session), sender, action);

                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Misc_Statue_02:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_USESET), EQUIPMENT_SLOT_END + 5, action, Tstr(session, LANG_TRANSMOG_CONFIRM_USESET) + sT->presetByName[player->GetGUID()][action], 0, false);
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/PaperDollInfoFrame/UI-GearManager-LeaveItem-Opaque:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_DELETESET), EQUIPMENT_SLOT_END + 7, action, Tstr(session, LANG_TRANSMOG_CONFIRM_DELETESET) + sT->presetByName[player->GetGUID()][action] + "?", 0, false);
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Ability_Spy:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_BACK), EQUIPMENT_SLOT_END + 4, 0);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            } break;
            case EQUIPMENT_SLOT_END + 7: // Delete preset
            {
                if (!sT->GetEnableSets())
                {
                    OnGossipHello(player, creature);
                    return true;
                }
                // action = presetID
                CharacterDatabase.Execute("DELETE FROM `custom_transmogrification_sets` WHERE Owner = {} AND PresetID = {}", player->GetGUID().GetCounter(), action);
                sT->presetById[player->GetGUID()][action].clear();
                sT->presetById[player->GetGUID()].erase(action);
                sT->presetByName[player->GetGUID()].erase(action);

                OnGossipSelect(player, creature, EQUIPMENT_SLOT_END + 4, 0);
            } break;
            case EQUIPMENT_SLOT_END + 8: // Save preset
            {
                if (!sT->GetEnableSets() || sT->presetByName[player->GetGUID()].size() >= sT->GetMaxSets())
                {
                    OnGossipHello(player, creature);
                    return true;
                }
                uint32 cost = 0;
                bool canSave = false;
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (!sT->GetSlotName(slot, session))
                        continue;
                    if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        uint32 entry = sT->GetFakeEntry(newItem->GetGUID());
                        if (!entry)
                            continue;
                        const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
                        if (!temp)
                            continue;
                        if (!sT->SuitableForTransmogrification(player, temp)) // no need to check?
                            continue;
                        cost += sT->GetSpecialPrice(temp);
                        canSave = true;
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, sT->GetItemIcon(entry, 30, 30, -18, 0) + sT->GetItemLink(entry, session), EQUIPMENT_SLOT_END + 8, 0);
                    }
                }
                if (canSave)
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/GuildBankFrame/UI-GuildBankFrame-NewTab:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_SAVESET), 0, 0, Tstr(session, LANG_TRANSMOG_INSERTSETNAME), cost*sT->GetSetCostModifier() + sT->GetSetCopperCost(), true);
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_UPDATEMENU), sender, action);
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Ability_Spy:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_BACK), EQUIPMENT_SLOT_END + 4, 0);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            } break;
            case EQUIPMENT_SLOT_END + 10: // Set info
            {
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Ability_Spy:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_BACK), EQUIPMENT_SLOT_END + 4, 0);
                SendGossipMenuFor(player, sT->GetSetNpcText(), creature->GetGUID());
            } break;
    #endif
            case EQUIPMENT_SLOT_END + 9: // Transmog info
            {
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Ability_Spy:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_BACK), EQUIPMENT_SLOT_END + 1, 0);
                SendGossipMenuFor(player, sT->GetTransmogNpcText(), creature->GetGUID());
            } break;
            default: // Transmogrify
            {
                if (!sender && !action)
                {
                    OnGossipHello(player, creature);
                    return true;
                }
                PerformTransmogrification(player, action, sender);
                CloseGossipMenuFor(player); // Wait for SetMoney to get fixed, issue #10053
            } break;
        }
        return true;
    }

#ifdef PRESETS
    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (sender)
        {
            // "sender" is an equipment slot for a search - execute the search
            std::string searchString(code);
            if (searchString.length() > MAX_SEARCH_STRING_LENGTH)
                searchString = searchString.substr(0, MAX_SEARCH_STRING_LENGTH);
            sT->searchStringByPlayer.erase(player->GetGUID().GetCounter());
            sT->searchStringByPlayer.insert({player->GetGUID().GetCounter(), searchString});
            OnGossipSelect(player, creature, EQUIPMENT_SLOT_END, sender - 1);
            return true;
        }
        if (action)
            return true; // should never happen
        if (!sT->GetEnableSets())
        {
            OnGossipHello(player, creature);
            return true;
        }
        std::string name(code);
        if (name.find('"') != std::string::npos || name.find('\\') != std::string::npos)
            ChatHandler(player->GetSession()).SendNotification(Tstr(player->GetSession(), LANG_TRANSMOG_PRESET_ERR_INVALID_NAME));
        else
        {
            for (uint8 presetID = 0; presetID < sT->GetMaxSets(); ++presetID) // should never reach over max
            {
                if (sT->presetByName[player->GetGUID()].find(presetID) != sT->presetByName[player->GetGUID()].end())
                    continue; // Just remember never to use presetByName[pGUID][presetID] when finding etc!

                int32 cost = 0;
                std::map<uint8, uint32> items;
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (!sT->GetSlotName(slot, player->GetSession()))
                        continue;
                    if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        uint32 entry = sT->GetFakeEntry(newItem->GetGUID());
                        if (!entry)
                            continue;
                        if (entry != HIDDEN_ITEM_ID)
                        {
                            const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
                            if (!temp)
                                continue;
                            if (!sT->SuitableForTransmogrification(player, temp))
                                continue;
                            cost += sT->GetSpecialPrice(temp);
                        }
                        items[slot] = entry;
                    }
                }
                if (items.empty())
                    break; // no transmogrified items were found to be saved
                cost *= sT->GetSetCostModifier();
                cost += sT->GetSetCopperCost();
                if (!player->HasEnoughMoney(cost))
                {
                    ChatHandler(player->GetSession()).SendNotification(Tstr(player->GetSession(), LANG_TRANSMOG_NOT_ENOUGH_MONEY));
                    break;
                }

                std::ostringstream ss;
                for (std::map<uint8, uint32>::iterator it = items.begin(); it != items.end(); ++it)
                {
                    ss << uint32(it->first) << ' ' << it->second << ' ';
                    sT->presetById[player->GetGUID()][presetID][it->first] = it->second;
                }
                sT->presetByName[player->GetGUID()][presetID] = name; // Make sure code doesnt mess up SQL!
                CharacterDatabase.Execute("REPLACE INTO `custom_transmogrification_sets` (`Owner`, `PresetID`, `SetName`, `SetData`) VALUES ({}, {}, \"{}\", \"{}\")", player->GetGUID().GetCounter(), uint32(presetID), name, ss.str());
                if (cost)
                    player->ModifyMoney(-cost);
                break;
            }
        }
        //OnGossipSelect(player, creature, EQUIPMENT_SLOT_END+4, 0);
        CloseGossipMenuFor(player); // Wait for SetMoney to get fixed, issue #10053
        return true;
    }
#endif

    void ShowTransmogItemsInGossipMenu(Player* player, Creature* creature, uint8 slot, uint16 gossipPageNumber) // Only checks bags while can use an item from anywhere in inventory
    {
        WorldSession* session = player->GetSession();
        Item* oldItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        bool hasSearchString;

        uint16 pageNumber = 0;
        uint32 startValue = 0;
        uint32 endValue = MAX_OPTIONS - 4;
        bool lastPage = true;
        if (gossipPageNumber > EQUIPMENT_SLOT_END + 10)
        {
            pageNumber = gossipPageNumber - EQUIPMENT_SLOT_END - 10;
            startValue = (pageNumber * (MAX_OPTIONS - 2));
            endValue = (pageNumber + 1) * (MAX_OPTIONS - 2) - 1;
        }

        if (oldItem)
        {
            uint32 price = GetTransmogPrice(oldItem->GetTemplate());
            std::ostringstream ss;
            ss << std::endl;
            if (sT->GetRequireToken())
                ss << std::endl << std::endl << sT->GetTokenAmount() << " x " << sT->GetItemLink(sT->GetTokenEntry(), session);
            std::string lineEnd = ss.str();

            std::unordered_map<uint32, std::string>::iterator searchStringIterator = sT->searchStringByPlayer.find(player->GetGUID().GetCounter());
            hasSearchString = !(searchStringIterator == sT->searchStringByPlayer.end());
            std::string searchDisplayValue(hasSearchString ? searchStringIterator->second : Tstr(session, LANG_TRANSMOG_SEARCH));
            std::vector<Item*> allowedItems = GetValidTransmogs(player, oldItem, hasSearchString, searchDisplayValue);

            if (allowedItems.size() > 0)
            {
                lastPage = false;
                // Offset values to add Search gossip item
                if (pageNumber == 0)
                {
                    if (hasSearchString)
                    {
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, sT->GetItemIcon(30620, 30, 30, -18, 0) + Tstr(session, LANG_TRANSMOG_SEARCHING_FOR) + searchDisplayValue, slot + 1, 0, Tstr(session, LANG_TRANSMOG_SEARCH_FOR_ITEM), 0, true);
                    }
                    else
                    {
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, sT->GetItemIcon(30620, 30, 30, -18, 0) + Tstr(session, LANG_TRANSMOG_SEARCH), slot + 1, 0, Tstr(session, LANG_TRANSMOG_SEARCH_FOR_ITEM), 0, true);
                    }
                }
                else
                {
                    startValue--;
                }
                if (sT->GetAllowHiddenTransmog())
                {
                    // Offset the start and end values to make space for invisible item entry
                    endValue--;
                    if (pageNumber != 0)
                    {
                        startValue--;
                    }
                    else
                    {
                        // Add invisible item entry
                        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/inv_misc_enggizmos_27:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_HIDESLOT), slot, UINT_MAX, Tstr(session, LANG_TRANSMOG_CONFIRM_HIDE_ITEM) + lineEnd, 0, false);
                    }
                }
                for (uint32 i = startValue; i <= endValue; i++)
                {
                    if (allowedItems.empty() || i > allowedItems.size() - 1)
                    {
                        lastPage = true;
                        break;
                    }
                    Item* newItem = allowedItems.at(i);
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, sT->GetItemIcon(newItem->GetEntry(), 30, 30, -18, 0) + sT->GetItemLink(newItem, session), slot, newItem->GetEntry(), Tstr(session, LANG_TRANSMOG_CONFIRM_USEITEM) + sT->GetItemIcon(newItem->GetEntry(), 40, 40, -15, -10) + sT->GetItemLink(newItem, session) + lineEnd, price, false);
                }
            }
            if (gossipPageNumber == EQUIPMENT_SLOT_END + 11)
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, Tstr(session, LANG_TRANSMOG_PREVIOUS_PAGE), EQUIPMENT_SLOT_END, slot);
                if (!lastPage)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, Tstr(session, LANG_TRANSMOG_NEXT_PAGE), gossipPageNumber + 1, slot);
            }
            else if (gossipPageNumber > EQUIPMENT_SLOT_END + 11)
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, Tstr(session, LANG_TRANSMOG_PREVIOUS_PAGE), gossipPageNumber - 1, slot);
                if (!lastPage)
                    AddGossipItemFor(player, GOSSIP_ICON_CHAT, Tstr(session, LANG_TRANSMOG_NEXT_PAGE), gossipPageNumber + 1, slot);
            }
            else if (!lastPage)
            {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, Tstr(session, LANG_TRANSMOG_NEXT_PAGE), EQUIPMENT_SLOT_END + 11, slot);
            }

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Enchant_Disenchant:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_REMOVETRANSMOG), EQUIPMENT_SLOT_END + 3, slot, Tstr(session, LANG_TRANSMOG_REMOVETRANSMOG_SLOT), 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_UPDATEMENU), EQUIPMENT_SLOT_END, slot);
        }
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/Ability_Spy:30:30:-18:0|t" + Tstr(session, LANG_TRANSMOG_BACK), EQUIPMENT_SLOT_END + 1, 0);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    static std::vector<ItemTemplate const*> GetSpoofedVendorItems (Item* target)
    {
        std::vector<ItemTemplate const*> spoofedItems;
        uint32 existingTransmog = sT->GetFakeEntry(target->GetGUID());
        if (sT->AllowHiddenTransmog && !existingTransmog)
        {
            ItemTemplate const* _hideSlotButton = sObjectMgr->GetItemTemplate(CUSTOM_HIDE_ITEM_VENDOR_ID);
            if (_hideSlotButton)
                spoofedItems.push_back(_hideSlotButton);
            else
            {
                _hideSlotButton = sObjectMgr->GetItemTemplate(FALLBACK_HIDE_ITEM_VENDOR_ID);
                spoofedItems.push_back(_hideSlotButton);
            }
        }
        if (existingTransmog)
        {
            ItemTemplate const* _removeTransmogButton = sObjectMgr->GetItemTemplate(CUSTOM_REMOVE_TMOG_VENDOR_ID);
            if (_removeTransmogButton)
                spoofedItems.push_back(_removeTransmogButton);
            else
            {
                _removeTransmogButton = sObjectMgr->GetItemTemplate(FALLBACK_REMOVE_TMOG_VENDOR_ID);
                spoofedItems.push_back(_removeTransmogButton);
            }
        }
        return spoofedItems;
    }

    static uint32 GetSpoofedItemPrice (uint32 itemId, ItemTemplate const* target)
    {
        switch (itemId)
        {
            case CUSTOM_HIDE_ITEM_VENDOR_ID:
            case FALLBACK_HIDE_ITEM_VENDOR_ID:
                return sT->HiddenTransmogIsFree ? 0 : sT->GetSpecialPrice(target);
            default:
                return 0;
        }
    }

    static void EncodeItemToPacket (WorldPacket& data, ItemTemplate const* proto, uint8& slot, uint32 price)
    {
        data << uint32(slot + 1);
        data << uint32(proto->ItemId);
        data << uint32(proto->DisplayInfoID);
        data << int32 (-1); //Infinite Stock
        data << uint32(price);
        data << uint32(proto->MaxDurability);
        data << uint32(1);  //Buy Count of 1
        data << uint32(0);
        slot++;
    }

    //The actual vendor options are handled in the player script below, OnBeforeBuyItemFromVendor
    static void ShowTransmogItemsInFakeVendor (Player* player, Creature* creature, uint8 slot)
    {
        Item* targetItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!targetItem)
        {
            ChatHandler(player->GetSession()).SendNotification(Tstr(player->GetSession(), LANG_TRANSMOG_MISSING_DEST_ITEM));
            CloseGossipMenuFor(player);
            return;
        }
        ItemTemplate const* targetTemplate = targetItem->GetTemplate();

        std::vector<Item*> itemList = GetValidTransmogs(player, targetItem, false, "");
        std::vector<ItemTemplate const*> spoofedItems = GetSpoofedVendorItems(targetItem);

        uint32 itemCount = itemList.size();
        uint32 spoofCount = spoofedItems.size();
        uint32 totalItems = itemCount + spoofCount;
        uint32 price = GetTransmogPrice(targetItem->GetTemplate());

        WorldPacket data(SMSG_LIST_INVENTORY, 8 + 1 + totalItems * 8 * 4);
        data << uint64(creature->GetGUID().GetRawValue());

        uint8 count = 0;
        size_t count_pos = data.wpos();
        data << uint8(count);

        for (uint32 i = 0; i < spoofCount && count < MAX_VENDOR_ITEMS; ++i)
        {
            EncodeItemToPacket (
                data, spoofedItems[i], count,
                GetSpoofedItemPrice(spoofedItems[i]->ItemId, targetTemplate)
            );
        }
        for (uint32 i = 0; i < itemCount && count < MAX_VENDOR_ITEMS; ++i)
        {
            ItemTemplate const* _proto = itemList[i]->GetTemplate();
            if (_proto) EncodeItemToPacket(data, _proto, count, price);
        }

        data.put(count_pos, count);
        player->GetSession()->SendPacket(&data);
    }
};

class PS_Transmogrification : public PlayerScript
{
private:
    void AddToDatabase(Player* player, Item* item)
    {
        if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE) && !sTransmogrification->GetAllowTradeable())
            return;
        if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_REFUNDABLE))
            return;
        ItemTemplate const* itemTemplate = item->GetTemplate();
        AddToDatabase(player, itemTemplate);
    }

    void AddToDatabase(Player* player, ItemTemplate const* itemTemplate)
    {
        if (!sT->GetTrackUnusableItems() && !sT->SuitableForTransmogrification(player, itemTemplate))
            return;
        if (itemTemplate->Class != ITEM_CLASS_ARMOR && itemTemplate->Class != ITEM_CLASS_WEAPON)
            return;
        uint32 itemId = itemTemplate->ItemId;
        WorldSession* session = player->GetSession();
        uint32 accountId = session->GetAccountId();
        std::string itemName = itemTemplate->Name1;

        int loc_idex = session->GetSessionDbLocaleIndex();
        if (ItemLocale const* il = sObjectMgr->GetItemLocale(itemId))
            ObjectMgr::GetLocaleString(il->Name, loc_idex, itemName);

        std::stringstream tempStream;
        tempStream << std::hex << ItemQualityColors[itemTemplate->Quality];
        std::string itemQuality = tempStream.str();
        bool showChatMessage = !(player->GetPlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG).value) && !sT->CanNeverTransmog(itemTemplate);
        if (sT->AddCollectedAppearance(accountId, itemId))
        {
            if (showChatMessage)
                ChatHandler(session).PSendSysMessage(R"(|c{}|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r {})", itemQuality, itemId, itemName, Tstr(session, LANG_TRANSMOG_ADDED_APPEARANCE));

            CharacterDatabase.Execute("INSERT INTO custom_unlocked_appearances (account_id, item_template_id) VALUES ({}, {})", accountId, itemId);
        }
    }

    void CheckRetroActiveQuestAppearances(Player* player)
    {
        const RewardedQuestSet& rewQuests = player->getRewardedQuests();
        for (RewardedQuestSet::const_iterator itr = rewQuests.begin(); itr != rewQuests.end(); ++itr)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(*itr);
            OnPlayerCompleteQuest(player, quest);
        }
        player->UpdatePlayerSetting("mod-transmog", SETTING_RETROACTIVE_CHECK, 1);
    }
public:
    PS_Transmogrification() : PlayerScript("Player_Transmogrify", {
        PLAYERHOOK_ON_EQUIP,
        PLAYERHOOK_ON_LOOT_ITEM,
        PLAYERHOOK_ON_CREATE_ITEM,
        PLAYERHOOK_ON_AFTER_STORE_OR_EQUIP_NEW_ITEM,
        PLAYERHOOK_ON_PLAYER_COMPLETE_QUEST,
        PLAYERHOOK_ON_AFTER_SET_VISIBLE_ITEM_SLOT,
        PLAYERHOOK_ON_AFTER_MOVE_ITEM_FROM_INVENTORY,
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_LOGOUT,
        PLAYERHOOK_ON_BEFORE_BUY_ITEM_FROM_VENDOR
    }) { }

    void OnPlayerEquip(Player* player, Item* it, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        if (!sT->GetUseCollectionSystem())
            return;
        AddToDatabase(player, it);
    }

    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override
    {
        if (!sT->GetUseCollectionSystem() || !item || typeid(*item) != typeid(Item))
            return;
        if (item->GetTemplate()->Bonding == ItemBondingType::BIND_WHEN_PICKED_UP || item->IsSoulBound())
        {
            AddToDatabase(player, item);
        }
    }

    void OnPlayerCreateItem(Player* player, Item* item, uint32 /*count*/) override
    {
        if (!sT->GetUseCollectionSystem())
            return;
        if (item->GetTemplate()->Bonding == ItemBondingType::BIND_WHEN_PICKED_UP || item->IsSoulBound())
        {
            AddToDatabase(player, item);
        }
    }

    void OnPlayerAfterStoreOrEquipNewItem(Player* player, uint32 /*vendorslot*/, Item* item, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/, ItemTemplate const* /*pProto*/, Creature* /*pVendor*/, VendorItem const* /*crItem*/, bool /*bStore*/) override
    {
        if (!sT->GetUseCollectionSystem())
            return;
        if (item->GetTemplate()->Bonding == ItemBondingType::BIND_WHEN_PICKED_UP || item->IsSoulBound())
        {
            AddToDatabase(player, item);
        }
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (!sT->GetUseCollectionSystem() || !quest)
            return;
        for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        {
            uint32 itemId = uint32(quest->RewardChoiceItemId[i]);
            if (!itemId)
                continue;
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
            AddToDatabase(player, itemTemplate);
        }

        for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
        {
            uint32 itemId = uint32(quest->RewardItemId[i]);
            if (!itemId)
                continue;
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
            AddToDatabase(player, itemTemplate);
        }
    }

    void OnPlayerAfterSetVisibleItemSlot(Player* player, uint8 slot, Item *item) override
    {
        if (!item)
            return;

        if (uint32 entry = sT->GetFakeEntry(item->GetGUID()))
        {
            player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (slot * 2), entry);
        }
    }

    void OnPlayerAfterMoveItemFromInventory(Player* /*player*/, Item* it, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        sT->DeleteFakeFromDB(it->GetGUID().GetCounter());
    }

    void OnPlayerLogin(Player* player) override
    {
        if (sT->EnableResetRetroActiveAppearances())
            player->UpdatePlayerSetting("mod-transmog", SETTING_RETROACTIVE_CHECK, 0);

        if (sT->EnableRetroActiveAppearances() && !(player->GetPlayerSetting("mod-transmog", SETTING_RETROACTIVE_CHECK).value))
            CheckRetroActiveQuestAppearances(player);

        ObjectGuid playerGUID = player->GetGUID();
        sT->entryMap.erase(playerGUID);
        QueryResult result = CharacterDatabase.Query("SELECT GUID, FakeEntry FROM custom_transmogrification WHERE Owner = {}", player->GetGUID().GetCounter());
        if (result)
        {
            do
            {
                ObjectGuid itemGUID = ObjectGuid::Create<HighGuid::Item>((*result)[0].Get<uint32>());
                uint32 fakeEntry = (*result)[1].Get<uint32>();
                if (fakeEntry == HIDDEN_ITEM_ID || sObjectMgr->GetItemTemplate(fakeEntry))
                {
                    sT->dataMap[itemGUID] = playerGUID;
                    sT->entryMap[playerGUID][itemGUID] = fakeEntry;
                }
            } while (result->NextRow());

            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    player->SetVisibleItemSlot(slot, item);
            }
        }

#ifdef PRESETS
        if (sT->GetEnableSets())
            sT->LoadPlayerSets(playerGUID);
#endif
    }

    void OnPlayerLogout(Player* player) override
    {
        ObjectGuid pGUID = player->GetGUID();
        for (Transmogrification::transmog2Data::const_iterator it = sT->entryMap[pGUID].begin(); it != sT->entryMap[pGUID].end(); ++it)
            sT->dataMap.erase(it->first);
        sT->entryMap.erase(pGUID);
        sT->selectionCache.erase(pGUID);

#ifdef PRESETS
        if (sT->GetEnableSets())
            sT->UnloadPlayerSets(pGUID);
#endif
    }

    void OnPlayerBeforeBuyItemFromVendor(Player* player, ObjectGuid vendorguid, uint32 /*vendorslot*/, uint32& itemEntry, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/) override
    {
        Creature* vendor = player->GetMap()->GetCreature(vendorguid);
        if (!vendor)
            return;

        if (!sT->IsTransmogVendor(vendor->GetEntry()))
            return;

        uint8 slot = sT->selectionCache[player->GetGUID()];

        if (itemEntry == CUSTOM_HIDE_ITEM_VENDOR_ID || itemEntry == FALLBACK_HIDE_ITEM_VENDOR_ID)
        {
            PerformTransmogrification(player, UINT_MAX, 0);
        }
        else if (itemEntry == CUSTOM_REMOVE_TMOG_VENDOR_ID || itemEntry == FALLBACK_REMOVE_TMOG_VENDOR_ID)
        {
            RemoveTransmogrification(player);
        }
        else
        {
            PerformTransmogrification(player, itemEntry, 0);
        }
        npc_transmogrifier::ShowTransmogItemsInFakeVendor(player, vendor, slot); //Refresh menu
        itemEntry = 0; //Prevents the handler from proceeding to core vendor handling
    }
};

class WS_Transmogrification : public WorldScript
{
public:
    WS_Transmogrification() : WorldScript("WS_Transmogrification", {
        WORLDHOOK_ON_STARTUP
    }) { }

    void OnStartup() override
    {
        sT->LoadConfig(false);
        //sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Deleting non-existing transmogrification entries...");
        CharacterDatabase.Execute("DELETE FROM custom_transmogrification WHERE NOT EXISTS (SELECT 1 FROM item_instance WHERE item_instance.guid = custom_transmogrification.GUID)");

#ifdef PRESETS
        // Clean even if disabled
        // Dont delete even if player has more presets than should
        CharacterDatabase.Execute("DELETE FROM `custom_transmogrification_sets` WHERE NOT EXISTS(SELECT 1 FROM characters WHERE characters.guid = custom_transmogrification_sets.Owner)");
#endif

        sT->LoadCollections();
    }
};

class global_transmog_script : public GlobalScript
{
public:
    global_transmog_script() : GlobalScript("global_transmog_script", {
        GLOBALHOOK_ON_ITEM_DEL_FROM_DB,
        GLOBALHOOK_ON_MIRRORIMAGE_DISPLAY_ITEM
    }) { }

    void OnItemDelFromDB(CharacterDatabaseTransaction trans, ObjectGuid::LowType itemGuid) override
    {
        sT->DeleteFakeFromDB(itemGuid, &trans);
    }

    void OnMirrorImageDisplayItem(const Item *item, uint32 &display) override
    {
        if (uint32 entry = sTransmogrification->GetFakeEntry(item->GetGUID()))
        {
            if (entry == HIDDEN_ITEM_ID)
            {
                display = 0;
            }
            else
            {
                display=uint32(sObjectMgr->GetItemTemplate(entry)->DisplayInfoID);
            }
        }
    }
};

class unit_transmog_script : public UnitScript
{
public:
    unit_transmog_script() : UnitScript("unit_transmog_script", true, {
        UNITHOOK_SHOULD_TRACK_VALUES_UPDATE_POS_BY_INDEX,
        UNITHOOK_ON_PATCH_VALUES_UPDATE
    }) { }

    bool ShouldTrackValuesUpdatePosByIndex(Unit const* unit, uint8 /*updateType*/, uint16 index) override
    {
        return unit->IsPlayer() && index >= PLAYER_VISIBLE_ITEM_1_ENTRYID && index <= PLAYER_VISIBLE_ITEM_19_ENTRYID && (index & 1);
    }

    void OnPatchValuesUpdate(Unit const* unit, ByteBuffer& valuesUpdateBuf, BuildValuesCachePosPointers& posPointers, Player* target) override
    {
        if (!unit->IsPlayer())
            return;

        for (auto it = posPointers.other.begin(); it != posPointers.other.end(); ++it)
        {
            uint16 index = it->first;
            if (index >= PLAYER_VISIBLE_ITEM_1_ENTRYID && index <= PLAYER_VISIBLE_ITEM_19_ENTRYID && (index & 1))
                if (Item* item = unit->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, ((index - PLAYER_VISIBLE_ITEM_1_ENTRYID) / 2U)))
                    if (!sTransmogrification->IsEnabled() || target->GetPlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG).value)
                        valuesUpdateBuf.put(it->second, item->GetEntry());
        }
    }
};

void AddSC_Transmog()
{
    new global_transmog_script();
    new unit_transmog_script();
    new npc_transmogrifier();
    new PS_Transmogrification();
    new WS_Transmogrification();
}
