find_package(yaml-cpp CONFIG REQUIRED)

add_library(talmon_config STATIC src/TalosConfigParser.cpp)
add_library(talmon::config ALIAS talmon_config)

target_include_directories(talmon_config PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(talmon_config
    PUBLIC talmon::domain
    PRIVATE yaml-cpp::yaml-cpp)
talmon_set_warnings(talmon_config)
