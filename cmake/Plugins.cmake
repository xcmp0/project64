# Plugins. Source/Settings is the plugins' side of the settings API.

pj64_vcxproj_sources(settings_sources "${PJ64_SRC}/Settings/Settings.vcxproj")
add_library(Settings STATIC ${settings_sources})
pj64_target(Settings Settings)
target_link_libraries(Settings PUBLIC Common)

if(NOT WIN32)
    # Plugins for the headless frontend (Source/Project64-headless), in the
    # places the Linux default settings name.
    # RSP: the portable plugin of the Android port, on Project64-rsp-core
    add_library(Project64-rsp-so SHARED "${PJ64_SRC}/Android/PluginRSP/main.cpp")
    pj64_target(Project64-rsp-so Android/PluginRSP)
    target_link_libraries(Project64-rsp-so PRIVATE Project64-rsp-core Settings)
    pj64_plugin(Project64-rsp-so Project64-RSP RSP)

    # Audio: the game's audio into a WAV file
    add_library(Project64-audio-wav SHARED "${PJ64_SRC}/Project64-audio-wav/AudioWav.cpp")
    pj64_target(Project64-audio-wav Project64-audio-wav)
    target_link_libraries(Project64-audio-wav PRIVATE Settings)
    pj64_plugin(Project64-audio-wav Project64-Audio-Wav Audio)

    # Input: controller states from a script
    add_library(Project64-input-script SHARED "${PJ64_SRC}/Project64-input-script/InputScript.cpp")
    pj64_target(Project64-input-script Project64-input-script)
    target_link_libraries(Project64-input-script PRIVATE Settings)
    pj64_plugin(Project64-input-script Project64-Input-Script Input)

    # Video: no rendering; the frame the VI shows from RDRAM
    add_library(Project64-video-null SHARED "${PJ64_SRC}/Project64-video-null/VideoNull.cpp")
    pj64_target(Project64-video-null Project64-video-null)
    pj64_plugin(Project64-video-null Project64-Video-Null GFX)
endif()

if(WIN32)
    # RSP: the interpreter, recompilers and HLE of Project64-rsp-core, with a debugger
    pj64_vcxproj_sources(rsp_sources "${PJ64_SRC}/Project64-rsp/Project64-rsp.vcxproj")
    pj64_rc(rsp_rc "${PJ64_SRC}/Project64-rsp/Project64-rsp.rc")
    add_library(Project64-rsp SHARED ${rsp_sources} "${rsp_rc}")
    pj64_target(Project64-rsp Project64-rsp)
    target_link_libraries(Project64-rsp PRIVATE Project64-rsp-core Settings comctl32 gdi32)
    pj64_plugin(Project64-rsp Project64-RSP RSP)

    # Input: N-Rage (DirectInput, XInput)
    pj64_vcxproj_sources(nrage_sources "${PJ64_SRC}/nragev20/NRage_Input_V2.vcxproj")
    pj64_rc(nrage_rc "${PJ64_SRC}/nragev20/NRagePluginV2.rc")
    add_library(PJ64_NRage SHARED ${nrage_sources} "${nrage_rc}")
    pj64_target(PJ64_NRage nragev20)
    pj64_version_header(PJ64_NRage nragev20)
    target_link_libraries(PJ64_NRage PRIVATE Common dinput8 dxguid xinput comctl32 comdlg32 gdi32 ole32 oleaut32 shell32 uuid winmm)
    pj64_plugin(PJ64_NRage PJ64_NRage Input)
endif()

if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Audio: DirectSound
    pj64_vcxproj_sources(audio_sources "${PJ64_SRC}/Project64-audio/Project64-audio.vcxproj")
    pj64_rc(audio_rc "${PJ64_SRC}/Project64-audio/Project64-audio.rc")
    add_library(Project64-audio SHARED ${audio_sources} "${audio_rc}")
    pj64_target(Project64-audio Project64-audio)
    pj64_wtl(Project64-audio)
    pj64_version_header(Project64-audio Project64-audio)
    target_link_libraries(Project64-audio PRIVATE Settings dsound dxguid comctl32 uxtheme ole32 winmm)
    pj64_plugin(Project64-audio Project64-Audio Audio)

    # Input: Project64-input, on SDL2 from the external/sdl submodule as in the
    # Visual Studio build (PJ64_SDL_DIR points elsewhere)
    set(PJ64_SDL_DIR "${PJ64_ROOT}/external/sdl" CACHE PATH "SDL2 sources for Project64-input")
    if(EXISTS "${PJ64_SDL_DIR}/CMakeLists.txt")
        set(SDL_SHARED OFF CACHE BOOL "Build a shared version of the library")
        set(SDL_STATIC ON CACHE BOOL "Build a static version of the library")
        set(SDL_TEST OFF CACHE BOOL "Build the SDL2_test library")
        # Off: SDL 2.28 asks for CMake 3.4 when it is on, which CMake 4 refuses
        set(SDL_CCACHE OFF CACHE BOOL "Use Ccache to speed up build")
        add_subdirectory("${PJ64_SDL_DIR}" "${PROJECT_BINARY_DIR}/external/sdl" EXCLUDE_FROM_ALL)
        pj64_vcxproj_sources(input_sources "${PJ64_SRC}/Project64-input/Project64-input.vcxproj")
        pj64_rc(input_rc "${PJ64_SRC}/Project64-input/Project64-input.rc")
        add_library(Project64-input SHARED ${input_sources} "${input_rc}")
        pj64_target(Project64-input Project64-input)
        pj64_wtl(Project64-input)
        pj64_version_header(Project64-input Project64-input)
        target_compile_definitions(Project64-input PRIVATE SDL_STATIC_LIB)
        target_link_libraries(Project64-input PRIVATE Settings SDL2::SDL2-static
            comctl32 gdi32 ole32 oleaut32 setupapi imm32 version winmm)
        pj64_plugin(Project64-input Project64-Input Input)
    else()
        message(STATUS "Project64-input needs SDL2: git submodule update --init external/sdl")
    endif()

    # Video: Project64-video (Glide64), OpenGL
    pj64_vcxproj_sources(video_sources "${PJ64_SRC}/Project64-video/Project64-video.vcxproj")
    pj64_rc(video_rc "${PJ64_SRC}/Project64-video/Project64-video.rc")
    add_library(Project64-video SHARED ${video_sources} "${video_rc}")
    pj64_target(Project64-video Project64-video)
    pj64_wtl(Project64-video)
    pj64_version_header(Project64-video Project64-video)
    target_include_directories(Project64-video PRIVATE
        "${PJ64_SRC}/Project64-video/Renderer/inc" "${PJ64_SRC}/Project64-video/inc")
    target_link_libraries(Project64-video PRIVATE Settings pj64_png opengl32 gdi32 comctl32 uxtheme ole32)
    pj64_plugin(Project64-video Project64-Video GFX)
elseif(WIN32)
    message(STATUS "The audio, input and video plugins need Clang (WTL); building RSP and N-Rage only")
endif()
