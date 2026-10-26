# Source/Common: strings, paths, files, ini files, threads, logging.

pj64_vcxproj_sources(common_sources "${PJ64_SRC}/Common/Common.vcxproj")
add_library(Common STATIC ${common_sources})
pj64_target(Common Common)
target_link_libraries(Common PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(WIN32)
    target_link_libraries(Common PUBLIC comdlg32 shell32 ole32)
endif()
