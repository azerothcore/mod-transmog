DELETE FROM `command` WHERE `name` IN ('transmog', 'transmog add', 'transmog sync', 'transmog add set');
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('transmog', 0, 'Syntax: .transmog <on/off>\nAllows seeing transmogrified items and the transmogrifier NPC.'),
('transmog add', 1, 'Syntax: .transmog add $player $item\nAdds an item to a player\'s appearance collection.'),
('transmog sync', 0, 'Syntax: .transmog sync\nSyncs transmog addon appearances with the server.'),
('transmog add set', 1, 'Syntax: .transmog add set $player $itemSet\nAdds items of an item set to a player\'s appearance collection.');