# Plugins. Source/Settings is the plugins' side of the settings API.

pj64_vcxproj_sources(settings_sources "${PJ64_SRC}/Settings/Settings.vcxproj")
add_library(Settings STATIC ${settings_sources})
pj64_target(Settings Settings)
target_link_libraries(Settings PUBLIC Common)

if(WIN32)
    # RSP: the interpreter, recompilers and HLE of Project64-rsp-core, with a debugger
    pj64_vcxproj_sources(rsp_sources "${PJ64_SRC}/Project64-rsp/Project64-rsp.vcxproj")
    pj64_rc(rsp_rc "${PJ64_SRC}/Project64-rsp/Project64-rsp.rc")
    add_library(Project64-rsp SHARED ${rsp_sources} "${rsp_rc}")
    pj64_target(Project64-rsp Project64-rsp)
    target_link_libraries(Project64-rsp PRIVATE Project64-rsp-core Settings comctl32 gdi32)
    pj64_plugin(Project64-rsp Project64-RSP RSP)

    # Audio: DirectSound
    pj64_vcxproj_sources(audio_sources "${PJ64_SRC}/Project64-audio/Project64-audio.vcxproj")
    pj64_rc(audio_rc "${PJ64_SRC}/Project64-audio/Project64-audio.rc")
    add_library(Project64-audio SHARED ${audio_sources} "${audio_rc}")
    pj64_target(Project64-audio Project64-audio)
    pj64_wtl(Project64-audio)
    pj64_version_header(Project64-audio Project64-audio)
    target_link_libraries(Project64-audio PRIVATE Settings dsound dxguid comctl32 uxtheme ole32 winmm)
    pj64_plugin(Project64-audio Project64-Audio Audio)

    # Input: N-Rage (DirectInput, XInput)
    pj64_vcxproj_sources(nrage_sources "${PJ64_SRC}/nragev20/NRage_Input_V2.vcxproj")
    pj64_rc(nrage_rc "${PJ64_SRC}/nragev20/NRagePluginV2.rc")
    add_library(PJ64_NRage SHARED ${nrage_sources} "${nrage_rc}")
    pj64_target(PJ64_NRage nragev20)
    pj64_version_header(PJ64_NRage nragev20)
    target_link_libraries(PJ64_NRage PRIVATE Common dinput8 dxguid xinput comctl32 comdlg32 gdi32 ole32 oleaut32 shell32 uuid winmm)
    pj64_plugin(PJ64_NRage PJ64_NRage Input)

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
endif()
