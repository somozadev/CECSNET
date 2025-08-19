using UnityEngine;
using System.Runtime.InteropServices; // Necesario para el atributo DllImport
using System;


public class ECSWrapper : MonoBehaviour
{
    public const string DLLName = "ecsnet";

    // Punteros para manejar las instancias C nativas.
    // networkArchitecturePtr se inicializará con el valor de retorno de la función C.
    private IntPtr networkArchitecturePtr = IntPtr.Zero;
    private IntPtr ecsPtr = IntPtr.Zero;
    
    // Variables públicas para configurar el manager desde el Inspector de Unity.
    public string ipAddress = "127.0.0.1";
    public ushort port = 8080;
    public bool isServer = false;

    [DllImport(DLLName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr ecs_init();

    [DllImport(DLLName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void ecs_destroy(IntPtr ecs);
    
    [DllImport(DLLName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int ecs_create_entity(IntPtr ecs);

    [DllImport(DLLName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void ecs_add_component(IntPtr ecs, int entity, int component_type, IntPtr data);
    
    // Funciones de la arquitectura de red
    [DllImport(DLLName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr network_cs_init(ref NetworkArchitectureConfig config, IntPtr ecs);

    [DllImport(DLLName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void network_cs_update(IntPtr architecture);

    [DllImport(DLLName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void network_cs_destroy(IntPtr architecture);

    // El método Start() es donde se inicializa la librería.
    private void Start()
    {
        ecs_t server_ecs, client_ecs;
        ecs_init(&server_ecs);
        ecs_register_builtin_components(&server_ecs);
        ecs_register_builtin_systems(&server_ecs);
        
        // 1. Crea una instancia de la estructura de configuración de C# con los valores deseados.
        NetworkArchitectureConfig config = new NetworkArchitectureConfig
        {
            type = 0, // 0 corresponde a ARCH_CLIENT_SERVER en tu enum
            ip_address = ipAddress,
            port = port,
            is_server = isServer
        };
        
        // 2. Llama a la función de inicialización de la librería C.
        // Ahora se asigna el valor de retorno de la función (un puntero) a networkArchitecturePtr.
        // Se pasa la estructura 'config' por referencia (ref) para que C pueda leer sus valores.
        // Se pasa ecsPtr como un puntero nulo, ya que tu código C lo maneja como un puntero opaco.
        networkArchitecturePtr = network_cs_init(ref config, ecsPtr);

        // 3. Verifica que la inicialización fue exitosa.
        if (networkArchitecturePtr != IntPtr.Zero)
        {
            Debug.Log("Librería de red inicializada correctamente.");
        }
        else
        {
            Debug.LogError("Fallo al inicializar la librería de red.");
        }
    }
    
    private void Update()
    {
        // Llama a la función de actualización en cada frame, como en tu bucle de prueba en C.
        if (networkArchitecturePtr != IntPtr.Zero)
        {
            network_cs_update(networkArchitecturePtr);
        }
    }
    
    // OnApplicationQuit se llama cuando la aplicación se cierra.
    private void OnApplicationQuit()
    {
        // Asegúrate de liberar los recursos de tu DLL cuando ya no se necesiten,
        // igual que en la parte de 'Cleanup' de tu código de prueba en C.
        if (networkArchitecturePtr != IntPtr.Zero)
        {
            network_cs_destroy(networkArchitecturePtr);
            networkArchitecturePtr = IntPtr.Zero;
        }
    }
}


[StructLayout(LayoutKind.Sequential)]
public struct NetworkArchitectureConfig
{
    public int type;
    [MarshalAs(UnmanagedType.LPStr)]
    public string ip_address;
    public UInt16 port;
    [MarshalAs(UnmanagedType.I1)]
    public bool is_server;
}
[StructLayout(LayoutKind.Sequential)]
public struct PositionComponent
{
    public float x;
    public float y;
}

[StructLayout(LayoutKind.Sequential)]
public struct VelocityComponent
{
    public float x;
    public float y;
}