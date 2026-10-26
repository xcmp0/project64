# Source/Project64: the Windows user interface. WTL 10 (Source/3rdParty/WTL)
# on ReactOS's ATL (Source/3rdParty/ReactOS-ATL) with the additions WTL needs
# (Source/3rdParty/atlcompat), as GCC and Clang have no ATL of their own.

pj64_vcxproj_sources(ui_sources "${PJ64_SRC}/Project64/Project64.vcxproj")

# The executable's manifest (common controls 6), as MSVC embeds it
set(ui_manifest_rc "${PJ64_GEN}/Project64-manifest.rc")
file(WRITE "${ui_manifest_rc}" "1 24 \"${PJ64_SRC}/Project64/Project64.exe.manifest\"\n")

pj64_rc(ui_rc "${PJ64_SRC}/Project64/UserInterface/UIResources.rc")
add_executable(Project64 WIN32 ${ui_sources} "${ui_rc}" "${ui_manifest_rc}")
target_include_directories(Project64 BEFORE PRIVATE
    "${PJ64_SRC}/3rdParty/atlcompat" "${PJ64_SRC}/3rdParty/ReactOS-ATL")
pj64_target(Project64 Project64)
# No ActiveX hosting in this UI; Windows Vista APIs
target_compile_definitions(Project64 PRIVATE _ATL_NO_HOSTING _WIN32_WINNT=0x0600 WINVER=0x0600)
# UIResources.rc includes ../../Project64-core/Version.h, generated in the build tree
file(MAKE_DIRECTORY "${PJ64_GEN}/Source/Project64/UserInterface")
set_property(SOURCE "${ui_rc}" APPEND PROPERTY INCLUDE_DIRECTORIES
    "${PJ64_GEN}/Source/Project64/UserInterface")
# WTL leans on MSVC's template parsing (names from dependent bases)
target_compile_options(Project64 PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fdelayed-template-parsing -fms-extensions>)
target_link_libraries(Project64 PRIVATE
    Project64-core pj64_duktape pj64_discord pj64_png pj64_7zip
    comctl32 comdlg32 gdi32 ole32 oleaut32 uuid shell32 shlwapi uxtheme winmm ws2_32 version advapi32)
