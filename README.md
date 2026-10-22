# Talmon — Talos Linux monitor for Windows

Native Windows desktop app (WinUI 3 + C++/WinRT) that monitors [Talos Linux](https://www.talos.dev/)
clusters by calling the **Talos gRPC API directly** (`machine.MachineService` on apid, port 50000,
mutual TLS with the credentials from your `talosconfig`). It does not use `talosctl`.

I've built that project to monitor my homelab, a 3 node cluster running TalosOS on the 3 cheapest MiniPCs I could find.

| Page | Talos API calls |
|---|---|
| **Panel** – one card per node: version, platform, uptime, CPU %, memory, load, services | `Version`, `Hostname`, `Memory`, `LoadAvg`, `SystemStat`, `ServiceList` |
| **Servicios** – state, health and last event of every system service | `ServiceList` |
| **Procesos** – filterable/sortable process table | `Processes` |
| **Logs** – live service log (follow, tail) | `Logs` (server streaming) |
| **Configuración** – talosconfig path, context, refresh interval, timeout | — |

## Requirements

* Windows 10 19041+ / Windows 11, x64
* Visual Studio 2026 (or 2022 17.x) with **Desktop development with C++** and **WinUI application development** (Windows App SDK C++ templates)
* vcpkg (the copy bundled with Visual Studio is used automatically when `VCPKG_ROOT` is not set)

## Build

```powershell
./build.ps1                         # Debug: vcpkg deps + core + tests + app
./build.ps1 -Configuration Release -Run
```

Step by step:

```powershell
cmake --preset windows-x64          # installs grpc, protobuf, yaml-cpp, gtest via vcpkg manifest
cmake --build --preset debug        # core libraries, talmon_backend.dll, talmon_tests.exe
ctest --preset debug                # GoogleTest suite
msbuild src/app/Talmon.vcxproj -restore -p:Configuration=Debug -p:Platform=x64
```

The first configure builds gRPC and its dependencies from source (≈30 min); later builds reuse the
vcpkg binary cache. The app is unpackaged and self-contained (Windows App SDK runtime copied next to
`build/app/x64/<Config>/Talmon.exe`).

At startup Talmon reads `%TALOSCONFIG%` or `%USERPROFILE%\.talos\config` (the same file `talosctl`
uses) and connects to the current context. Preferences are stored in `%LOCALAPPDATA%\Talmon\settings.ini`.

## Architecture

Clean/hexagonal layering; dependencies only point inwards.

```
                    ┌────────────────────────────── src/app (WinUI 3, MSBuild) ──────────────────────────────┐
                    │ MainWindow (shell) · Pages implement IMonitorView · item view models · AppServices      │
                    │ (composition root)                                                                     │
                    └───────────────┬───────────────────────────────────────────────┬────────────────────────┘
                                    │ uses                                           │ creates via
                                    ▼                                                ▼
 src/application  ClusterMonitor · MonitorSession ·                 src/infrastructure/backend  talmon_backend.dll
 (static lib)     LogStreamSession · ProcessQuery ·                   createTalosClientFactory()
                  ServiceHealth · Formatting · Settings               createTalosConfigSource()
                                    │                                   │            │
                                    ▼                                   ▼            ▼
 src/domain       Models · Error/Result (std::expected) ◄──── grpc/ GrpcTalosClient,   config/ YamlTalosConfigSource
 (header only)    Ports: IMachineInfoApi, ISystemMetricsApi,       ProtoMapper (ACL)      (yaml-cpp, base64)
                  IServiceApi, IProcessApi, ILogApi,             settings/ IniSettingsStore
                  ITalosClient, ITalosClientFactory,
                  ITalosConfigSource, IClock
```

How SOLID shows up in the code:

* **S** – each class has one reason to change: `ProtoMapper` only translates protobuf → domain,
  `GrpcTalosClient` only does transport, `ClusterMonitor` only aggregates node health, pages only render.
* **O** – new data sources or views are added by implementing a port (`ITalosClient`, `ISettingsStore`)
  or `IMonitorView`; the shell and use cases don't change.
* **L** – every port implementation (gRPC client, gmock doubles, in-process fake server) is
  interchangeable; the tests run the same use cases against fakes.
* **I** – the Talos API is split into small interfaces (`IServiceApi`, `ILogApi`, …);
  `LogStreamSession` depends on `ILogApi` only.
* **D** – the application layer depends on abstractions in `domain`; concrete gRPC/YAML/OpenSSL code
  lives behind `talmon_backend.dll`, so no third-party header reaches the UI project.

Other details:

* Talos API protos are vendored from Talos **v1.14.2** in `proto/` and compiled with `protoc` +
  `grpc_cpp_plugin` at build time.
* Talos writes Ed25519 client keys under a non-standard `ED25519 PRIVATE KEY` PEM label; it is relabelled
  to PKCS#8 `PRIVATE KEY` before handing it to gRPC/OpenSSL.
* Requests are routed to a node with the `node` gRPC metadata header (apid proxies them).
* Blocking gRPC calls run on the thread pool (`RunInBackground`) and results are marshalled back to
  the UI thread; a connection *generation* counter discards stale results after switching context.
* Log streams run on a `std::jthread`; stopping requests `TryCancel()` on the gRPC context through a
  `std::stop_callback`.

## Tests

`tests/` (GoogleTest + GoogleMock, 87 tests):

* application use cases with gmock doubles (`ClusterMonitor`, `MonitorSession`, `LogStreamSession`, …)
* pure logic (CPU usage, formatting, process filtering, service classification, log line assembly)
* talosconfig parsing, settings persistence
* `GrpcTalosClient` end to end against an **in-process fake `MachineService` gRPC server**
  (node routing metadata, status mapping, deadlines, streaming and cancellation)

## Layout

```
proto/                  Talos API .proto files (common, machine, google/rpc/status)
src/domain/             entities, Result<T>, ports
src/application/        use cases
src/infrastructure/     grpc/ config/ settings/ backend/ (DLL boundary)
src/app/                WinUI 3 application (Talmon.vcxproj)
tests/                  GoogleTest suite
build.ps1               end-to-end build
```
