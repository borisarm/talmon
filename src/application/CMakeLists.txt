# Application layer: use cases and presentation-agnostic logic. Depends only on the domain.
add_library(talmon_application STATIC
    src/ClusterMonitor.cpp
    src/Formatting.cpp
    src/LogBuffer.cpp
    src/LogStreamSession.cpp
    src/MonitorSession.cpp
    src/ProcessQuery.cpp
    src/ServiceHealth.cpp
    src/Settings.cpp
)
add_library(talmon::application ALIAS talmon_application)

target_include_directories(talmon_application PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(talmon_application PUBLIC talmon::domain Threads::Threads)
talmon_set_warnings(talmon_application)
