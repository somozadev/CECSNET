using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using TMPro;
using ECSNet;

public class SimpleClient : MonoBehaviour
{
    [Header("UI Input")]
    public TMP_InputField ipInput;
    public TMP_InputField portInput;

    private IntPtr _ecs = IntPtr.Zero;
    private IntPtr _networkArch = IntPtr.Zero;
    private GCHandle _selfHandle;

    private PacketReceivedCallback _packetReceivedDelegate;
    private PeerConnectedCallback _peerConnectedDelegate;
    private PeerDisconnectedCallback _peerDisconnectedDelegate;

    private string _serverPeerId;

    public static List<string> logs = new List<string>();
    public  TMP_Text LogsTextGO;
    public static TMP_Text LogsText;

    void Start()
    {
        _selfHandle = GCHandle.Alloc(this);
        _ecs = ECSNet.Native.ecs_create();
        ECSNet.Native.net_socket_init();

        _packetReceivedDelegate = OnPacketReceived;
        _peerConnectedDelegate = OnPeerConnected;
        _peerDisconnectedDelegate = OnPeerDisconnected;

        LogsText = LogsTextGO;
    }

    private static void AddLog(string log)
    {
        logs.Add(log);
        LogsText.text += log + "\n";
    }
    void Update()
    {
        if (_networkArch != IntPtr.Zero)
        {
            ECSNet.Native.network_architecture_update(_networkArch, Time.deltaTime);
        }
        if (_ecs != IntPtr.Zero)
        {
            ECSNet.Native.ecs_update(_ecs, Time.deltaTime);
        }
        
    }

    public void Connect()
    {
        string ip = ipInput != null ? ipInput.text : "127.0.0.1";
        ushort port = 51660;
        ushort.TryParse(portInput != null ? portInput.text : "51660", out port);

        AddLog($"Trying to connect to {ip}:{port}...");

        NetworkArchitectureConfig cfg = new NetworkArchitectureConfig
        {
            type = NetworkArchitectureType.ArchClientServer,
            ipAddress = ip,
            port = 0,
            isServer = false,
            tcpPort = port,
            udpPort = port,
            ecsSyncHz = 60.0f,
            onPeerConnected = _peerConnectedDelegate,
            onPeerDisconnected = _peerDisconnectedDelegate,
            onPacketReceived = _packetReceivedDelegate,
            onClientInput = null,
            userData = GCHandle.ToIntPtr(_selfHandle)
        };

        ECSNet.Native.network_architecture_init(out _networkArch, ref cfg, _ecs);
    }

    public void Disconnect()
    {
        Native.network_architecture_destroy(_networkArch);
        OnPeerDisconnected(IntPtr.Zero, IntPtr.Zero);
    }
    // Callbacks
    private static void OnPeerConnected(IntPtr userData, IntPtr peerPtr)
    {
        var handle = GCHandle.FromIntPtr(userData);
        if (handle.Target is SimpleClient client)
        {
            AddLog("Connected to the server.");
        }
    }

    private static void OnPeerDisconnected(IntPtr userData, IntPtr peerPtr)
    {
        var handle = GCHandle.FromIntPtr(userData);
        if (handle.Target is SimpleClient client)
        {
            AddLog("Disconnected from the server.");
        }
    }

    private static void OnPacketReceived(IntPtr userData, IntPtr peerPtr, IntPtr dataPtr, int length)
    {
        AddLog("Packet received!");
    }
}
