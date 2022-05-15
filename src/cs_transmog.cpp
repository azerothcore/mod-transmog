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

#include "Chat.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
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
            { "set", HandleAddTransmogItemSet,    SEC_MODERATOR, Console::No },
            { "",    HandleAddTransmogItem,       SEC_MODERATOR, Console::No },
        };

        static ChatCommandTable transmogTable =
        {
            { "add", addCollectionTable },
            { "",    HandleDisableTransMogVisual, SEC_PLAYER,    Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "transmog", transmogTable },
        };

        return commandTable;
    }

    static bool HandleDisableTransMogVisual(ChatHandler* handler, bool hide)
    {
        Player* player = handler->GetPlayer();

        if (hide)
        {
            player->UpdatePlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG, 0);
            handler->SendSysMessage(LANG_CMD_TRANSMOG_SHOW);
        }
        else
        {
            player->UpdatePlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG, 1);
            handler->SendSysMessage(LANG_CMD_TRANSMOG_HIDE);
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
        if (!player || !player->IsConnected())
            return false;

        Player* target = player->GetConnectedPlayer();

        if (!sTransmogrification->GetTrackUnusableItems() && !sTransmogrification->SuitableForTransmogrification(target, itemTemplate))
        {
            handler->SendSysMessage(LANG_CMD_TRANSMOG_ADD);
            handler->SetSentErrorMessage(true);
            return true;
        }

        if (itemTemplate->Class != ITEM_CLASS_ARMOR && itemTemplate->Class != ITEM_CLASS_WEAPON)
            return true;

        uint32 itemId = itemTemplate->ItemId;
        uint32 accountId = target->GetSession()->GetAccountId();
        std::stringstream tempStream;
        tempStream << std::hex << ItemQualityColors[itemTemplate->Quality];
        std::string itemQuality = tempStream.str();
        std::string query = "SELECT account_id, item_template_id FROM custom_unlocked_appearances WHERE account_id = " + std::to_string(accountId) + " AND item_template_id = " + std::to_string(itemId);
        target->GetSession()->GetQueryProcessor().AddCallback(CharacterDatabase.AsyncQuery(query).WithCallback([=](QueryResult result)
        {
            if (!result)
            {
                if (!(target->GetPlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG).value))
                {
                    ChatHandler(target->GetSession()).PSendSysMessage(R"(|c%s|Hitem:%u:0:0:0:0:0:0:0:0|h[%s]|h|r has been added to your appearance collection.)", itemQuality.c_str(), itemId, itemTemplate->Name1.c_str());
                }
                CharacterDatabase.Execute("INSERT INTO custom_unlocked_appearances (account_id, item_template_id) VALUES ({}, {})", accountId, itemId);
            }
        }));

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
        if (!player || !player->IsConnected())
            return false;

        Player* target = player->GetConnectedPlayer();
        ItemSetEntry const* set = sItemSetStore.LookupEntry(uint32(itemSetId));

        if (!set)
        {
            handler->PSendSysMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, uint32(itemSetId));
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 itemId;
        for (uint32 i = 0; i < MAX_ITEM_SET_ITEMS; ++i)
        {
            itemId = set->itemId[i];
            if (itemId)
            {
                ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
                if (itemTemplate)
                {
                    if (!sTransmogrification->GetTrackUnusableItems() && !sTransmogrification->SuitableForTransmogrification(target, itemTemplate))
                        continue;
                    if (itemTemplate->Class != ITEM_CLASS_ARMOR && itemTemplate->Class != ITEM_CLASS_WEAPON)
                        continue;

                    uint32 accountId = target->GetSession()->GetAccountId();
                    std::string query = "SELECT account_id, item_template_id FROM custom_unlocked_appearances WHERE account_id = " + std::to_string(accountId) + " AND item_template_id = " + std::to_string(itemId);
                    target->GetSession()->GetQueryProcessor().AddCallback(CharacterDatabase.AsyncQuery(query).WithCallback([=](QueryResult result)
                    {
                        if (!result)
                            CharacterDatabase.Execute("INSERT INTO custom_unlocked_appearances (account_id, item_template_id) VALUES ({}, {})", accountId, itemId);
                    }));
                }
            }
        }

        if (!(target->GetPlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG).value))
        {
            int locale = handler->GetSessionDbcLocale();
            std::string setName = set->name[locale];
            ChatHandler(target->GetSession()).PSendSysMessage("ItemSet |cffffffff|Hitemset:%d|h[%s %s]|h|r has been added to your appearance collection.", uint32(itemSetId), setName.c_str(), localeNames[locale]);
        }

        return true;
    }
};

void AddSC_transmog_commandscript()
{
    new transmog_commandscript();
}
