find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# --- Generated Talos API (protobuf messages + gRPC stubs) --------------------------------------
set(TALMON_PROTO_DIR "${PROJECT_SOURCE_DIR}/proto")
set(TALMON_PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${TALMON_PROTO_GEN_DIR}")

add_library(talmon_talos_proto STATIC
    "${TALMON_PROTO_DIR}/google/rpc/status.proto"
    "${TALMON_PROTO_DIR}/common/common.proto"
    "${TALMON_PROTO_DIR}/machine/machine.proto"
)
target_include_directories(talmon_talos_proto PUBLIC "$<BUILD_INTERFACE:${TALMON_PROTO_GEN_DIR}>")
target_link_libraries(talmon_talos_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)
# Generated code is not ours: keep it quiet.
target_compile_options(talmon_talos_proto PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W0>)
# Talos marks several API fields/messages as deprecated and gRPC deprecates some of its own
# types; including those headers must not flood consumers with C4996.
target_compile_options(talmon_talos_proto PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/wd4996>)

protobuf_generate(
    TARGET talmon_talos_proto
    LANGUAGE cpp
    IMPORT_DIRS "${TALMON_PROTO_DIR}"
    PROTOC_OUT_DIR "${TALMON_PROTO_GEN_DIR}")

protobuf_generate(
    TARGET talmon_talos_proto
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    IMPORT_DIRS "${TALMON_PROTO_DIR}"
    PROTOC_OUT_DIR "${TALMON_PROTO_GEN_DIR}")

# --- Adapter: ITalosClient over gRPC ------------------------------------------------------------
add_library(talmon_grpc STATIC
    src/GrpcTalosClient.cpp
    src/ProtoMapper.cpp
)
add_library(talmon::grpc ALIAS talmon_grpc)

target_include_directories(talmon_grpc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(talmon_grpc PUBLIC talmon::domain talmon_talos_proto)
talmon_set_warnings(talmon_grpc)
