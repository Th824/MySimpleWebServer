aux_source_directory(. SRCS)

add_executable(http_server ${SRCS})

target_link_libraries(http_server base net pthread)