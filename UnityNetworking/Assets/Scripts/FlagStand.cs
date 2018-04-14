using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class FlagStand : MonoBehaviour {

    private GameManager gm;

    private void Start()
    {
        gm = GameObject.FindGameObjectWithTag("GameController").GetComponent<GameManager>();
    }

    private void OnTriggerEnter(Collider other)
    {
        if (other.CompareTag("Flag"))
        {
            gm.PlayerScored();
        }
    }
}
