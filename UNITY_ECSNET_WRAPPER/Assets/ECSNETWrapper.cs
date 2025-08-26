using System;
using System.Runtime.InteropServices;
using Unity.VisualScripting;
using UnityEngine;

namespace ECSNET
{
    internal static class EcsNetNative
    {
        private const string DllName = "ecsnet";

#if UNITY_STANDALONE_WIN
        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr LoadLibrary(string lpFileName);
#endif

        private static IntPtr _dllHandle;


        private static IntPtr EnsureDll()
        {
            if (_dllHandle == IntPtr.Zero)
            {
                _dllHandle = LoadLibrary(DllName + ".dll");
                if (_dllHandle == IntPtr.Zero)
                    throw new Exception("No se pudo cargar ecsnet.dll");
            }
            return _dllHandle;
        }

        private static int ReadGlobalInt(string symbol)
        {
            IntPtr handle = EnsureDll();
            IntPtr addr = GetProcAddress(handle, symbol);
            if (addr == IntPtr.Zero)
                throw new Exception($"No se encontró el símbolo global {symbol} en ecsnet.dll");
            return Marshal.ReadInt32(addr);
        }


        // ==========================
        // Built-in component IDs (variables globales exportadas)
        // ==========================
        public static int COMPONENT_POSITION => ReadGlobalInt("COMPONENT_POSITION");
        public static int COMPONENT_ROTATION => ReadGlobalInt("COMPONENT_ROTATION");
        public static int COMPONENT_TRANSFORM => ReadGlobalInt("COMPONENT_TRANSFORM");
        public static int COMPONENT_VELOCITY => ReadGlobalInt("COMPONENT_VELOCITY");
        public static int COMPONENT_NETWORKED_ENTITY => ReadGlobalInt("COMPONENT_NETWORKED_ENTITY");


        // ==========================
        // ECS core
        // ==========================
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ecs_create();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_destroy(IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_init(IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_update(IntPtr ecs, float dt);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ecs_create_entity(IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_destroy_entity(IntPtr ecs, int entity);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ecs_try_create_entity_by_id(IntPtr ecs, uint networkId);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ecs_add_component(IntPtr ecs, int entity, int component, IntPtr data);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_remove_component(IntPtr ecs, int entity, int component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ecs_has_component(IntPtr ecs, int entity, int component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ecs_get_component(IntPtr ecs, int entity, int component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_mark_component_dirty(IntPtr ecs, int entity, int component);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ecs_register_component(IntPtr ecs, ComponentDescriptor desc);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_serialize_entity(IntPtr ecs, int entity, IntPtr buffer, ref int length);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ecs_deserialize_entity(IntPtr ecs, IntPtr buffer, int length);

        // ==========================
        // Networking core
        // ==========================
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void net_socket_init();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void net_socket_cleanup();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_architecture_init(out IntPtr arch, ref NetworkArchitectureConfig config, IntPtr ecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_architecture_update(IntPtr arch, float dt);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_architecture_destroy(IntPtr arch);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_init(out IntPtr impl);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_update(IntPtr impl, float dt);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_destroy(IntPtr impl);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_mark_entity_destroy(IntPtr impl, int entity);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void network_cs_mark_network_id_destroy(IntPtr impl, uint netId);

        // ==========================
        // Protocol handler
        // ==========================
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void protocol_handler_init(IntPtr handler);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void protocol_handler_pack_client_input(IntPtr handler, int entity, byte cmd);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void protocol_handler_pack_entity_update(IntPtr handler, int entity, IntPtr data, ushort length);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void protocol_handler_send_packet(IntPtr connectionManager, string peerId, IntPtr handler);

        // ==========================
        // Built-in serializers
        // ==========================
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void serialize_position(IntPtr data, IntPtr outBuffer);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void deserialize_position(IntPtr inBuffer, IntPtr data);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void serialize_rotation(IntPtr data, IntPtr outBuffer);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void deserialize_rotation(IntPtr inBuffer, IntPtr data);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void serialize_transform(IntPtr data, IntPtr outBuffer);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void deserialize_transform(IntPtr inBuffer, IntPtr data);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void serialize_velocity(IntPtr data, IntPtr outBuffer);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void deserialize_velocity(IntPtr inBuffer, IntPtr data);
    }

    // ==========================
    // Struct mappings
    // ==========================
    [StructLayout(LayoutKind.Sequential)]
    public struct ComponentDescriptor
    {
        public UIntPtr size;
        [MarshalAs(UnmanagedType.LPStr)] public string name;
        public IntPtr serialize;
        public IntPtr deserialize;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NetworkArchitectureConfig
    {
        public int type;
        [MarshalAs(UnmanagedType.LPStr)] public string ip_address;
        [MarshalAs(UnmanagedType.I1)] public bool is_server;
        public ushort tcp_port;
        public ushort udp_port;
        public IntPtr on_peer_connected;
        public IntPtr on_peer_disconnected;
        public IntPtr on_client_input;
        public IntPtr user_data;
        public float ecs_sync_hz;
    }

    // ==========================
    // ECSNet API de conveniencia
    // ==========================
    public static class EcsNet
    {
        private static IntPtr _ecs = Marshal.AllocHGlobal(1024 * 1024); // TODO: Mejor usar un ctor nativo en C
        private static IntPtr _arch = IntPtr.Zero;

        public static void InitECS()
        {
            _ecs = EcsNetNative.ecs_create();

            // cachear IDs después de que ecs_register_builtin_components se ejecute
            COMPONENT_POSITION = EcsNetNative.COMPONENT_POSITION;
            COMPONENT_ROTATION = EcsNetNative.COMPONENT_ROTATION;
            COMPONENT_TRANSFORM = EcsNetNative.COMPONENT_TRANSFORM;
            COMPONENT_VELOCITY = EcsNetNative.COMPONENT_VELOCITY;
            COMPONENT_NETWORKED_ENTITY = EcsNetNative.COMPONENT_NETWORKED_ENTITY;

            Debug.Log("[ECSNet] ECS inicializado");
        }


        public static void OnApplicationQuit()
        {
            // EcsNetNative.ecs_destroy(_ecs);
        }

        public static void UpdateECS(float dt) => EcsNetNative.ecs_update(_ecs, dt);

        public static int CreateEntity() => EcsNetNative.ecs_create_entity(_ecs);

        public static void DestroyEntity(int entity) => EcsNetNative.ecs_destroy_entity(_ecs, entity);

        public static int CreateEntityById(uint networkId) => EcsNetNative.ecs_try_create_entity_by_id(_ecs, networkId);

        public static void AddComponent<T>(int entity, int componentId, ref T data) where T : struct
        {
            int size = Marshal.SizeOf<T>();
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(data, ptr, false);
            bool ok = EcsNetNative.ecs_add_component(_ecs, entity, componentId, ptr);
            Marshal.FreeHGlobal(ptr);

            if (!ok)
                throw new Exception($"ecs_add_component failed for entity {entity}, comp {componentId}");

        }

        public static T GetComponent<T>(int entity, int componentId) where T : struct
        {
            IntPtr ptr = EcsNetNative.ecs_get_component(_ecs, entity, componentId);
            if (ptr == IntPtr.Zero)
                throw new Exception($"Entity {entity} does not have component {componentId}");
            return Marshal.PtrToStructure<T>(ptr);
        }

        public static void SetComponent<T>(int entity, int componentId, ref T data) where T : struct
        {
            IntPtr ptr = EcsNetNative.ecs_get_component(_ecs, entity, componentId);
            if (ptr == IntPtr.Zero)
                throw new Exception($"Entity {entity} does not have component {componentId}");

            Marshal.StructureToPtr(data, ptr, false);
            EcsNetNative.ecs_mark_component_dirty(_ecs, entity, componentId);
        }

        public static bool HasComponent(int entity, int componentId) => EcsNetNative.ecs_has_component(_ecs, entity, componentId);

        public static int RegisterComponent<T>(string name) where T : struct
        {
            ComponentDescriptor desc = new ComponentDescriptor
            {
                name = name,
                size = (UIntPtr)Marshal.SizeOf<T>(),
                serialize = IntPtr.Zero,
                deserialize = IntPtr.Zero
            };
            return EcsNetNative.ecs_register_component(_ecs, desc);
        }

        public static void InitNetwork(bool isServer, string ip, ushort tcpPort, ushort udpPort)
        {
            NetworkArchitectureConfig cfg = new NetworkArchitectureConfig
            {
                type = 0,
                ip_address = ip,
                is_server = isServer,
                tcp_port = tcpPort,
                udp_port = udpPort,
                ecs_sync_hz = 60.0f,
                on_peer_connected = IntPtr.Zero,
                on_peer_disconnected = IntPtr.Zero,
                on_client_input = IntPtr.Zero,
                user_data = IntPtr.Zero
            };
            EcsNetNative.network_architecture_init(out _arch, ref cfg, _ecs);
            Debug.Log($"[ECSNet] Networking init (server={isServer})");
        }

        public static void UpdateNetwork(float dt)
        {
            if (_arch != IntPtr.Zero)
                EcsNetNative.network_architecture_update(_arch, dt);
        }

        public static void ShutdownNetwork()
        {
            if (_arch != IntPtr.Zero)
                EcsNetNative.network_architecture_destroy(_arch);
            _arch = IntPtr.Zero;
        }

        // ======================
        // Helpers de built-in
        // ======================
        public static int COMPONENT_POSITION;
        public static int COMPONENT_ROTATION;
        public static int COMPONENT_TRANSFORM;
        public static int COMPONENT_VELOCITY;
        public static int COMPONENT_NETWORKED_ENTITY;

    }

    // ======================
    // Structs de componentes
    // ======================
    [StructLayout(LayoutKind.Sequential)]
    public struct position_t
    {
        public float x;
        public float y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct velocity_t
    {
        public float vx;
        public float vy;
    }
}