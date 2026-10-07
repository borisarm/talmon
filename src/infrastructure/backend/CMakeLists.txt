add_library(talmon_backend SHARED src/Backend.cpp)
add_library(talmon::backend ALIAS talmon_backend)

target_include_directories(talmon_backend PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_definitions(talmon_backend PRIVATE TALMON_BACKEND_EXPORTS)
target_link_libraries(talmon_backend
    PUBLIC talmon::domain
    PRIVATE talmon::grpc talmon::config)
talmon_set_warnings(talmon_backend)
