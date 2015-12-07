
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} cur_aSRCS)
set(cur_SRCS "${cur_aSRCS}")



include_directories(${CMAKE_CURRENT_SOURCE_DIR}
${PROJECT_SOURCE_DIR}
/usr/include
)

add_library(dcutil STATIC ${cur_SRCS})

link_directories(
/usr/local/lib)

target_link_libraries(dcutil
)

