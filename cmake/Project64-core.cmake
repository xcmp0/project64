# Source/Project64-core: the N64 system, CPU, memory, plugins and settings.

set(core_exclude "")
if(NOT WIN32)
    # 7z ROMs are Windows-only (see ThirdParty.cmake).
    list(APPEND core_exclude "3rdParty/7zip.cpp")
endif()
pj64_vcxproj_sources(core_sources "${PJ64_SRC}/Project64-core/Project64-core.vcxproj" EXCLUDE ${core_exclude})
# Files this fork adds (not in the upstream project)
list(APPEND core_sources "${PJ64_SRC}/Project64-core/N64System/GameKeys.cpp")
add_library(Project64-core STATIC ${core_sources})
pj64_target(Project64-core Project64-core)
target_link_libraries(Project64-core PUBLIC Common Project64-rsp-core pj64_asmjit pj64_softfloat pj64_zlib)
if(WIN32)
    target_link_libraries(Project64-core PUBLIC pj64_7zip ole32 winmm)
endif()
pj64_version_header(Project64-core Project64-core)
