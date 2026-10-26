// Project64-headless: runs a ROM with no window, for scripts and tests.
//
//   Project64-headless [options] <rom>
//
// It runs a number of frames (vertical interrupts), can dump frames as PNG
// files, has the WAV audio plugin record the audio and the script input
// plugin play a controller script, and reports through its exit code. The
// options become settings in the base directory's Project64.cfg, as the
// UI's choices do. The run is deterministic for one build and CPU core: the
// timing jitter has a fixed seed.

#include <Common/DynamicLibrary.h>
#include <Common/StdString.h>
#include <Common/SyncEvent.h>
#include <Common/Thread.h>
#include <Common/path.h>
#include <Project64-core/AppInit.h>
#include <Project64-core/Debugger.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Notification.h>
#include <Project64-core/Plugins/Plugin.h>
#include <Project64-core/Settings.h>
#include <png/png.h>

#if defined(__x86_64__) && defined(__GLIBC__)
#include <malloc.h>
#endif

#include <atomic>
#include <chrono>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

namespace
{
enum ExitCode
{
    Exit_Done = 0,
    Exit_Error = 1,
    Exit_Usage = 2,
    Exit_Timeout = 3,
    Exit_Stopped = 4,
};

const char Usage[] =
    "usage: Project64-headless [options] <rom>\n"
    "  --frames <n>         vertical interrupts to run (default 600)\n"
    "  --cpu <core>         interpreter, recompiler or sync (both, compared);\n"
    "                       default: the game's setting\n"
    "  --seed <n>           seed of the DMA timing jitter (default 0)\n"
    "  --rsp-gfx <mode>     hle: display lists go to the graphics plugin; lle: the\n"
    "                       RSP runs the microcode, the plugin gets RDP lists;\n"
    "                       default: the game's setting\n"
    "  --gfx|--rsp|--audio|--input <file>\n"
    "                       plugin, relative to the plugin directory\n"
    "  --wav <file>         record the audio (WAV audio plugin)\n"
    "  --script <file>      controller script (script input plugin)\n"
    "  --dump <dir>         write frames to <dir>/frame-<n>.png; by default the last\n"
    "  --dump-at <n,...>    frames to write\n"
    "  --dump-every <k>     write every k-th frame\n"
    "  --timeout <s>        stop after s seconds of wall time (default: none)\n"
    "  --keep-going         run on after the core reports an error (default: stop)\n"
    "  --basedir <dir>      settings and data directory (default: the program's)\n"
    "  --save-dir <dir>     game saves (default: the base directory's Save/);\n"
    "                       an empty one starts the game afresh\n"
    "  --verbose            print the core's messages\n"
    "exit code: 0 done, 1 an error was reported, 2 bad arguments or ROM,\n"
    "           3 timeout, 4 emulation stopped early\n";

struct Options
{
    std::string Rom;
    std::string BaseDir;
    std::string Cpu;
    std::string RspGfx;
    std::string Gfx, Rsp, Audio, Input;
    std::string Wav, Script;
    std::string DumpDir;
    std::string SaveDir;
    std::set<uint32_t> DumpAt;
    uint32_t DumpEvery = 0;
    uint32_t Frames = 600;
    uint32_t Seed = 0;
    uint32_t Timeout = 0;
    bool Verbose = false;
    bool KeepGoing = false;
};

Options g_Options;
std::atomic<uint32_t> g_Frame(0);
std::atomic<bool> g_Error(false);
SyncEvent g_Done;
uint32_t g_MainThread = 0;

// An error from the emulation: the first one ends the run unless asked
// otherwise, so its traces (such as the sync CPU's log) stay as they were
void ErrorReported(void)
{
    g_Error = true;
    if (!g_Options.KeepGoing && g_BaseSystem != nullptr && CThread::GetCurrentThreadId() != g_MainThread &&
        g_Settings->LoadBool(GameRunning_CPU_Running))
    {
        g_BaseSystem->CloseCpu();
        g_Done.Trigger();
    }
}
uint32_t g_WavSetting = 0;

typedef void(CALL * ReadScreenFunc)(void ** Dest, long * Width, long * Height);
typedef void(CALL * DllCrtFreeFunc)(void * Address);
DynLibHandle g_GfxHandle = nullptr;
ReadScreenFunc g_ReadScreen = nullptr;
DllCrtFreeFunc g_DllCrtFree = nullptr;

class Notification :
    public CNotification
{
public:
    void DisplayError(const char * Message) const override
    {
        fprintf(stderr, "error: %s\n", Message);
        ErrorReported();
    }
    void DisplayError(LanguageStringID StringID) const override
    {
        DisplayError(g_Lang != nullptr ? g_Lang->GetString(StringID).c_str() : stdstr_f("message %d", StringID).c_str());
    }
    void FatalError(const char * Message) const override
    {
        fprintf(stderr, "fatal: %s\n", Message);
        ErrorReported();
    }
    void FatalError(LanguageStringID StringID) const override
    {
        FatalError(g_Lang != nullptr ? g_Lang->GetString(StringID).c_str() : stdstr_f("message %d", StringID).c_str());
    }
    void DisplayWarning(const char * Message) const override
    {
        fprintf(stderr, "warning: %s\n", Message);
    }
    void DisplayWarning(LanguageStringID StringID) const override
    {
        DisplayWarning(g_Lang != nullptr ? g_Lang->GetString(StringID).c_str() : stdstr_f("message %d", StringID).c_str());
    }
    void DisplayMessage(int /*DisplayTime*/, const char * Message) const override
    {
        if (g_Options.Verbose && Message[0] != '\0')
        {
            fprintf(stderr, "%s\n", Message);
        }
    }
    void DisplayMessage(int DisplayTime, LanguageStringID StringID) const override
    {
        if (g_Lang != nullptr)
        {
            DisplayMessage(DisplayTime, g_Lang->GetString(StringID).c_str());
        }
    }
    void DisplayMessage2(const char * /*Message*/) const override
    {
    }
    bool AskYesNoQuestion(const char * Question) const override
    {
        fprintf(stderr, "question (answered no): %s\n", Question);
        return false;
    }
    void BreakPoint(const char * FileName, int32_t LineNumber) override
    {
        fprintf(stderr, "error: core breakpoint at %s:%d\n", FileName, LineNumber);
        ErrorReported();
    }
    void AppInitDone(void) override
    {
    }
    bool ProcessGuiMessages(void) const override
    {
        return false;
    }
    void ChangeFullScreen(void) const override
    {
    }
};

// No window: the graphics plugin owns its own (offscreen) context
class Window :
    public RenderWindow
{
public:
    void GfxThreadInit() override
    {
    }
    void GfxThreadDone() override
    {
    }
    void SwapWindow() override
    {
    }
};

// Needed by the sync CPU, which runs with the debugger on; nothing to show
class Debugger :
    public CDebugger
{
public:
    void OpenCommandWindow(void) override {}
    void OpenMemoryWindow(void) override {}
    void OpenMemoryDump(void) override {}
    void OpenMemorySearch(void) override {}
    void OpenTLBWindow(void) override {}
    void OpenScriptsWindow(void) override {}
    void OpenSymbolsWindow(void) override {}
    void OpenDMALogWindow(void) override {}
    void OpenCPULogWindow(void) override {}
    void OpenExcBreakpointsWindow(void) override {}
    void OpenStackTraceWindow(void) override {}
    void OpenStackViewWindow(void) override {}
    void TLBChanged(void) override {}
    void FrameDrawn(void) override {}
    void WaitForStep(void) override
    {
        // Reached on an unknown opcode: no one can step, so stop
        fprintf(stderr, "error: the CPU stopped for a debugger step (unknown opcode?)\n");
        g_Error = true;
        g_Settings->SaveBool(Debugger_SteppingOps, false);
        g_Done.Trigger();
    }
    bool ExecutionBP(uint32_t) override { return false; }
    bool ReadBP8(uint32_t) override { return false; }
    bool ReadBP16(uint32_t) override { return false; }
    bool ReadBP32(uint32_t) override { return false; }
    bool ReadBP64(uint32_t) override { return false; }
    bool WriteBP8(uint32_t) override { return false; }
    bool WriteBP16(uint32_t) override { return false; }
    bool WriteBP32(uint32_t) override { return false; }
    bool WriteBP64(uint32_t) override { return false; }
    void CPUStepStarted(void) override {}
    void CPUStep(void) override {}
    void CPUStepEnded(void) override {}
    void PIFReadStarted(void) override {}
    void RSPReceivedTask(void) override {}
    void PIDMAReadStarted(void) override {}
    void PIDMAWriteStarted(void) override {}
    void EmulationStarted(void) override {}
    void EmulationStopped(void) override {}
};

Notification g_Notification;
Window g_Window, g_SyncWindow;
Debugger g_HeadlessDebugger;

bool ParseNumber(const char * Text, uint32_t & Value)
{
    char * End;
    unsigned long Number = strtoul(Text, &End, 10);
    if (*Text == '\0' || *End != '\0')
    {
        return false;
    }
    Value = (uint32_t)Number;
    return true;
}

bool ParseOptions(int argc, char ** argv)
{
    for (int i = 1; i < argc; i++)
    {
        std::string Option = argv[i];
        if (Option == "--verbose")
        {
            g_Options.Verbose = true;
            continue;
        }
        if (Option == "--keep-going")
        {
            g_Options.KeepGoing = true;
            continue;
        }
        if (Option.compare(0, 2, "--") != 0)
        {
            if (!g_Options.Rom.empty())
            {
                return false;
            }
            g_Options.Rom = Option;
            continue;
        }
        if (i + 1 >= argc)
        {
            return false;
        }
        const char * Value = argv[++i];
        bool ok = true;
        if (Option == "--frames")
        {
            ok = ParseNumber(Value, g_Options.Frames) && g_Options.Frames > 0;
        }
        else if (Option == "--cpu")
        {
            g_Options.Cpu = Value;
            ok = g_Options.Cpu == "interpreter" || g_Options.Cpu == "recompiler" || g_Options.Cpu == "sync";
        }
        else if (Option == "--seed")
        {
            ok = ParseNumber(Value, g_Options.Seed);
        }
        else if (Option == "--rsp-gfx")
        {
            g_Options.RspGfx = Value;
            ok = g_Options.RspGfx == "hle" || g_Options.RspGfx == "lle";
        }
        else if (Option == "--gfx")
        {
            g_Options.Gfx = Value;
        }
        else if (Option == "--rsp")
        {
            g_Options.Rsp = Value;
        }
        else if (Option == "--audio")
        {
            g_Options.Audio = Value;
        }
        else if (Option == "--input")
        {
            g_Options.Input = Value;
        }
        else if (Option == "--wav")
        {
            g_Options.Wav = Value;
        }
        else if (Option == "--script")
        {
            g_Options.Script = Value;
        }
        else if (Option == "--dump")
        {
            g_Options.DumpDir = Value;
        }
        else if (Option == "--dump-at")
        {
            std::string List = Value;
            for (size_t Start = 0; ok && Start <= List.size();)
            {
                size_t End = List.find(',', Start);
                if (End == std::string::npos)
                {
                    End = List.size();
                }
                uint32_t Frame;
                ok = ParseNumber(List.substr(Start, End - Start).c_str(), Frame);
                g_Options.DumpAt.insert(Frame);
                Start = End + 1;
            }
        }
        else if (Option == "--dump-every")
        {
            ok = ParseNumber(Value, g_Options.DumpEvery) && g_Options.DumpEvery > 0;
        }
        else if (Option == "--timeout")
        {
            ok = ParseNumber(Value, g_Options.Timeout);
        }
        else if (Option == "--basedir")
        {
            g_Options.BaseDir = Value;
        }
        else if (Option == "--save-dir")
        {
            g_Options.SaveDir = Value;
        }
        else
        {
            ok = false;
        }
        if (!ok)
        {
            fprintf(stderr, "bad option: %s %s\n", Option.c_str(), Value);
            return false;
        }
    }
    if (!g_Options.DumpDir.empty() && g_Options.DumpAt.empty() && g_Options.DumpEvery == 0)
    {
        g_Options.DumpAt.insert(g_Options.Frames);
    }
    return !g_Options.Rom.empty();
}

// Absolute path, so plugins and the core agree whatever the working directory
std::string FullPath(const std::string & File)
{
    if (File.empty())
    {
        return File;
    }
    CPath Path(File.c_str());
    Path.NormalizePath(CPath(CPath::CURRENT_DIRECTORY));
    return (const char *)Path;
}

bool WritePng(const char * File, const uint8_t * Bgr, long Width, long Height)
{
    FILE * Output = fopen(File, "wb");
    if (Output == nullptr)
    {
        return false;
    }
    png_structp Png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop Info = Png != nullptr ? png_create_info_struct(Png) : nullptr;
    std::string Row((size_t)Width * 3, '\0');
    bool ok = Info != nullptr && setjmp(png_jmpbuf(Png)) == 0;
    if (ok)
    {
        png_init_io(Png, Output);
        png_set_IHDR(Png, Info, (png_uint_32)Width, (png_uint_32)Height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(Png, Info);
        // Bottom row first in BGR: top row first in RGB
        for (long y = Height - 1; y >= 0; y--)
        {
            const uint8_t * Source = Bgr + (size_t)y * Width * 3;
            for (long x = 0; x < Width; x++)
            {
                Row[x * 3] = (char)Source[x * 3 + 2];
                Row[x * 3 + 1] = (char)Source[x * 3 + 1];
                Row[x * 3 + 2] = (char)Source[x * 3];
            }
            png_write_row(Png, (png_bytep)&Row[0]);
        }
        png_write_end(Png, Info);
    }
    png_destroy_write_struct(&Png, Info != nullptr ? &Info : nullptr);
    fclose(Output);
    return ok;
}

void DumpFrame(uint32_t Frame)
{
    if (g_ReadScreen == nullptr)
    {
        return;
    }
    void * Image = nullptr;
    long Width = 0, Height = 0;
    g_ReadScreen(&Image, &Width, &Height);
    std::string File = stdstr_f("%s/frame-%05u.png", g_Options.DumpDir.c_str(), Frame);
    if (Image == nullptr || Width <= 0 || Height <= 0)
    {
        fprintf(stderr, "warning: frame %u: the graphics plugin returned no picture\n", Frame);
    }
    else if (!WritePng(File.c_str(), (const uint8_t *)Image, Width, Height))
    {
        fprintf(stderr, "error: cannot write %s\n", File.c_str());
        g_Error = true;
    }
    if (Image != nullptr)
    {
        if (g_DllCrtFree != nullptr)
        {
            g_DllCrtFree(Image);
        }
        else
        {
            free(Image);
        }
    }
}

// After each UpdateScreen, on the emulation thread
void FrameDrawn(void *)
{
    uint32_t Frame = ++g_Frame;
    if (Frame > g_Options.Frames)
    {
        return;
    }
    if (!g_Options.DumpDir.empty() &&
        (g_Options.DumpAt.count(Frame) != 0 || (g_Options.DumpEvery != 0 && Frame % g_Options.DumpEvery == 0)))
    {
        DumpFrame(Frame);
    }
    if (Frame == g_Options.Frames)
    {
        // The recording ends here; the CPU stops at its next check
        if (g_WavSetting != 0)
        {
            g_Settings->SaveString((SettingID)g_WavSetting, "");
        }
        g_BaseSystem->CloseCpu();
        g_Done.Trigger();
    }
}

// Settings a plugin registered: found by name once the plugin is loaded
void SetPluginSetting(const char * Name, const std::string & Value, uint32_t * Id = nullptr)
{
    uint32_t Setting = CSettings::FindSetting(g_Settings, Name);
    if (Setting != 0)
    {
        g_Settings->SaveString((SettingID)Setting, Value.c_str());
    }
    else if (!Value.empty())
    {
        fprintf(stderr, "warning: no plugin reads %s; the chosen plugin cannot take that option\n", Name);
    }
    if (Id != nullptr)
    {
        *Id = Setting;
    }
}

void ApplyOptions(void)
{
    // As fast as the host allows, no UI pause, controllers read once per VI
    g_Settings->SaveBool(UserInterface_BasicMode, false);
    g_Settings->SaveBool(GameRunning_LimitFPS, false);
    g_Settings->SaveBool(Setting_UpdateControllerOnRefresh, true);
    g_Settings->SaveBool(Debugger_Enabled, g_Options.Cpu == "sync");
    if (g_Options.Cpu == "sync")
    {
        g_Debugger = &g_HeadlessDebugger;
    }

    // Plugins: the core reloads them when these change
    if (!g_Options.Gfx.empty())
    {
        g_Settings->SaveString(Plugin_GFX_Current, g_Options.Gfx.c_str());
    }
    if (!g_Options.Rsp.empty())
    {
        g_Settings->SaveString(Plugin_RSP_Current, g_Options.Rsp.c_str());
    }
    if (!g_Options.Audio.empty())
    {
        g_Settings->SaveString(Plugin_AUDIO_Current, g_Options.Audio.c_str());
    }
    if (!g_Options.Input.empty())
    {
        g_Settings->SaveString(Plugin_CONT_Current, g_Options.Input.c_str());
    }
    if (!g_Options.SaveDir.empty())
    {
        std::string SaveDir = FullPath(g_Options.SaveDir);
        if (SaveDir.back() != '/')
        {
            SaveDir += '/';
        }
        g_Settings->SaveString(Directory_NativeSaveSelected, SaveDir.c_str());
        g_Settings->SaveBool(Directory_NativeSaveUseSelected, true);
    }
    else
    {
        g_Settings->SaveBool(Directory_NativeSaveUseSelected, false);
    }
    SetPluginSetting("WavFile", FullPath(g_Options.Wav), &g_WavSetting);
    SetPluginSetting("ScriptFile", FullPath(g_Options.Script));
}

// The graphics plugin's ReadScreen, for frame dumps
void FindReadScreen(void)
{
    CPath PluginDir(g_Settings->LoadStringVal(Directory_Plugin).c_str());
    PluginDir.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
    CPath File((const char *)PluginDir, g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str());
    g_GfxHandle = DynamicLibraryOpen(File, false);
    if (g_GfxHandle != nullptr)
    {
        g_ReadScreen = (ReadScreenFunc)DynamicLibraryGetProc(g_GfxHandle, "ReadScreen");
        g_DllCrtFree = (DllCrtFreeFunc)DynamicLibraryGetProc(g_GfxHandle, "DllCrtFree");
    }
    if (g_ReadScreen == nullptr)
    {
        fprintf(stderr, "warning: the graphics plugin cannot read the screen; no frame dumps\n");
    }
}

int Run(void)
{
    std::string Rom = FullPath(g_Options.Rom);
    if (!CN64System::LoadFileImage(Rom.c_str()))
    {
        fprintf(stderr, "error: cannot load %s\n", Rom.c_str());
        return Exit_Usage;
    }
    if (g_Options.Cpu == "interpreter")
    {
        g_Settings->SaveDword(Game_CpuType, CPU_Interpreter);
    }
    else if (g_Options.Cpu == "recompiler")
    {
        g_Settings->SaveDword(Game_CpuType, CPU_Recompiler);
    }
    else if (g_Options.Cpu == "sync")
    {
        g_Settings->SaveDword(Game_CpuType, CPU_SyncCores);
    }
    if (!g_Options.RspGfx.empty())
    {
        g_Settings->SaveBool(Game_UseHleGfx, g_Options.RspGfx == "hle");
    }
    if (!g_Options.DumpDir.empty())
    {
        CPath DumpDir(FullPath(g_Options.DumpDir).c_str(), "");
        if (!DumpDir.DirectoryExists())
        {
            DumpDir.DirectoryCreate();
        }
        FindReadScreen();
    }

    g_BaseSystem = new CN64System(g_Plugins, g_Options.Seed, false, false);
    g_BaseSystem->RegisterCallBack(CN64SystemCB_FrameDrawn, nullptr, FrameDrawn);
    auto Start = std::chrono::steady_clock::now();
    g_BaseSystem->StartEmulation(true);

    int Result = Exit_Done;
    bool Started = false;
    for (;;)
    {
        if (g_Done.IsTriggered(100))
        {
            break;
        }
        double Elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
        bool Running = g_Settings->LoadBool(GameRunning_CPU_Running);
        Started = Started || Running;
        if ((Started && !Running) || (!Started && Elapsed > 30))
        {
            Result = Exit_Stopped;
            break;
        }
        if (g_Options.Timeout != 0 && Elapsed > g_Options.Timeout)
        {
            Result = Exit_Timeout;
            break;
        }
    }
    double Elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
    CN64System::CloseSystem();
    if (!g_Options.Cpu.empty())
    {
        g_Settings->DeleteSetting(Game_CpuType);
    }
    if (!g_Options.RspGfx.empty())
    {
        g_Settings->DeleteSetting(Game_UseHleGfx);
    }

    uint32_t Frames = g_Frame < g_Options.Frames ? (uint32_t)g_Frame : g_Options.Frames;
    fprintf(stderr, "%u frames in %.2f s (%.1f VI/s)%s\n", Frames, Elapsed, Elapsed > 0 ? Frames / Elapsed : 0.0,
            Result == Exit_Timeout ? ", timeout" : Result == Exit_Stopped ? ", stopped early" : "");
    if (Result == Exit_Done && g_Error)
    {
        Result = Exit_Error;
    }
    return Result;
}
} // namespace

int main(int argc, char ** argv)
{
#if defined(__x86_64__) && defined(__GLIBC__)
    // The x64 recompiler reaches the emulator's objects with 32-bit
    // displacements: one heap, in the brk area just above the program, which
    // is linked at a low address (no PIE)
    mallopt(M_MMAP_THRESHOLD, 32 * 1024 * 1024);
    mallopt(M_ARENA_MAX, 1);
#endif
    if (!ParseOptions(argc, argv))
    {
        fputs(Usage, stderr);
        return Exit_Usage;
    }
    g_MainThread = CThread::GetCurrentThreadId();
    std::string BaseDir = g_Options.BaseDir.empty() ? std::string((const char *)CPath(CPath::MODULE_DIRECTORY)) : FullPath(g_Options.BaseDir);
    if (BaseDir.empty() || BaseDir.back() != '/')
    {
        BaseDir += '/';
    }
    if (!AppInit(&g_Notification, BaseDir.c_str(), 1, argv))
    {
        fprintf(stderr, "error: cannot start the core in %s\n", BaseDir.c_str());
        return Exit_Error;
    }
    g_Plugins->SetRenderWindows(&g_Window, &g_SyncWindow);
    ApplyOptions();
    int Result = Run();
    if (g_GfxHandle != nullptr)
    {
        DynamicLibraryClose(g_GfxHandle);
    }
    AppCleanup();
    return Result;
}
