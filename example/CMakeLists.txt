add_executable(simple_compress simple_compress.cpp)
target_link_libraries(simple_compress PRIVATE ALP)

add_executable(simple_compress_varlen simple_compress_varlen.cpp)
target_link_libraries(simple_compress_varlen PRIVATE ALP)

add_executable(simple_compress_varlen_c simple_compress_varlen.c)
target_link_libraries(simple_compress_varlen_c PRIVATE ALP)

add_executable(simple_compress32 simple_compress32.cpp)
target_link_libraries(simple_compress32 PRIVATE ALP)

add_executable(rd_compress rd_compress.cpp)
target_link_libraries(rd_compress PRIVATE ALP)

add_executable(adaptive_compress adaptive_compress.cpp)
target_link_libraries(adaptive_compress PRIVATE ALP)

add_executable(rd_compress32 rd_compress32.cpp)
target_link_libraries(rd_compress32 PRIVATE ALP)
