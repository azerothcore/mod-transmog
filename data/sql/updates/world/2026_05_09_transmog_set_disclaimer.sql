-- Set bonus disclaimer strings and command (entries 11119-11121)
DELETE FROM `acore_string` WHERE `entry` BETWEEN 11119 AND 11121;
INSERT INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`)
VALUES
(11119, '|cFF4DB3FFSet bonuses won''t appear in the item tooltip while transmogrified, but they are still fully active.\nTo stop seeing this notice, type |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF.|r',
    '|cFF4DB3FF변형 중에는 아이템 툴팁에 세트 보너스가 표시되지 않지만, 여전히 완전히 활성화되어 있습니다.\n이 알림을 중지하려면 |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF 을 입력하세요.|r',
    '|cFF4DB3FFLes bonus de set n''apparaîtront pas dans l''info-bulle de l''objet lors de la transmogrification, mais ils restent pleinement actifs.\nPour arrêter d''afficher cette notice, tapez |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF.|r',
    '|cFF4DB3FFSetboni werden in der Gegenstandsbeschreibung während der Transmogrifizierung nicht angezeigt, sind aber weiterhin voll aktiv.\nUm diesen Hinweis zu deaktivieren, gib |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF ein.|r',
    '|cFF4DB3FF幻化后物品提示中不会显示套装加成，但套装加成仍然完全有效。\n若要停止显示此提示，请输入 |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF。|r',
    '|cFF4DB3FF幻化後物品提示中不會顯示套裝加成，但套裝加成仍然完全有效。\n若要停止顯示此提示，請輸入 |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF。|r',
    '|cFF4DB3FFLas bonificaciones de conjunto no aparecerán en la descripción del objeto mientras esté transfigurado, pero siguen activas.\nPara dejar de ver este aviso, escribe |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF.|r',
    '|cFF4DB3FFLas bonificaciones de conjunto no aparecerán en la descripción del objeto mientras esté transfigurado, pero siguen activas.\nPara dejar de ver este aviso, escribe |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF.|r',
    '|cFF4DB3FFБонусы набора не будут отображаться в подсказке предмета при трансмогрификации, но они по-прежнему полностью активны.\nЧтобы скрыть это уведомление, введите |cFFFFFFFF.transmog disclaimer off|cFF4DB3FF.|r'),
(11120, 'Set bonus disclaimer enabled.',
    '세트 보너스 알림이 활성화되었습니다.',
    'Avertissement de bonus de set activé.',
    'Setbonus-Hinweis aktiviert.',
    '套装加成提示已启用。',
    '套裝加成提示已啟用。',
    'Aviso de bonificación de conjunto activado.',
    'Aviso de bonificación de conjunto activado.',
    'Уведомление о бонусах набора включено.'),
(11121, 'Set bonus disclaimer disabled.',
    '세트 보너스 알림이 비활성화되었습니다.',
    'Avertissement de bonus de set désactivé.',
    'Setbonus-Hinweis deaktiviert.',
    '套装加成提示已禁用。',
    '套裝加成提示已禁用。',
    'Aviso de bonificación de conjunto desactivado.',
    'Aviso de bonificación de conjunto desactivado.',
    'Уведомление о бонусах набора отключено.');

DELETE FROM `command` WHERE `name` = 'transmog disclaimer';
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('transmog disclaimer', 0, 'Syntax: .transmog disclaimer <on/off>\nToggles the set bonus disclaimer notice shown when transmogrifying set items.');
