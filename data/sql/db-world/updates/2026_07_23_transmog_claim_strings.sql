-- Strings for the `.transmog claim` command, sent via PSendModuleSysMessage.
DELETE FROM `module_string` WHERE `module` = 'mod-transmog' AND `id` BETWEEN 81 AND 88;
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
-- Claim command strings
('mod-transmog', 81, 'Could not claim item: appearance collection is disabled on this server.'),
('mod-transmog', 82, 'You do not have that item in your bags.'),
('mod-transmog', 83, 'Usage: .transmog claim [item link], .transmog claim <bag> <slot> (bag 0 = backpack, 1-4 = bags; slots start at 1), or .transmog claim all.'),
('mod-transmog', 84, 'Invalid bag or slot. Bags are 0 (backpack) to 4, and slots start at 1.'),
('mod-transmog', 85, 'There is no item in that bag slot.'),
('mod-transmog', 86, 'Claimed {} new appearance(s) from your bags.'),
('mod-transmog', 87, 'No new appearances to claim from your bags.'),
('mod-transmog', 88, 'You have already collected this appearance.');

DELETE FROM `module_string_locale` WHERE `module` = 'mod-transmog' AND `id` BETWEEN 81 AND 88;
INSERT INTO `module_string_locale` (`module`, `id`, `locale`, `string`) VALUES
-- ID 81: CLAIM_DISABLED
('mod-transmog', 81, 'koKR', '아이템을 획득할 수 없습니다: 이 서버에서는 외형 컬렉션이 비활성화되어 있습니다.'),
('mod-transmog', 81, 'frFR', 'Impossible de collecter l''apparence : la collection d''apparences est désactivée sur ce serveur.'),
('mod-transmog', 81, 'deDE', 'Gegenstand konnte nicht beansprucht werden: Die Aussehen-Sammlung ist auf diesem Server deaktiviert.'),
('mod-transmog', 81, 'zhCN', '无法认领物品：本服务器已禁用外观收藏系统。'),
('mod-transmog', 81, 'zhTW', '無法認領物品：本伺服器已停用外觀收藏系統。'),
('mod-transmog', 81, 'esES', 'No se pudo reclamar el objeto: la colección de apariencias está desactivada en este servidor.'),
('mod-transmog', 81, 'esMX', 'No se pudo reclamar el objeto: la colección de apariencias está desactivada en este servidor.'),
('mod-transmog', 81, 'ruRU', 'Не удалось получить предмет: коллекция обликов отключена на этом сервере.'),
-- ID 82: CLAIM_NOT_IN_BAGS
('mod-transmog', 82, 'koKR', '가방에 해당 아이템이 없습니다.'),
('mod-transmog', 82, 'frFR', 'Vous n''avez pas cet objet dans votre inventaire.'),
('mod-transmog', 82, 'deDE', 'Du hast diesen Gegenstand nicht in deinen Taschen.'),
('mod-transmog', 82, 'zhCN', '你的背包中没有该物品。'),
('mod-transmog', 82, 'zhTW', '你的背包中沒有該物品。'),
('mod-transmog', 82, 'esES', 'No tienes ese objeto en tus bolsas.'),
('mod-transmog', 82, 'esMX', 'No tienes ese objeto en tus bolsas.'),
('mod-transmog', 82, 'ruRU', 'У вас нет этого предмета в сумках.'),
-- ID 83: CLAIM_USAGE
('mod-transmog', 83, 'koKR', '사용법: .transmog claim [아이템 링크], .transmog claim <가방> <칸> (가방 0 = 기본 가방, 1-4 = 착용한 가방; 칸은 1부터 시작), 또는 .transmog claim all.'),
('mod-transmog', 83, 'frFR', 'Utilisation : .transmog claim [lien d''objet], .transmog claim <sac> <emplacement> (sac 0 = sac à dos, 1-4 = sacs ; les emplacements commencent à 1), ou .transmog claim all.'),
('mod-transmog', 83, 'deDE', 'Verwendung: .transmog claim [Gegenstandslink], .transmog claim <Tasche> <Platz> (Tasche 0 = Rucksack, 1-4 = Taschen; Plätze beginnen bei 1) oder .transmog claim all.'),
('mod-transmog', 83, 'zhCN', '用法：.transmog claim [物品链接]、.transmog claim <背包> <格子>（背包 0 = 主背包，1-4 = 已装备的背包；格子从 1 开始），或 .transmog claim all。'),
('mod-transmog', 83, 'zhTW', '用法：.transmog claim [物品連結]、.transmog claim <背包> <格子>（背包 0 = 主背包，1-4 = 已裝備的背包；格子從 1 開始），或 .transmog claim all。'),
('mod-transmog', 83, 'esES', 'Uso: .transmog claim [enlace de objeto], .transmog claim <bolsa> <espacio> (bolsa 0 = mochila, 1-4 = bolsas; los espacios empiezan en 1), o .transmog claim all.'),
('mod-transmog', 83, 'esMX', 'Uso: .transmog claim [enlace de objeto], .transmog claim <bolsa> <espacio> (bolsa 0 = mochila, 1-4 = bolsas; los espacios empiezan en 1), o .transmog claim all.'),
('mod-transmog', 83, 'ruRU', 'Использование: .transmog claim [ссылка на предмет], .transmog claim <сумка> <ячейка> (сумка 0 = рюкзак, 1-4 = сумки; ячейки начинаются с 1), или .transmog claim all.'),
-- ID 84: CLAIM_INVALID_SLOT
('mod-transmog', 84, 'koKR', '잘못된 가방 또는 칸입니다. 가방은 0(기본 가방)부터 4까지이며, 칸은 1부터 시작합니다.'),
('mod-transmog', 84, 'frFR', 'Sac ou emplacement invalide. Les sacs vont de 0 (sac à dos) à 4, et les emplacements commencent à 1.'),
('mod-transmog', 84, 'deDE', 'Ungültige Tasche oder ungültiger Platz. Taschen gehen von 0 (Rucksack) bis 4, und Plätze beginnen bei 1.'),
('mod-transmog', 84, 'zhCN', '背包或格子无效。背包为 0（主背包）到 4，格子从 1 开始。'),
('mod-transmog', 84, 'zhTW', '背包或格子無效。背包為 0（主背包）到 4，格子從 1 開始。'),
('mod-transmog', 84, 'esES', 'Bolsa o espacio no válido. Las bolsas van de 0 (mochila) a 4, y los espacios empiezan en 1.'),
('mod-transmog', 84, 'esMX', 'Bolsa o espacio no válido. Las bolsas van de 0 (mochila) a 4, y los espacios empiezan en 1.'),
('mod-transmog', 84, 'ruRU', 'Неверная сумка или ячейка. Сумки — от 0 (рюкзак) до 4, ячейки начинаются с 1.'),
-- ID 85: CLAIM_EMPTY_SLOT
('mod-transmog', 85, 'koKR', '해당 가방 칸에 아이템이 없습니다.'),
('mod-transmog', 85, 'frFR', 'Aucun objet dans cet emplacement de sac.'),
('mod-transmog', 85, 'deDE', 'In diesem Taschenplatz befindet sich kein Gegenstand.'),
('mod-transmog', 85, 'zhCN', '该背包格子中没有物品。'),
('mod-transmog', 85, 'zhTW', '該背包格子中沒有物品。'),
('mod-transmog', 85, 'esES', 'No hay ningún objeto en ese espacio de la bolsa.'),
('mod-transmog', 85, 'esMX', 'No hay ningún objeto en ese espacio de la bolsa.'),
('mod-transmog', 85, 'ruRU', 'В этой ячейке сумки нет предмета.'),
-- ID 86: CLAIM_ALL_RESULT
('mod-transmog', 86, 'koKR', '가방에서 새로운 외형 {}개를 획득했습니다.'),
('mod-transmog', 86, 'frFR', '{} nouvelle(s) apparence(s) collectée(s) depuis vos sacs.'),
('mod-transmog', 86, 'deDE', '{} neue(s) Aussehen aus deinen Taschen beansprucht.'),
('mod-transmog', 86, 'zhCN', '已从背包中认领 {} 个新外观。'),
('mod-transmog', 86, 'zhTW', '已從背包中認領 {} 個新外觀。'),
('mod-transmog', 86, 'esES', 'Se reclamaron {} apariencia(s) nueva(s) de tus bolsas.'),
('mod-transmog', 86, 'esMX', 'Se reclamaron {} apariencia(s) nueva(s) de tus bolsas.'),
('mod-transmog', 86, 'ruRU', 'Получено новых обликов из сумок: {}.'),
-- ID 87: CLAIM_ALL_NONE
('mod-transmog', 87, 'koKR', '가방에서 획득할 새로운 외형이 없습니다.'),
('mod-transmog', 87, 'frFR', 'Aucune nouvelle apparence à collecter dans vos sacs.'),
('mod-transmog', 87, 'deDE', 'Keine neuen Aussehen zum Beanspruchen in deinen Taschen.'),
('mod-transmog', 87, 'zhCN', '背包中没有可认领的新外观。'),
('mod-transmog', 87, 'zhTW', '背包中沒有可認領的新外觀。'),
('mod-transmog', 87, 'esES', 'No hay apariencias nuevas que reclamar en tus bolsas.'),
('mod-transmog', 87, 'esMX', 'No hay apariencias nuevas que reclamar en tus bolsas.'),
('mod-transmog', 87, 'ruRU', 'В сумках нет новых обликов для получения.'),
-- ID 88: CLAIM_ALREADY
('mod-transmog', 88, 'koKR', '이미 이 외형을 수집했습니다.'),
('mod-transmog', 88, 'frFR', 'Vous avez déjà collecté cette apparence.'),
('mod-transmog', 88, 'deDE', 'Du hast dieses Aussehen bereits gesammelt.'),
('mod-transmog', 88, 'zhCN', '你已经收集了该外观。'),
('mod-transmog', 88, 'zhTW', '你已經收集了該外觀。'),
('mod-transmog', 88, 'esES', 'Ya has coleccionado esta apariencia.'),
('mod-transmog', 88, 'esMX', 'Ya has coleccionado esta apariencia.'),
('mod-transmog', 88, 'ruRU', 'Вы уже собрали этот облик.');

DELETE FROM `command` WHERE `name` = 'transmog claim';
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('transmog claim', 0, 'Syntax:\n.transmog claim [item link]\n.transmog claim <bag> <slot>\n.transmog claim all\nClaims the appearance of an item in your bags without equipping it, binding the item to you. Provide a shift-clicked item link, a bag and slot position (bag 0 = backpack, 1-4 = equipped bags; slots start at 1), or "all" to claim every new appearance in your bags.');
