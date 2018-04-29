using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Bullet : MonoBehaviour {
    void OnCollisionEnter(Collision collision)
    {
        GameObject hit = collision.gameObject;
        Health health = hit.GetComponent<Health>();
        PlayerController pc = hit.GetComponent<PlayerController>();
        if (health != null)
        {
            health.TakeDamage(10);
        }

        if(pc != null)
        {
            pc.DropFlag();
            pc.RpcDropFlag();
        }

        Destroy(gameObject);
    }
}
