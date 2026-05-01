function(set_options target)
target_include_directories(${target} PRIVATE
  ${CMAKE_SOURCE_DIR}/lib/include
)

target_compile_options(${target} PRIVATE
  $<$<CONFIG:Debug>:-Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast -g -DDEBUG>
  $<$<CONFIG:Release>:-O2>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(${target} PRIVATE -fsanitize=address -fsanitize=undefined)
  target_link_options(${target} PRIVATE -fsanitize=address -fsanitize=undefined)
endif()
endfunction()

add_executable(iterative s_iterative.cpp)
set_options(iterative)

add_executable(fork s_fork.cpp)
set_options(fork)

add_executable(thread s_thread.cpp)
set_options(thread)

add_executable(epoll s_epoll.cpp)
set_options(epoll)
