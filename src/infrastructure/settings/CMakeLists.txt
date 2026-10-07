add_library(talmon_settings STATIC src/IniSettingsStore.cpp)
add_library(talmon::settings ALIAS talmon_settings)

target_include_directories(talmon_settings PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(talmon_settings PUBLIC talmon::application)
talmon_set_warnings(talmon_settings)
