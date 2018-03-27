using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerSpawner : MonoBehaviour {

    public PlayerController playerPrefab;
    public GameObject flagPrefab;

    private SphereCollider rangeBubble;

	// Use this for initialization
	void Start ()
    {
        rangeBubble = GetComponent<SphereCollider>();
        Spawn(playerPrefab.gameObject);
        Spawn(flagPrefab);
	}

    private void Spawn(GameObject o)
    {
        Vector3 spawnpoint = rangeBubble == null ?
            transform.position : 
            ToXZPlane(Random.insideUnitCircle * rangeBubble.radius);

        Instantiate(o,
            spawnpoint,
            transform.rotation);
    }

    private Vector3 ToXZPlane(Vector2 incoming)
    {
        Vector3 v = new Vector3(incoming.x, transform.position.y + playerPrefab.transform.localScale.y, incoming.y);
        return v;
    }
}
