# CECSNET

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build](https://img.shields.io/github/actions/workflow/status/somozadev/CECSNET/build.yaml?branch=ECSNET-0.1)](https://github.com/somozadev/CECSNET/actions/workflows/build.yml)
[![Docs](https://img.shields.io/badge/docs-doxygen-blue.svg)](https://somozadev.github.io/CECSNET)

CECSNET is a performant **Entity Component System (ECS)** implemented in C, featuring built-in **client-server networking**, both static and dynamic library builds,  ready‑to‑use examples including cross‑engine integrations (Unreal & Unity), unit tests, and ready‑to‑use demos.

---

##  Overview

CECSNET offers:

- A clean, minimal‑overhead ECS core written in C.
- Built‑in networking modules for synchronizing ECS state over client-server architecture.
- Multiple integrations and examples:
  - **SFML_ECSNET_RAINDROPS** – raindrop simulation showcasing ECS and networking.
  - **SFML_ECSNET_PONG** – a classic pong game with ECS + networking.
  - **UNREAL_ECSNET** – Includes the Unreal Engine plugin importing ECSNET, a Unreal engine demo client project using it with a Raindrops server, and a demo build.
  - **UNITY_ECSNET** – Includes the C# wrapper to integrate ECSNET in Unity, a Unity demo build client using ECSNET networking as well as the project, and a unitypackage with the wrapper.
- Precompiled library artifacts (static/shared) and DLL. located under:
  - `ecsnet/build/lib/static/Release` → static `.lib` / `.a`
  - `ecsnet/build/bin/shared/Release` → dynamic `.dll` / `.so`
---

##  Features

- **ECS Core in C** – entity creation, component registration, system processing.
- **Networking Layer** – modules like `connection_manager`, `protocol_handler`, `network_cs`, and `network_map` enable client-server communication.
- **Serialisation** support for component syncing.
- **Sample Components** – built-in position, velocity, transform, and network entity components.
- **Extensible Design** – easy to add custom components and systems.
- **Multi‑platform** – works on Windows, Linux, macOS.


---

## 📂 Repository Structure

```
├── ecsnet/                          # Core library
│   ├── src/, include/               # Source and headers
│   ├── build/lib/static/Release/    # Static .lib
│   ├── build/lib/shared/Release/    # Shared .lib
│   └── build/bin/shared/Release/    # DLL
│
├── ecsnet/tests/                    # Unit tests (C executables)
│   └── test_*.exe                   # Example: test_ecs_basic.exe
│
├── SFML_ECSNET_RAINDROPS/           # SFML raindrops example
├── SFML_ECSNET_PONG/                # Pong example using SFML
├── UNREAL_ECSNET/                   # Unreal Engine plugin & demo
└── UNITY_ECSNET/                    # Unity wrapper and demo
```

---

##  How It Works

CECSNET is structured for clarity and modularity:

1. **Core ECS** handles entity/component lifecycle and update systems.
2. **Networking Modules**:
   - `connection_manager`: manage sockets and peer connections.
   - `protocol_handler`: encode/decode packets.
   - `network_cs`: manage ECS state synchronization between client and server.
   - `network_map`: translate network IDs to local entity references.
3. **Synchronization** – components can define custom (de)serialization to transmit network updates efficiently.
4. **Examples** demonstrate real-world usage from simulation to interactive games.

---

## 🧪 Running Tests

Unit tests are located under `ecsnet/tests/*`. To run them:

1. Open a terminal (cmd) in the project root.
2. Execute any of the `test_*.exe` files, e.g.:

```
test_dirty_flags.exe
test_ecs_basic.exe
test_ecs_dynamic.exe
test_net_socket.exe
test_network_architecture_basic.exe
test_network_ecs_integration.exe
test_serialization.exe
```

These ensure correct behavior of the ECS core and networking.

---

## 🛠 Building the Library

Clone & build:

```bash
git clone --branch ECSNET-0.1 https://github.com/somozadev/CECSNET.git
cd CECSNET
```

Use your preferred build system (CMake, Visual Studio, Makefiles). Ensure both static and dynamic builds are generated.

---

## 🎮 Demos

### 1. Native C++ + SFML

**Raindrops**
- Source: `SFML_ECSNET_RAINDROPS/src/RaindropsClient.cpp` & `RaindropsServer.cpp`
- Executables: `SFML_ECSNET_RAINDROPS/cmake-build-release/Release/`
- Behavior: Clients spawn raindrops by clicking.

**Pong**
- Source: `SFML_ECSNET_PONG/src/PONGClient.cpp` & `PONGServer.cpp`
- Executables: `SFML_ECSNET_PONG/cmake-build-release/Release/`
- Behavior: Clients move paddles with **W/S** or **Arrow Up/Down**.

**How to run:**
1. Start the `Server.exe` first (Windows Defender may warn since it's unsigned; temporarily disable antivirus if necessary).
2. Launch as many `Client.exe` instances as desired.

---

### 2. Unreal Engine Plugin

- Plugin: `UNREAL_ECSNET/Uecsnet/Plugins/ECSNET4UNREAL/`
- Demo Executable: `UNREAL_ECSNET/Uecsnet/Windows/Uecsnet.exe`
- Requires: `RaindropsServer.exe` running (copy available in `UNREAL_ECSNET/Uecsnet/RaindropsServer.exe`).

Behavior: Syncs like the Raindrops demo, but in Unreal Engine 5. Press **Space** to spawn drops where the camera is looking.

---

### 3. Unity C# Wrapper + Demo

- Wrapper: `UNITY_ECSNET/UNITY_ECSNET_WRAPPER/ECSNETWrapper.cs`
- Demo: `UNITY_ECSNET/UNITY_ECSNET_DEMO/`
- Client Executable: `UNITY_ECSNET/UNITY_ECSNET_DEMO/Build/TestECSNET.exe`

**How to run:**
1. Start `PongServer.exe`.
2. Run the Unity client executable.
3. Enter the server IP (default placeholders work for convenience). The client prints connection status on screen.

---


##  Documentation

API docs are auto-generated via **Doxygen**. Access at:
- [Online Documentation](https://somozadev.github.io/CECSNET)
- PDF: `./ecsnet_library_documentation.pdf`

Or generate it manually:

```bash
cd ecsnet
doxygen Doxyfile
open docs/html/index.html
```

---

##  Roadmap

- **Delta synchronization** to optimize bandwidth  ✓
- Support for **Peer-to-Peer (P2P)** network models  
- Enhanced **Unity and game engine integrations**  
- More example projects (e.g., mobile, web-based)

---

##  Contributing

Contributions welcome! Feel free to open issues or pull requests. Please follow the existing code conventions and ensure any examples remain functional.

---

##  License

CECSNET is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for full details.
