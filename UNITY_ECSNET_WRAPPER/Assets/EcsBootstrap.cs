using UnityEngine;
using ECSNET;

public class EcsBootstrap : MonoBehaviour
{
    [Header("Network Config")]
    public bool isServer = true;
    public string serverIp = "127.0.0.1";
    public ushort tcpPort = 7777;
    public ushort udpPort = 7778;

    void Awake()
    {
        EcsNet.InitECS();
        EcsNet.UpdateECS(0f); 
        EcsNet.InitNetwork(isServer, serverIp, tcpPort, udpPort);
    }

    void Update()
    {
        // step ECS
        EcsNet.UpdateECS(Time.deltaTime);
        // step Networking
        EcsNet.UpdateNetwork(Time.deltaTime);
    }

    void OnApplicationQuit()
    {
        EcsNet.ShutdownNetwork();
        EcsNet.OnApplicationQuit();
    }
}