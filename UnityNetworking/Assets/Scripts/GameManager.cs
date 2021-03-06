﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class GameManager : NetworkBehaviour {

    public struct Score
    {
        public int id;
        public int score;

        public Score(int i, int s)
        {
            id = i;
            score = s;
        }

        public Score(int i)
        {
            id = i;
            score = 0;
        }

        public bool Equals(Score s)
        {
            return s == this;
        }

        public bool Equals(int i)
        {
            return id == i;
        }

        public override bool Equals(object obj)
        {
            return base.Equals(obj);
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public override string ToString()
        {
            return id + " :: " + score;
        }

        public static bool operator ==(Score s1, Score s2)
        {
            return s1.id == s2.id;
        }

        public static bool operator !=(Score s1, Score s2)
        {
            return s1.id == s2.id;
        }
    }

    public class SyncScoreList : SyncListStruct<Score>
    { }

    [HideInInspector]
    public SyncScoreList playerScores = new SyncScoreList();

    [SyncVar]
    public int playerWithFlag;
    private Dictionary<int, PlayerController> players = new Dictionary<int, PlayerController>();

    // Delegate setups
    public delegate void ScoreChange();
    public static event ScoreChange ScoreAction;

    private int roomCount = 1;
    private List<RoomController> roomList = new List<RoomController>();

    [HideInInspector]
    public GameObject flagRef;
    [HideInInspector]
    public GameObject flagStandRef;

    [SerializeField]
    [Range(5, 10)]
    private int flagRoomMin = 8;
    [SerializeField]
    [Range(11, 16)]
    private int flagRoomMax = 13;

    private int flagRoom;

    private void Awake()
    {
        Debug.Log("Gamemanager is awake");
        SwitchBoard.gm = this;

        Debug.Log(SwitchBoard.gm == this ? "Switchboard.gm assigned correctly" : "Switchboard.gm ASSIGNED WRONG");
    }

    private void Start()
    {
        flagRoom = Random.Range(flagRoomMin, flagRoomMax);
    }

    private void OnDisable()
    {
        SwitchBoard.gm = null;
    }

    [Server]
    public void Register(RoomController rc)
    {
        roomCount++;
        Debug.Log("Registering room: " + roomCount);
        rc.gameObject.name = "Room" + roomCount;

        roomList.Add(rc);

        if (roomCount == flagRoom)
        {
            Debug.Log("Spawning flag stuff!");
            rc.SpawnFlag();
            GetRandomRoom().SpawnFlagstand();
            RpcFindFlags();
        }
    }

    [Server]
    private RoomController GetRandomRoom()
    {
        int ridx = Random.Range(0, roomList.Count);
        Debug.Log("room index: " + ridx);
        return roomList[ridx];
    }

    [ClientRpc]
    private void RpcFindFlags()
    {
        flagRef = GameObject.FindGameObjectWithTag("Flag");
        flagStandRef = GameObject.FindGameObjectWithTag("FlagStand");
    }

    public void Register(PlayerController pc)
    {
        if (!scoresContainID((int)pc.netId.Value))
        {
            players.Add((int)pc.netId.Value, pc);
            if(pc.isLocalPlayer)
            { 
                Score registrantScore = new Score((int)pc.netId.Value);
                CmdRegister(registrantScore);
            }
        }
    }

    public void Deregister(PlayerController pc)
    {
        Score registrantScore = new Score();
        if (pc.isLocalPlayer && scoresContainID((int)pc.netId.Value, out registrantScore))
        {
            players.Remove((int)pc.netId.Value);
            CmdDeregister(registrantScore);
        }
    }

    [Command]
    private void CmdRegister(Score pc)
    {
        int id = pc.id;

        if (!scoresContainID(id))
        {
            playerScores.Add(new Score(id));
        }
    }

    [Command]
    private void CmdDeregister(Score pc)
    {
        int scoreIndex = findScoreByID(pc.id);

        if (scoreIndex != -1)
        {
            playerScores.RemoveAt(scoreIndex);
        }
    }

    private bool scoresContainID(int id)
    {
        bool found = false;
        foreach (Score s in playerScores)
        {
            found |= s.Equals(id);
            if (found)
            {
                break;
            }
        }

        return found;
    }

    private bool scoresContainID(int id, out Score result)
    {
        bool found = false;
        foreach (Score s in playerScores)
        {
            found |= s.Equals(id);
            if (found)
            {
                result = s;
                break;
            }
        }

        result = new Score(id);
        return found;
    }

    /// <summary>
    /// Searches playerScores for a given Score id.
    /// </summary>
    /// <param name="id">The Score id to search for.</param>
    /// <returns>Index of score, or -1 if not found</returns>
    private int findScoreByID(int id)
    {
        for(int i = 0; i < playerScores.Count; i++)
        {
            if (playerScores[i].Equals(id))
            {
                return i;
            }
        }

        return -1;
    }

    [Server]
    public void FlagHolderUpdate(PlayerController pc, Transform flagTransform)
    {
        Debug.Log("<< FlagHolderUpdate");
        playerWithFlag = (int)pc.netId.Value;
        Debug.Log("Player with flag: " + playerWithFlag);
        PlayerController registeredPC;
        if(players.TryGetValue(playerWithFlag, out registeredPC))
        {
            Debug.Log(playerWithFlag + " was registered");

            registeredPC.pickupObject(flagRef.transform);
            registeredPC.RpcPickupFlag();

            RpcTurnOffFlagParticles();
        }
        else
        {
            Debug.Log(playerWithFlag + " was not registered");
        }

        Debug.Log(">> FlagHolderUpdate");
    }

    [Command]
    public void CmdFlagHolderUpdate()
    {
        playerWithFlag = -1;
    }

    [ClientRpc]
    private void RpcTurnOffFlagParticles()
    {
        //Debug.Log("gm " + transform.GetHashCode() + " turning off particles : " +
        //    flagRef.GetComponent<ParticleSystem>().IsAlive());

        flagRef.GetComponent<ParticleSystem>().Stop();
    }

    public void PlayerScored()
    {
        Score newScore = new Score(playerWithFlag, 1);
        if (scoresContainID(playerWithFlag))
        {
            int idx = findScoreByID(playerWithFlag);
            newScore = new Score(playerScores[idx].id, playerScores[idx].score + 1);
            playerScores[idx] = newScore;

            Destroy(flagRef.gameObject);

            GetRandomRoom().SpawnFlag();
            RpcFindFlags();
        }
        else
        {
            Debug.LogWarning("Player " + playerWithFlag + " just scored and couldn't be found in score list");
            CmdRegister(newScore);
        }

        Debug.Log("Player " + playerWithFlag + " has scored!");
        Debug.Log("Current standings are now: ");
        foreach (Score s in playerScores)
        {
            Debug.Log(s.ToString());
        }

        if (ScoreAction != null)
        {
            ScoreAction();
        }
    }
}
