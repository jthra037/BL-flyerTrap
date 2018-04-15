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
        Debug.Log("<< Spawn : " + transform.GetHashCode());
        CmdSpawn();
        Debug.Log(">> Spawn : " + transform.GetHashCode());
    }

    private void CmdSpawn()
    {
        Debug.Log("<< CmdSpawn on " + transform.GetHashCode());
        Debug.Log("Can spawn: " + (ShouldSpawn && !isBlocked));
        if (ShouldSpawn && !isBlocked)
        {
            int ridx = Random.Range(0, roomPrefabs.Length);
            Debug.Log("Random index: " + ridx);

            GameObject room = Instantiate(roomPrefabs[ridx], transform.position, transform.rotation);
            NetworkServer.Spawn(room);
            ShouldSpawn = false;
        }
        Debug.Log(">> CmdSpawn on " + transform.GetHashCode());
    }

    private void OnTriggerStay(Collider other)
    {
        isBlocked = true;
    }
}
