# Source/Project64-rsp-core: RSP interpreter, recompilers and HLE tasks.

pj64_vcxproj_sources(rsp_core_sources "${PJ64_SRC}/Project64-rsp-core/Project64-rsp-core.vcxproj")
add_library(Project64-rsp-core STATIC ${rsp_core_sources})
pj64_target(Project64-rsp-core Project64-rsp-core)
target_link_libraries(Project64-rsp-core PUBLIC Common pj64_asmjit pj64_zlib)
pj64_version_header(Project64-rsp-core Project64-rsp-core)
