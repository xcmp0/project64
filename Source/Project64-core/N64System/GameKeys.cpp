#include "stdafx.h"

#include <Common/path.h>
#include <Project64-core/N64System/GameKeys.h>
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/Settings/SettingType/SettingsType-RDBUser.h>

void SaveGameKeys(const char * GameName, const char * DatabaseKey)
{
    // The listeners of Game_IniKey read both keys
    g_Settings->SaveString(Game_DatabaseKey, DatabaseKey);
    g_Settings->SaveString(Game_IniKey, GameName);
}

void ClearGameKeys(void)
{
    g_Settings->SaveString(Game_DatabaseKey, "");
    g_Settings->SaveString(Game_IniKey, "");
}

static void CopySaveDirectory(SettingID Directory, const char * From, const char * To)
{
    CPath Source(g_Settings->LoadStringVal(Directory).c_str(), "");
    Source.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
    CPath Target(Source);
    Source.AppendDirectory(From);
    Target.AppendDirectory(To);
    if (!Source.DirectoryExists() || Target.DirectoryExists())
    {
        return;
    }
    WriteTrace(TraceN64System, TraceInfo, "Copying saves from %s to %s", (const char *)Source, (const char *)Target);
    Target.DirectoryCreate();
    CPath File((const char *)Source, "*");
    if (File.FindFirst())
    {
        do
        {
            File.CopyTo(CPath((const char *)Target, File.GetNameExtension().c_str()), false);
        } while (File.FindNext());
    }
}

void MigrateGameKeys(const char * GameName, const char * DatabaseKey, const char * OldSaveDir)
{
    CSettingTypeRDBUser::CopySection(DatabaseKey, GameName);
    CSettingTypeApplication::CopySection(DatabaseKey, GameName);
    if (OldSaveDir == nullptr || !g_Settings->LoadBool(Setting_UniqueSaveDir))
    {
        return;
    }
    CopySaveDirectory(Directory_NativeSave, OldSaveDir, GameName);
    if (g_Settings->LoadStringVal(Directory_InstantSave) != g_Settings->LoadStringVal(Directory_NativeSave))
    {
        CopySaveDirectory(Directory_InstantSave, OldSaveDir, GameName);
    }
}
