// Per-game keys: two builds of one game (same internal name and country,
// different CRCs) share one settings section and one save directory, the
// shipped database stays keyed by CRC (with the name and country fallback),
// and the old CRC-keyed user data is copied over once.

#include "Test.h"

#include <Common/File.h>
#include <Project64-core/AppInit.h>
#include <Project64-core/N64System/GameKeys.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/Settings.h>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace
{
const char kKnownIdent[] = "11111111-22222222-C:4A";
const char kNewIdent[] = "33333333-44444444-C:4A";

void WriteText(const fs::path & File, const char * Text)
{
    fs::create_directories(File.parent_path());
    std::ofstream(File, std::ios::binary) << Text;
}

std::string ReadText(const fs::path & File)
{
    std::ifstream In(File, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(In), std::istreambuf_iterator<char>());
}
} // namespace

// A minimal big-endian (.z64) image: header CRCs, internal name and country,
// and boot code (run from DMEM at A4000040) that puts a counting loop at
// 80000400 and jumps to it. Filler after the code gives the boot area the
// word sum of a CIC-NUS-6102 boot, so the core knows the chip.
void WriteTestRom(const fs::path & File, uint32_t Crc1, uint32_t Crc2)
{
    std::vector<uint8_t> Image(0x101000, 0);
    const uint8_t Magic[4] = {0x80, 0x37, 0x12, 0x40};
    std::copy(Magic, Magic + 4, Image.begin());
    for (int i = 0; i < 4; i++)
    {
        Image[0x10 + i] = (uint8_t)(Crc1 >> (24 - 8 * i));
        Image[0x14 + i] = (uint8_t)(Crc2 >> (24 - 8 * i));
    }
    const char Name[20] = {'T', 'E', 'S', 'T', 'G', 'A', 'M', 'E', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    std::copy(Name, Name + 20, Image.begin() + 0x20);
    Image[0x3E] = 'J';
    const uint32_t Boot[] = {
        0x3C088000, // lui t0, 0x8000
        0x3C09256B, // lui t1, 0x256B         t1 = addiu t3, t3, 1
        0x35290001, // ori t1, t1, 0x0001
        0xAD090400, // sw t1, 0x400(t0)
        0x3C090800, // lui t1, 0x0800         t1 = j 0x80000400
        0x35290100, // ori t1, t1, 0x0100
        0xAD090404, // sw t1, 0x404(t0)
        0xAD000408, // sw zero, 0x408(t0)     nop (delay slot)
        0x350A0400, // ori t2, t0, 0x0400
        0x01400008, // jr t2
        0x00000000, // nop
    };
    uint64_t Missing = 0x000000D057C85244;
    size_t Offset = 0x40;
    for (size_t i = 0; i < sizeof(Boot) / sizeof(Boot[0]) || Missing != 0; i++, Offset += 4)
    {
        uint32_t Word = i < sizeof(Boot) / sizeof(Boot[0]) ? Boot[i] : (uint32_t)std::min<uint64_t>(Missing, 0xFFFFFFFF);
        Missing -= Word;
        for (int b = 0; b < 4; b++)
        {
            Image[Offset + b] = (uint8_t)(Word >> (24 - 8 * b));
        }
    }
    std::ofstream(File, std::ios::binary).write((const char *)Image.data(), Image.size());
}

int TestGameKeys(const fs::path & WorkDir)
{
    TestBegin("game-keys");
    fs::remove_all(WorkDir);
    fs::create_directories(WorkDir);

    // Shipped database: only the first build is listed
    WriteText(WorkDir / "Config" / "Project64.rdb",
              "[11111111-22222222-C:4A]\nGood Name=Test Game (J)\nInternal Name=TESTGAME\nCounter Factor=3\n");
    // User data from before the name keys: a CRC section and a <name>-<md5> save directory
    WriteText(WorkDir / "Config" / "Project64.rdb.user", "[11111111-22222222-C:4A]\nCounter Factor=1\n");
    WriteText(WorkDir / "Config" / "Project64.cfg", "[11111111-22222222-C:4A]\nCheat_Test_Active=1\n");
    WriteTestRom(WorkDir / "known.z64", 0x11111111, 0x22222222);
    WriteTestRom(WorkDir / "new.z64", 0x33333333, 0x44444444);

    TestNotification Notify;
    std::string Base = WorkDir.string() + "/";
    CHECK(AppInit(&Notify, Base.c_str(), 0, nullptr));

    {
        // The index of the name and country fallback (ROM browser and settings)
        CIniFile Database((WorkDir / "Config" / "Project64.rdb").string().c_str());
        std::map<std::string, std::string> Index;
        DatabaseNameIndex(Database, Index);
        CHECK_EQ(Index.size(), (size_t)1);
        CHECK_EQ(Index["TESTGAME-C:4A"], std::string(kKnownIdent));
    }

    {
        CN64Rom Known;
        CHECK(Known.LoadN64Image((WorkDir / "known.z64").string().c_str()));
        WriteText(WorkDir / "Save" / ("TESTGAME-" + Known.GetRomMD5()) / "TESTGAME.eep", "old save");
        Known.SaveRomSettingID(false);

        CHECK_EQ(g_Settings->LoadStringVal(Game_IniKey), std::string("TESTGAME"));
        CHECK_EQ(g_Settings->LoadStringVal(Game_DatabaseKey), std::string(kKnownIdent));
        CHECK_EQ(g_Settings->LoadStringVal(Game_UniqueSaveDir), std::string("TESTGAME"));
        CHECK_EQ(g_Settings->LoadStringVal(Rdb_GoodName), std::string("Test Game (J)"));
        CHECK_EQ(g_Settings->LoadDword(Rdb_CounterFactor), 3u);
        // Migrated from the CRC section
        CHECK_EQ(g_Settings->LoadDword(Game_CounterFactor), 1u);
        CHECK_EQ(ReadText(WorkDir / "Save" / "TESTGAME" / "TESTGAME.eep"), std::string("old save"));
        CHECK(fs::exists(WorkDir / "Save" / ("TESTGAME-" + Known.GetRomMD5()) / "TESTGAME.eep"));

        g_Settings->SaveDword(Game_CounterFactor, 4);
    }
    {
        CN64Rom New;
        CHECK(New.LoadN64Image((WorkDir / "new.z64").string().c_str()));
        New.SaveRomSettingID(false);

        CHECK_EQ(g_Settings->LoadStringVal(Game_IniKey), std::string("TESTGAME"));
        CHECK_EQ(g_Settings->LoadStringVal(Game_DatabaseKey), std::string(kNewIdent));
        CHECK_EQ(g_Settings->LoadStringVal(Game_UniqueSaveDir), std::string("TESTGAME"));
        // Not listed: the entry with the same internal name and country
        CHECK_EQ(g_Settings->LoadStringVal(Rdb_GoodName), std::string("Test Game (J)"));
        CHECK_EQ(g_Settings->LoadDword(Rdb_CounterFactor), 3u);
        CHECK(g_Settings->LoadBool(Game_Known));
        // Shared with the first build
        CHECK_EQ(g_Settings->LoadDword(Game_CounterFactor), 4u);
        CHECK(!fs::exists(WorkDir / "Save" / ("TESTGAME-" + New.GetRomMD5())));

        g_Settings->DeleteSetting(Game_CounterFactor);
        CHECK_EQ(g_Settings->LoadDword(Game_CounterFactor), 3u);
        g_Settings->SaveDword(Game_CounterFactor, 5);
        New.ClearRomSettingID();
        CHECK_EQ(g_Settings->LoadStringVal(Game_IniKey), std::string(""));
    }
    AppCleanup();

    std::string User = ReadText(WorkDir / "Config" / "Project64.rdb.user");
    CHECK(User.find("[TESTGAME]") != std::string::npos);
    CHECK(User.find("Counter Factor=5") != std::string::npos);
    // The CRC section is copied, never removed
    CHECK(User.find("[11111111-22222222-C:4A]") != std::string::npos);
    CHECK(User.find("[33333333-44444444-C:4A]") == std::string::npos);
    // The chosen cheats follow the name too
    std::string Config = ReadText(WorkDir / "Config" / "Project64.cfg");
    std::string::size_type Section = Config.find("[TESTGAME]");
    CHECK(Section != std::string::npos);
    CHECK(Config.find("Cheat_Test_Active=1", Section) != std::string::npos);
    return TestEnd();
}

// --basedir replaces the frontend's base directory, and relative paths in the
// settings resolve against it
int TestBaseDir(const fs::path & WorkDir)
{
    TestBegin("base-dir");
    fs::remove_all(WorkDir);
    WriteText(WorkDir / "Config" / "Project64.cfg", "[Native Save Directory]\nDirectory=MySaves/\nUse Selected=1\n");

    TestNotification Notify;
    std::string Dir = WorkDir.string();
    char Arg0[] = "Project64-test", Arg1[] = "--basedir";
    std::vector<char> Arg2(Dir.begin(), Dir.end());
    Arg2.push_back('\0');
    char * Argv[] = {Arg0, Arg1, Arg2.data(), nullptr};
    const std::string Separator(1, (char)fs::path::preferred_separator);
    CHECK(AppInit(&Notify, "/nonexistent-base/", 3, Argv));
    CHECK_EQ(g_Settings->LoadStringVal(Cmd_BaseDirectory), Dir + Separator);
    CHECK_EQ(g_Settings->LoadStringVal(SupportFile_RomDatabase), (WorkDir / "Config" / "Project64.rdb").string());
    CHECK_EQ(g_Settings->LoadStringVal(Directory_NativeSave), (WorkDir / "MySaves").string() + Separator);
    AppCleanup();
    return TestEnd();
}
