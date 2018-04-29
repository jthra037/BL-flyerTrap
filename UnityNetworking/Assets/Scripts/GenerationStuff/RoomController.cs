using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class RoomController : NetworkBehaviour {

    [SerializeField]
    private Transform[] objectSpawnPoint;

    [SerializeField] private GameObject flagPrefab;
    [SerializeField] private GameObject flagstandPrefab;

    private RoomSpawner[] spawners;

    private GameManager gm;

	// Use this for initialization
	void Start ()
    {
        spawners = GetComponentsInChildren<RoomSpawner>();
        gm = GameObject.FindGameObjectWithTag("GameController").GetComponent<GameManager>();
        gm.Register(this);
	}

    private void spawnNeighbours()
    {
        if (isServer)
        { 
            foreach(RoomSpawner spawner in spawners)
            {
                spawner.Spawn();
            }
        }
    }

    public void SpawnFlag()
    {
        Debug.Log(" << SpawnFLag : " + transform.GetHashCode());
        int ridx = Random.Range(0, objectSpawnPoint.Length);
        GameObject flag = Instantiate(flagPrefab, 
            objectSpawnPoint[ridx].position, 
            objectSpawnPoint[ridx].rotation);

        NetworkServer.Spawn(flag);
        Debug.Log(" >> SpawnFLag : " + transform.GetHashCode());
    }

    public void SpawnFlagstand()
    {
        Debug.Log(" << SpawnFLagstand : " + transform.GetHashCode());
        int ridx = Random.Range(0, objectSpawnPoint.Length - 1);
        GameObject flagstand = Instantiate(flagstandPrefab,
            objectSpawnPoint[ridx].position,
            objectSpawnPoint[ridx].rotation);

        NetworkServer.Spawn(flagstand);
        Debug.Log(" >> SpawnFLagstand : " + transform.GetHashCode());
    }

    private void OnTriggerEnter(Collider other)
    {
        if(other.CompareTag("Player"))
        {
            spawnNeighbours();
        }
    }
}
