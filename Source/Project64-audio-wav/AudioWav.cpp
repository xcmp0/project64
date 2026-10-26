// Project64-audio-wav: an audio plugin that plays nothing and writes what the
// game sends to the audio interface into a WAV file (16-bit stereo at the
// game's rate), for the headless frontend.
//
// It keeps no time of its own: with "Fixed Audio" (the default) the core
// paces the audio interface. Settings, in Project64.cfg:
//
//   [AudioWav]
//   WavFile=<path of the WAV file; empty writes nothing>
//
// The file is opened when a ROM opens and completed when it closes, or as
// soon as WavFile is cleared while the game runs, so the frontend can end the
// recording at an exact point. (Setting names are unique across plugins:
// frontends find them by name.)

#include <Project64-plugin-spec/Audio.h>
#include <Settings/Settings.h>
#include <stdio.h>
#include <string.h>

namespace
{
enum
{
    Set_File = 1,
};

// Clock of the audio DAC by video system: NTSC, PAL, MPAL
const uint32_t DacClock[3] = {48681812, 49656530, 48628316};

// The core maps RDRAM with room to spare; a DMA never reads past 8 MB
const uint32_t RdramLimit = 0x800000;

AUDIO_INFO g_AudioInfo;
FILE * g_File = nullptr;
uint32_t g_Frequency = 0;
uint32_t g_DataBytes = 0;
uint8_t g_Samples[0x40000];

void PutU16(uint8_t * Dest, uint32_t Value)
{
    Dest[0] = (uint8_t)Value;
    Dest[1] = (uint8_t)(Value >> 8);
}

void PutU32(uint8_t * Dest, uint32_t Value)
{
    PutU16(Dest, Value);
    PutU16(Dest + 2, Value >> 16);
}

// RIFF header for 16-bit stereo PCM
void WriteHeader(void)
{
    uint8_t Header[44];
    memcpy(Header, "RIFF", 4);
    PutU32(Header + 4, 36 + g_DataBytes);
    memcpy(Header + 8, "WAVEfmt ", 8);
    PutU32(Header + 16, 16);
    PutU16(Header + 20, 1);
    PutU16(Header + 22, 2);
    PutU32(Header + 24, g_Frequency);
    PutU32(Header + 28, g_Frequency * 4);
    PutU16(Header + 32, 4);
    PutU16(Header + 34, 16);
    memcpy(Header + 36, "data", 4);
    PutU32(Header + 40, g_DataBytes);
    fseek(g_File, 0, SEEK_SET);
    fwrite(Header, sizeof(Header), 1, g_File);
    fseek(g_File, 0, SEEK_END);
}

void CloseFile(void)
{
    if (g_File != nullptr)
    {
        WriteHeader();
        fclose(g_File);
        g_File = nullptr;
    }
}

void OpenFile(void)
{
    CloseFile();
    g_DataBytes = 0;
    char Path[1024];
    GetSettingSz(Set_File, Path, sizeof(Path));
    if (Path[0] != '\0')
    {
        g_File = fopen(Path, "wb");
        if (g_File != nullptr)
        {
            WriteHeader();
        }
    }
}

void FileChanged(void *)
{
    char Path[1024];
    GetSettingSz(Set_File, Path, sizeof(Path));
    if (Path[0] == '\0')
    {
        CloseFile();
    }
}
} // namespace

EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = AUDIO_SPECS_VERSION;
    PluginInfo->Type = PLUGIN_TYPE_AUDIO;
    snprintf(PluginInfo->Name, sizeof(PluginInfo->Name), "Project64 WAV writer");
    // Byte-swapped RDRAM (normal memory: no, swapped: yes)
    PluginInfo->Reserved1 = false;
    PluginInfo->Reserved2 = true;
}

EXPORT void CALL PluginLoaded(void)
{
    SetModuleName("AudioWav");
    RegisterSetting(Set_File, Data_String_General, "WavFile", "", 0, "");
    SettingsRegisterChange(false, Set_File, nullptr, FileChanged);
}

EXPORT int32_t CALL InitiateAudio(AUDIO_INFO Audio_Info)
{
    g_AudioInfo = Audio_Info;
    return true;
}

EXPORT void CALL RomOpen(void)
{
    OpenFile();
}

EXPORT void CALL RomClosed(void)
{
    CloseFile();
}

EXPORT void CALL CloseDLL(void)
{
    CloseFile();
}

EXPORT void CALL AiDacrateChanged(int32_t SystemType)
{
    uint32_t Clock = DacClock[SystemType >= 0 && SystemType <= 2 ? SystemType : 0];
    uint32_t Frequency = Clock / (*g_AudioInfo.AI_DACRATE_REG + 1);
    // The header holds one rate: the one in force when the first samples come
    if (g_DataBytes == 0)
    {
        g_Frequency = Frequency;
    }
}

EXPORT void CALL AiLenChanged(void)
{
    uint32_t Length = *g_AudioInfo.AI_LEN_REG & 0x3FFF8;
    uint32_t Address = *g_AudioInfo.AI_DRAM_ADDR_REG & 0x00FFFFF8;
    if (g_File == nullptr || Length == 0 || Address + Length > RdramLimit)
    {
        return;
    }
    // RDRAM is byte-swapped by 32-bit word: a frame (left, right; big-endian
    // 16-bit) reads as right, left in little-endian order
    const uint8_t * Source = g_AudioInfo.RDRAM + Address;
    for (uint32_t i = 0; i < Length; i += 4)
    {
        g_Samples[i] = Source[i + 2];
        g_Samples[i + 1] = Source[i + 3];
        g_Samples[i + 2] = Source[i];
        g_Samples[i + 3] = Source[i + 1];
    }
    fwrite(g_Samples, Length, 1, g_File);
    g_DataBytes += Length;
}

EXPORT uint32_t CALL AiReadLength(void)
{
    return 0;
}

EXPORT void CALL AiUpdate(int32_t /*Wait*/)
{
}

// High-level audio is left to the RSP plugin ("Audio HLE" off)
EXPORT void CALL ProcessAList(void)
{
}

EXPORT void CALL DllAbout(void * /*hParent*/)
{
}

void UseUnregisteredSetting(int /*SettingID*/)
{
}
