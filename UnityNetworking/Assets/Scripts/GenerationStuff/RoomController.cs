using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class RoomController : NetworkBehaviour {

    private RoomSpawner[] spawners;

	// Use this for initialization
	void Start ()
    {
        spawners = GetComponentsInChildren<RoomSpawner>();	
	}

    private void spawnNeighbours()
    {
        Debug.Log("<< spawnNeighbours : " + transform.GetHashCode());
        if (isServer)
        { 
            foreach(RoomSpawner spawner in spawners)
            {
                Debug.Log("Calling spawner : " + spawner.transform.GetHashCode());
                spawner.Spawn();
            }
        }
    }

    private void OnTriggerEnter(Collider other)
    {
        if(other.CompareTag("Player"))
        {
            Debug.Log("Player entered " + transform.GetHashCode());
            spawnNeighbours();
        }
    }
}
