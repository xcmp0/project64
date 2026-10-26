#pragma once

// What the user and the plugins write for a game (game settings, the chosen
// cheats and enhancements, saves) is keyed by the game's internal name, so
// every build of the game shares it. The shipped databases stay keyed by the
// ROM's CRC ident.

// Sets Game_DatabaseKey (CRC ident) and Game_IniKey (name), in that order
void SaveGameKeys(const char * GameName, const char * DatabaseKey);
void ClearGameKeys(void);

// One-time move from the CRC keys, before SaveGameKeys: an empty name section
// takes the keys of the CRC section, and a missing save directory <GameName>
// takes the files of <OldSaveDir> (copied, never deleted; nullptr skips saves)
void MigrateGameKeys(const char * GameName, const char * DatabaseKey, const char * OldSaveDir);
