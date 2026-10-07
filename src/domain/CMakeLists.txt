# Domain layer: entities, value objects and ports. Header only, no third-party dependencies.
add_library(talmon_domain INTERFACE)
add_library(talmon::domain ALIAS talmon_domain)

target_include_directories(talmon_domain INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(talmon_domain INTERFACE cxx_std_23)
