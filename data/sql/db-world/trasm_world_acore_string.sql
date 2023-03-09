SET @STRING_ENTRY := 11100;

DELETE FROM `acore_string` WHERE `entry` IN  (@STRING_ENTRY+0,@STRING_ENTRY+1,@STRING_ENTRY+2,@STRING_ENTRY+3,@STRING_ENTRY+4,@STRING_ENTRY+5,@STRING_ENTRY+6,@STRING_ENTRY+7,@STRING_ENTRY+8,@STRING_ENTRY+9,@STRING_ENTRY+10, @STRING_ENTRY+11, @STRING_ENTRY+12, @STRING_ENTRY+13, @STRING_ENTRY+14, @STRING_ENTRY+15, @STRING_ENTRY+16);

INSERT INTO `acore_string` (`entry`, `content_default`) VALUES
(@STRING_ENTRY+0, 'Item successfully transmogrified.'),
(@STRING_ENTRY+1, 'Equipment slot is empty.'),
(@STRING_ENTRY+2, 'Invalid source item selected.'),
(@STRING_ENTRY+3, 'Source item does not exist.'),
(@STRING_ENTRY+4, 'Destination item does not exist.'),
(@STRING_ENTRY+5, 'Selected items are invalid.'),
(@STRING_ENTRY+6, 'You don''t have enough money.'),
(@STRING_ENTRY+7, 'You don''t have enough tokens.'),
(@STRING_ENTRY+8, 'All your transmogrifications were removed.'),
(@STRING_ENTRY+9, 'No transmogrification found.'),
(@STRING_ENTRY+10, 'Invalid name inserted.'),
(@STRING_ENTRY+11, 'Showing transmogrified items, relog to update the current area.'),
(@STRING_ENTRY+12, 'Hiding transmogrified items, relog to update the current area.'),
(@STRING_ENTRY+13, 'The selected item is not suitable for transmogrification.'),
(@STRING_ENTRY+14, 'The selected item cannot be used for transmogrification of the target player.'),
(@STRING_ENTRY+15, 'Performing transmog appearance sync....'),
(@STRING_ENTRY+16, 'Appearance sync complete.');

UPDATE acore_string SET
    locale_zhCN = 
        CASE entry
            WHEN @STRING_ENTRY+0 THEN '物品幻化成功。'
            WHEN @STRING_ENTRY+1 THEN '装备栏为空。'
            WHEN @STRING_ENTRY+2 THEN '选择的源物品无效。'
            WHEN @STRING_ENTRY+3 THEN '源物品不存在。'
            WHEN @STRING_ENTRY+4 THEN '目标物品不存在。'
            WHEN @STRING_ENTRY+5 THEN '选择的物品无效。'
            WHEN @STRING_ENTRY+6 THEN '你没有足够的钱。'
            WHEN @STRING_ENTRY+7 THEN '你没有足够的代币。'
            WHEN @STRING_ENTRY+8 THEN '你的所有幻化已被移除。'
            WHEN @STRING_ENTRY+9 THEN '未找到幻化。'
            WHEN @STRING_ENTRY+10 THEN '插入的名称无效。'
            WHEN @STRING_ENTRY+11 THEN '显示幻化后的物品，请重新登录以更新当前区域。'
            WHEN @STRING_ENTRY+12 THEN '隐藏幻化后的物品，请重新登录以更新当前区域。'
            WHEN @STRING_ENTRY+13 THEN '选择的物品不适用于幻化。'
            WHEN @STRING_ENTRY+14 THEN '所选物品无法用于目标玩家的幻化。'
            WHEN @STRING_ENTRY+15 THEN '执行幻化外观同步....'
            WHEN @STRING_ENTRY+16 THEN '外观同步完成。'
            ELSE locale_zhCN
        END;