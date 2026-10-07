find_package(GTest CONFIG REQUIRED)

add_executable(talmon_tests
    application/ClusterMonitorTests.cpp
    application/CpuUsageTests.cpp
    application/FormattingTests.cpp
    application/LogBufferTests.cpp
    application/LogStreamSessionTests.cpp
    application/MonitorSessionTests.cpp
    application/ProcessQueryTests.cpp
    application/ServiceHealthTests.cpp
    application/SettingsTests.cpp
    infrastructure/GrpcTalosClientTests.cpp
    infrastructure/IniSettingsStoreTests.cpp
    infrastructure/ProtoMapperTests.cpp
    infrastructure/TalosConfigParserTests.cpp
)

target_include_directories(talmon_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(talmon_tests PRIVATE
    talmon::application
    talmon::settings
    talmon::config
    talmon::grpc
    GTest::gtest
    GTest::gmock
    GTest::gmock_main)
talmon_set_warnings(talmon_tests)

include(GoogleTest)
gtest_discover_tests(talmon_tests DISCOVERY_MODE PRE_TEST)
