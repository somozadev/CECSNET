using UnityEngine;
using ECSNET;

[RequireComponent(typeof(EcsSyncTransform))]
public class BallController : MonoBehaviour
{
    private int _entity;

    void Start()
    {
        _entity = GetComponent<EcsSyncTransform>().EntityId;

        // velocity inicial
        velocity_t vel = new velocity_t { vx = 3.0f, vy = 2.0f };
        EcsNet.AddComponent(_entity, EcsNet.COMPONENT_VELOCITY, ref vel);
    }

    void Update()
    {
        // Rebotar contra paredes
        Vector3 pos = transform.position;
        if (pos.y > 4.5f || pos.y < -4.5f)
        {
            velocity_t vel = EcsNet.GetComponent<velocity_t>(_entity, EcsNet.COMPONENT_VELOCITY);
            vel.vy *= -1;
            EcsNet.SetComponent(_entity, EcsNet.COMPONENT_VELOCITY, ref vel);
        }
    }
}