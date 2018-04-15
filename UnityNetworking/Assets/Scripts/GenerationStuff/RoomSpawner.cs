using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class RoomSpawner : NetworkBehaviour {

    [SerializeField]
    private GameObject[] roomPrefabs;

    public bool ShouldSpawn = true;
    private bool isBlocked = false;

    public void Spawn()
    {
        CmdSpawn();
    }

    private void CmdSpawn()
    {
        if (ShouldSpawn && !isBlocked)
        {
            int ridx = Random.Range(0, roomPrefabs.Length);

            GameObject room = Instantiate(roomPrefabs[ridx], transform.position, transform.rotation);
            NetworkServer.Spawn(room);
            ShouldSpawn = false;
        }
    }

    private void OnTriggerStay(Collider other)
    {
        isBlocked = true;
    }
}
