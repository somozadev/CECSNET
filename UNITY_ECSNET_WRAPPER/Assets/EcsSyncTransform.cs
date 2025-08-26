using UnityEngine;
using ECSNET;

public class EcsSyncTransform : MonoBehaviour
{
    public int EntityId { get; private set; }

    void Awake()
    {
        // Crear entidad ECS para este GameObject
        EntityId = EcsNet.CreateEntity();

        // Position inicial
        position_t pos = new position_t { x = transform.position.x, y = transform.position.y };
        EcsNet.AddComponent(EntityId, EcsNet.COMPONENT_POSITION, ref pos);
    }

    void Update()
    {
        if (EcsNet.HasComponent(EntityId, EcsNet.COMPONENT_POSITION))
        {
            position_t pos = EcsNet.GetComponent<position_t>(EntityId, EcsNet.COMPONENT_POSITION);
            transform.position = new Vector3(pos.x, pos.y, 0);
        }
    }
}