// Project64-input-script: controller input read from a script, for the
// headless frontend and other unattended runs.
//
// Settings, in Project64.cfg:
//
//   [InputScript]
//   ScriptFile=<path of the script; empty: no buttons pressed>
//   Pak=<1 none, 2 controller pak (default), 3 rumble pak> for controller 1
//
// Script lines (# starts a comment):
//
//   <frame> [p<controller>] <state>
//
// The state holds from that frame until the next line for the same
// controller (1 to 4, default 1): button names A B Z START L R CU CD CL CR DU
// DD DL DR, the stick as X=<n> Y=<n> (-128 to 127), or - for nothing pressed.
// Frames count the core's reads of controller 1, from 0 when the ROM opens.
// With "Update Controller On Refresh" (which the headless frontend sets),
// the core reads the controllers once per vertical interrupt, so a frame is
// a VI.
//
// Controller 1 is always present; controllers 2 to 4 are present when the
// script names them.

#include <Project64-plugin-spec/Input.h>
#include <Settings/Settings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <string>
#include <vector>

namespace
{
enum
{
    Set_ScriptFile = 1,
    Set_Pak = 2,
};

struct Step
{
    uint32_t Frame;
    uint32_t Buttons;
};

struct ButtonName
{
    const char * Name;
    uint32_t Mask;
};

// Bits of BUTTONS::Value
const ButtonName Buttons[] = {
    {"DR", 0x0001}, {"DL", 0x0002}, {"DD", 0x0004}, {"DU", 0x0008},
    {"START", 0x0010}, {"Z", 0x0020}, {"B", 0x0040}, {"A", 0x0080},
    {"CR", 0x0100}, {"CL", 0x0200}, {"CD", 0x0400}, {"CU", 0x0800},
    {"R", 0x1000}, {"L", 0x2000},
};

CONTROL * g_Controls = nullptr;
std::vector<Step> g_Script[4];
uint32_t g_Frame = 0;
uint32_t g_Reads = 0;

void Report(const char * File, int Line, const char * Problem)
{
    fprintf(stderr, "input script %s:%d: %s\n", File, Line, Problem);
}

bool ParseState(const char * File, int Line, char * Token, uint32_t & Value)
{
    for (; Token != nullptr; Token = strtok(nullptr, " \t\r\n"))
    {
        if (strcmp(Token, "-") == 0)
        {
            continue;
        }
        if ((Token[0] == 'X' || Token[0] == 'x' || Token[0] == 'Y' || Token[0] == 'y') && Token[1] == '=')
        {
            long Axis = strtol(Token + 2, nullptr, 10);
            if (Axis < -128 || Axis > 127)
            {
                Report(File, Line, "stick value out of range");
                return false;
            }
            int Shift = (Token[0] == 'X' || Token[0] == 'x') ? 16 : 24;
            Value = (Value & ~(0xFFu << Shift)) | ((uint32_t)(Axis & 0xFF) << Shift);
            continue;
        }
        bool Found = false;
        for (const ButtonName & Button : Buttons)
        {
            if (strcasecmp(Token, Button.Name) == 0)
            {
                Value |= Button.Mask;
                Found = true;
                break;
            }
        }
        if (!Found)
        {
            Report(File, Line, (std::string("unknown button ") + Token).c_str());
            return false;
        }
    }
    return true;
}

void LoadScript(void)
{
    for (std::vector<Step> & Script : g_Script)
    {
        Script.clear();
    }
    char File[1024];
    GetSettingSz(Set_ScriptFile, File, sizeof(File));
    if (File[0] == '\0')
    {
        return;
    }
    FILE * Input = fopen(File, "r");
    if (Input == nullptr)
    {
        Report(File, 0, "cannot open the script");
        return;
    }
    char Text[1024];
    for (int Line = 1; fgets(Text, sizeof(Text), Input) != nullptr; Line++)
    {
        char * Comment = strchr(Text, '#');
        if (Comment != nullptr)
        {
            *Comment = '\0';
        }
        char * Token = strtok(Text, " \t\r\n");
        if (Token == nullptr)
        {
            continue;
        }
        char * End;
        unsigned long Frame = strtoul(Token, &End, 10);
        if (*End != '\0')
        {
            Report(File, Line, "a line starts with a frame number");
            continue;
        }
        int Controller = 0;
        Token = strtok(nullptr, " \t\r\n");
        if (Token != nullptr && (Token[0] == 'p' || Token[0] == 'P') && Token[1] >= '1' && Token[1] <= '4' && Token[2] == '\0')
        {
            Controller = Token[1] - '1';
            Token = strtok(nullptr, " \t\r\n");
        }
        uint32_t Value = 0;
        if (!ParseState(File, Line, Token, Value))
        {
            continue;
        }
        std::vector<Step> & Script = g_Script[Controller];
        if (!Script.empty() && Script.back().Frame > Frame)
        {
            Report(File, Line, "frames go backwards");
            continue;
        }
        Script.push_back(Step{(uint32_t)Frame, Value});
    }
    fclose(Input);
}

uint32_t StateAt(int Controller, uint32_t Frame)
{
    uint32_t Value = 0;
    for (const Step & Entry : g_Script[Controller])
    {
        if (Entry.Frame > Frame)
        {
            break;
        }
        Value = Entry.Buttons;
    }
    return Value;
}

void SetupControllers(void)
{
    if (g_Controls == nullptr)
    {
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        g_Controls[i].Present = (i == 0 || !g_Script[i].empty()) ? PRESENT_CONT : PRESENT_NONE;
        g_Controls[i].RawData = false;
        g_Controls[i].Plugin = PLUGIN_NONE;
    }
    uint32_t Pak = GetSetting(Set_Pak);
    g_Controls[0].Plugin = (Pak >= PLUGIN_NONE && Pak <= PLUGIN_RUMBLE_PAK) ? (int32_t)Pak : PLUGIN_MEMPAK;
}
} // namespace

EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = CONTROLLER_SPECS_VERSION;
    PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
    snprintf(PluginInfo->Name, sizeof(PluginInfo->Name), "Project64 input script");
    PluginInfo->Reserved1 = false;
    PluginInfo->Reserved2 = true;
}

EXPORT void CALL PluginLoaded(void)
{
    SetModuleName("InputScript");
    RegisterSetting(Set_ScriptFile, Data_String_General, "ScriptFile", "", 0, "");
    RegisterSetting(Set_Pak, Data_DWORD_General, "Pak", "", PLUGIN_MEMPAK, nullptr);
}

EXPORT void CALL InitiateControllers(CONTROL_INFO * ControlInfo)
{
    g_Controls = ControlInfo->Controls;
    LoadScript();
    SetupControllers();
}

EXPORT void CALL RomOpen(void)
{
    LoadScript();
    SetupControllers();
    g_Frame = 0;
    g_Reads = 0;
}

EXPORT void CALL RomClosed(void)
{
}

EXPORT void CALL GetKeys(int32_t Control, BUTTONS * Keys)
{
    if (Control == 0)
    {
        g_Frame = g_Reads++;
    }
    Keys->Value = (Control >= 0 && Control < 4) ? StateAt(Control, g_Frame) : 0;
}

EXPORT void CALL ControllerCommand(int32_t /*Control*/, uint8_t * /*Command*/)
{
}

EXPORT void CALL ReadController(int /*Control*/, uint8_t * /*Command*/)
{
}

EXPORT void CALL WM_KeyDown(uint32_t /*wParam*/, uint32_t /*lParam*/)
{
}

EXPORT void CALL WM_KeyUp(uint32_t /*wParam*/, uint32_t /*lParam*/)
{
}

EXPORT void CALL CloseDLL(void)
{
}

EXPORT void CALL DllAbout(void * /*hParent*/)
{
}

void UseUnregisteredSetting(int /*SettingID*/)
{
}
