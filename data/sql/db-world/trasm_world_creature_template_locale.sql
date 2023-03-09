SET
@Entry = 190010;
DELETE FROM `creature_template_locale` WHERE `entry` = 190010;
INSERT INTO `creature_template_locale` (`entry`, `locale`, `Name`, `Title`, `VerifiedBuild`) VALUES
(@Entry, "zhCN", "时光编制者", "幻化师", 18019);