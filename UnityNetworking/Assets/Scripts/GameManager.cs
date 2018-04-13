using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GameManager : MonoBehaviour {
    private void Awake()
    {
        SwitchBoard.gm = this;
    }

    private void OnDisable()
    {
        SwitchBoard.gm = null;
    }
}
