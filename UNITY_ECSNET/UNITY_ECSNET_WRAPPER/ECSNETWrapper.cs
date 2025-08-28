using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

/*
 * Auto‑generated wrapper for the native ECSNet C library.
 *
 * This file exposes the public API marked with ECSNET_API in the C headers
 * via P/Invoke so that managed code (such as a Unity project) can call
 * directly into the unmanaged ecsnet.dll.  It also declares a handful of
 * supporting types (enums, structs and delegate types) that mirror their
 * C counterparts.  See the C header files in the ecsnet distribution for
 * documentation on each function and data structure.
 *
 * To use this wrapper simply drop this file into your Unity project (for
 * example under Assets/Scripts) and ensure that the ecsnet DLL is placed
 * somewhere in the plugin search path (e.g. Assets/Plugins/x86_64 on
 * Windows).  All functions use the C calling convention.  The wrapper
 * deliberately omits any higher level logic; it merely exposes the raw
 * interop surfaces.
 */

namespace ECSNet
{
    /// <summary>
    /// Enumeration of network architecture types.  See network_architecture_type_t in C code.
    /// </summary>
    public enum NetworkArchitectureType : int
    {
        ArchClientServer = 0,
        ArchP2P = 1,
    }

    /// <summary>
    /// Enumeration of packet types used by the protocol handler.  Mirrors packet_type_t in C.
    /// </summary>
    public enum PacketType : int
    {
        Invalid = 0,
        EntityUpdate = 1,
        MultiEntityUpdate = 2,
        ClientRegister = 3,
        ServerAck = 4,
        ClientInput = 5
    }

    /// <summary>
    /// Flags used for client input commands.  See protocol_handler.h INPUT_UP/INPUT_DOWN definitions.
    /// </summary>
    [Flags]
    public enum InputFlags : byte
    {
        None = 0x00,
        Up = 0x01,
        Down = 0x02,
        Spawn = 0x80
    }

    /// <summary>
    /// Delegate type for peer connection notifications.  Mirrors the function
    /// pointer signature void (*on_peer_connected)(void*, peer_t*) in C.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PeerConnectedCallback(IntPtr userData, IntPtr peer);

    /// <summary>
    /// Delegate type for peer disconnection notifications.  Mirrors void (*on_peer_disconnected)(void*, peer_t*) in C.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PeerDisconnectedCallback(IntPtr userData, IntPtr peer);

    /// <summary>
    /// Delegate type for packet reception.  Mirrors void (*on_packet_received)(void*, peer_t*, const void*, int) in C.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PacketReceivedCallback(IntPtr userData, IntPtr peer, IntPtr data, int length);

    /// <summary>
    /// Delegate type for client input messages received on the server.  Mirrors void (*on_client_input)(void*, peer_t*, entity_t, uint8_t, const void*, uint16_t) in C.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void ClientInputCallback(IntPtr userData, IntPtr peer, uint entityId, byte command, IntPtr extra, ushort extraLength);

    /// <summary>
    /// Managed representation of the network_architecture_config_t struct in C.  The
    /// field order and packing are chosen to match the native layout exactly.  Note
    /// that strings and delegates are marshalled automatically by the runtime.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct NetworkArchitectureConfig
    {
        /// <summary>Type of network architecture (client/server, P2P, etc).</summary>
        public NetworkArchitectureType type;

        /// <summary>IP address to bind or connect to (null for default).</summary>
        [MarshalAs(UnmanagedType.LPStr)]
        public string ipAddress;

        /// <summary>Main port for communication (unused in client/server).</summary>
        public ushort port;

        /// <summary>When true the node acts as a server; false means a client.</summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool isServer;

        /// <summary>TCP port for connection establishment.</summary>
        public ushort tcpPort;

        /// <summary>UDP port for unreliable communication.</summary>
        public ushort udpPort;

        /// <summary>Frequency in Hz at which the ECS synchronisation runs.</summary>
        public float ecsSyncHz;

        /// <summary>Callback invoked when a new peer connects.</summary>
        public PeerConnectedCallback onPeerConnected;

        /// <summary>Callback invoked when a peer disconnects.</summary>
        public PeerDisconnectedCallback onPeerDisconnected;

        /// <summary>Callback invoked when a packet is received.</summary>
        public PacketReceivedCallback onPacketReceived;

        /// <summary>Callback invoked on the server when client input is received.</summary>
        public ClientInputCallback onClientInput;

        /// <summary>Opaque user data passed to all callbacks.</summary>
        public IntPtr userData;
    }

    /// <summary>
    /// Representation of the packet header used by the protocol handler.
    ///
    /// The native <c>packet_header_t</c> has the following layout:
    ///
    /// <list type="bullet">
    ///   <item><c>uint16_t size</c> – total packet length in bytes</item>
    ///   <item><c>uint16_t pad</c> – two bytes of padding to align the next field</item>
    ///   <item><c>packet_type_t type</c> – the packet type (32‑bit enum)</item>
    /// </list>
    ///
    /// Without the explicit padding field the struct would only be 6 bytes in C# when
    /// using <c>Pack = 1</c>, whereas the native code expects an 8‑byte header.  The
    /// extra <see cref="padding"/> field ensures the managed and unmanaged layouts
    /// match exactly on all platforms.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct PacketHeader
    {
        /// <summary>Total size of the packet in bytes (including this header).</summary>
        public ushort size;
        /// <summary>Two bytes of unused padding required by the native layout.</summary>
        public ushort padding;
        /// <summary>The type of the packet.</summary>
        public PacketType type;
    }

    /// <summary>
    /// Representation of a network packet consisting of a header and a fixed‑size
    /// payload.  The payload length is MAX_PACKET_SIZE - sizeof(PacketHeader) and
    /// matches the C definition.  MAX_PACKET_SIZE is defined as 1024 in the
    /// native headers.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct NetworkPacket
    {
        public PacketHeader header;
        // Reserve space for the packet payload.  The native MAX_PACKET_SIZE is
        // 1024 bytes and the header consumes 8 bytes (size + pad + type), leaving
        // 1016 bytes for the data.  We declare the field as a fixed buffer to
        // avoid array marshalling overhead.
        public fixed byte data[1016];
    }

    /// <summary>
    /// Representation of the protocol handler.  It simply contains two
    /// network_packet_t structures for inbound and outbound traffic.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct ProtocolHandler
    {
        public NetworkPacket outPacket;
        public NetworkPacket inPacket;
    }

    /// <summary>
    /// Static class exposing all native functions.  Each method is marked
    /// extern and is decorated with DllImport specifying the calling convention.
    /// If the native library has a different name on your platform adjust
    /// the DllImport attribute accordingly (e.g. "ecsnet.dll" on Windows or
    /// "libecsnet.so" on Linux/macOS).
    /// </summary>
    public static class Native
    {
        private const string DllName = "ecsnet";

        // -----------------------------------------------------------------
        // ECS core functions
        // -----------------------------------------------------------------

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ecs_create();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_destroy(IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_init(IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ecs_create_entity(IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ecs_try_create_entity_by_id(IntPtr ecs, uint id);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_destroy_entity(IntPtr ecs, uint entity);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ecs_serialize_entity(IntPtr ecs, uint entity, IntPtr outBuffer, ref UIntPtr outSize, UIntPtr maxOutSize);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ecs_deserialize_entity(IntPtr ecs, IntPtr inBuffer);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ecs_add_component(IntPtr ecs, uint entity, uint component, IntPtr data);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ecs_get_component(IntPtr ecs, uint entity, uint component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ecs_get_component_name(IntPtr ecs, uint component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ecs_has_component(IntPtr ecs, uint entity, uint component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ecs_is_component_dirty(IntPtr ecs, uint entity, uint component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ecs_remove_component(IntPtr ecs, uint entity, uint component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_mark_component_dirty(IntPtr ecs, uint entity, uint component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_set_dirty_hook(EcsDirtyHook hook);

        /// <summary>
        /// Delegate for the ecs dirty hook callback.  It is invoked when any
        /// component becomes dirty in the native ECS.  This wrapper does not
        /// use the hook directly but exposes it so that clients can set their
        /// own handlers if necessary.
        /// </summary>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void EcsDirtyHook(uint entity);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ecs_get_dirty_components(IntPtr ecs, uint entity, IntPtr outDirtyComponents);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_clear_component_dirty(IntPtr ecs, uint entity, uint component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ecs_register_component(IntPtr ecs, ComponentDescriptor descriptor);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_register_system(IntPtr ecs, SystemFunc function);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void SystemFunc(IntPtr ecs, float dt);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_update(IntPtr ecs, float dt);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_register_builtin_systems(IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_register_builtin_components(IntPtr ecs);

        // -----------------------------------------------------------------
        // Component descriptor and related types
        // -----------------------------------------------------------------

        /// <summary>
        /// Managed mirror of component_descriptor_t.  When registering a new
        /// component type in the ECS, populate this structure and pass it to
        /// ecs_register_component().  Pointers to serialize and deserialize
        /// functions may be set to IntPtr.Zero if you do not need custom
        /// serialisation.  Names must be ASCII strings as LPStr.
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        public struct ComponentDescriptor
        {
            public UIntPtr size;
            [MarshalAs(UnmanagedType.LPStr)]
            public string name;
            public SerializeFunc serialize;
            public DeserializeFunc deserialize;
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void SerializeFunc(IntPtr dataIn, IntPtr bufferOut);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void DeserializeFunc(IntPtr bufferIn, IntPtr dataOut);

        // -----------------------------------------------------------------
        // Network architecture functions
        // -----------------------------------------------------------------

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_architecture_init(out IntPtr architecture, ref NetworkArchitectureConfig config, IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_architecture_update(IntPtr architecture, float dt);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_architecture_destroy(IntPtr architecture);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool network_architecture_connect_to_server(IntPtr architecture, [MarshalAs(UnmanagedType.LPStr)] string ipAddress, ushort port);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool network_architecture_send_to_peer(IntPtr architecture, uint peerId, IntPtr data, int length);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool network_architecture_send_entity_update(IntPtr architecture, uint peerId, uint entityId, IntPtr componentData, int dataLength);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool network_architecture_broadcast(IntPtr architecture, IntPtr data, int length);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int network_architecture_get_peer_count(IntPtr architecture);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr network_architecture_get_peer(IntPtr architecture, uint peerId);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr network_architecture_get_connection_manager(IntPtr architecture);

        // -----------------------------------------------------------------
        // Client/server specific API (network_cs)
        // -----------------------------------------------------------------

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr network_cs_init(ref NetworkArchitectureConfig config, IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_update(IntPtr networkCs, float dt);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_destroy(IntPtr networkCs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_mark_entity_destroy(IntPtr networkCs, uint entity);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_mark_network_id_destroy(IntPtr networkCs, uint networkId);

        // -----------------------------------------------------------------
        // Protocol handler functions
        // -----------------------------------------------------------------

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void protocol_handler_init(IntPtr handler);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void protocol_handler_pack_entity_update(IntPtr handler, uint entityId, IntPtr data, ushort dataLength);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "protocol_handler_pack_client_input")]
        public static extern void protocol_handler_pack_client_input(IntPtr handler, uint entityId, byte inputCmd, IntPtr extra, ushort extraLength);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "protocol_handler_unpack_client_input")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool protocol_handler_unpack_client_input(IntPtr packet, out uint entityId, out byte inputCmd, out IntPtr extra, out ushort extraLength);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void protocol_handler_send_packet(IntPtr connectionManager, [MarshalAs(UnmanagedType.LPStr)] string peerId, IntPtr handler);

        // -----------------------------------------------------------------
        // Socket subsystem functions
        // -----------------------------------------------------------------

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void net_socket_init();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void net_socket_cleanup();

        // -----------------------------------------------------------------
        // Shim function to send input easily from Unity.  It wraps the
        // protocol_handler_pack_client_input() and send logic for convenience.
        // See ecsnet_unity_shim.h for the native definition.
        // -----------------------------------------------------------------

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecsnet_send_input(IntPtr architecture, IntPtr serverPeer, byte cmd);
    }
}