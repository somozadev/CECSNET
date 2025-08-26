using System;
using UnityEngine;
using ECSNET;

[RequireComponent(typeof(EcsSyncTransform))]
public class PaddleController : MonoBehaviour
{
    public KeyCode upKey = KeyCode.W;
    public KeyCode downKey = KeyCode.S;
    public float speed = 5f;

    private int _entity;

    void Start()
    {
        _entity = GetComponent<EcsSyncTransform>().EntityId;

        if (!EcsNet.HasComponent(_entity, EcsNet.COMPONENT_VELOCITY))
        {
            velocity_t vel = new velocity_t { vx = 0, vy = 0 };
            EcsNet.AddComponent(_entity, EcsNet.COMPONENT_VELOCITY, ref vel);
        }
        Debug.Log($"Has velocity? {EcsNet.HasComponent(_entity, EcsNet.COMPONENT_VELOCITY)}");
        try
        {
            velocity_t velCheck = EcsNet.GetComponent<velocity_t>(_entity, EcsNet.COMPONENT_VELOCITY);
            Debug.Log($"Velocity initial: {velCheck.vx}, {velCheck.vy}");
        }
        catch (Exception e)
        {
            Debug.LogError("GetComponent failed: " + e);
        }

    }


    void Update()
    {
        velocity_t vel = new velocity_t { vx = 0, vy = 0 };

        if (Input.GetKey(upKey)) vel.vy = speed;
        if (Input.GetKey(downKey)) vel.vy = -speed;

        EcsNet.SetComponent(_entity, EcsNet.COMPONENT_VELOCITY, ref vel);
    }
}