# CECSNET

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build](https://img.shields.io/github/actions/workflow/status/somozadev/CECSNET/build.yml?branch=ECSNET-0.1)](https://github.com/somozadev/CECSNET/actions)
[![Docs](https://img.shields.io/badge/docs-doxygen-blue.svg)](https://somozadev.github.io/CECSNET)

CECSNET is a performant **Entity Component System (ECS)** implemented in C, featuring built-in **client-server networking**, ready‑to‑use examples (SFML and Unity), and both static and dynamic library builds.

---

##  Overview

CECSNET offers:

- A clean, minimal‑overhead ECS core written in C.
- Built‑in networking modules for synchronizing ECS state over client-server architecture.
- Pre‑built example applications:
  - **SFML_ECSNET_RAINDROPS** – visualizes ECS behavior with raindrop simulation.
  - **SFML_ECSNET_PONG** – classic pong game using ECS and networking.
  - **UNITY_ECSNET_WRAPPER** – Unity C# wrapper to integrate CECSNET in Unity projects.
- Compiled library artifacts located under:
  - `ecsnet/build/lib/static/Debug` → static `.lib` / `.a`
  - `ecsnet/build/bin/shared/Debug` → dynamic `.dll` / `.so`

---

##  Features

- **ECS Core in C** – entity creation, component registration, system processing.
- **Networking Layer** – modules like `connection_manager`, `protocol_handler`, `network_cs`, and `network_map` enable client-server communication.
- **Serialisation** support for component syncing.
- **Sample Components** – built-in position, velocity, transform, and network entity components.
- **Extensible Design** – easy to add custom components and systems.
- **Multi‑platform** – works on Windows, Linux, macOS.

---

##  Getting Started

### Clone & Structure

```bash
git clone --branch ECSNET-0.1 https://github.com/somozadev/CECSNET.git
cd CECSNET
```

Explore the structure:

```
├── ecsnet/                          # Core library
│   ├── src/, include/               # Source and headers
│   └── build/                       # Pre-built binaries (static & shared)
├── SFML_ECSNET_RAINDROPS/           # SFML raindrops example
├── SFML_ECSNET_PONG/                # Pong example using SFML
└── UNITY_ECSNET_WRAPPER/            # Unity C# wrapper project
```

### Building the Library

Use your preferred build system (e.g., CMake, Visual Studio, or Makefiles). Ensure both static and dynamic builds are generated as shipped.

### Running Examples

1. **SFML Examples**  
   Open the respective folder (e.g., `SFML_ECSNET_RAINDROPS/`), follow README inside to build and run using CECSNET.

2. **Unity Wrapper**  
   Open `UNITY_ECSNET_WRAPPER/` in Unity Editor and integrate the CECSNET library to utilize ECS with Unity components.

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

##  Documentation

API docs are auto-generated via **Doxygen**. Access online at:

👉 [Documentation](https://somozadev.github.io/CECSNET)

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
