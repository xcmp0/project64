# Source/Project64: the Windows user interface (WTL, see pj64_wtl).

pj64_vcxproj_sources(ui_sources "${PJ64_SRC}/Project64/Project64.vcxproj")

# The executable's manifest (common controls 6), as MSVC embeds it
set(ui_manifest_rc "${PJ64_GEN}/Project64-manifest.rc")
file(WRITE "${ui_manifest_rc}" "1 24 \"${PJ64_SRC}/Project64/Project64.exe.manifest\"\n")

pj64_rc(ui_rc "${PJ64_SRC}/Project64/UserInterface/UIResources.rc")
add_executable(Project64 WIN32 ${ui_sources} "${ui_rc}" "${ui_manifest_rc}")
pj64_target(Project64 Project64)
pj64_wtl(Project64)
target_link_libraries(Project64 PRIVATE
    Project64-core pj64_duktape pj64_discord pj64_png pj64_7zip
    comctl32 comdlg32 gdi32 ole32 oleaut32 uuid shell32 shlwapi uxtheme winmm ws2_32 version advapi32)
