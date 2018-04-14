using System.Collections;
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

    //private Dictionary<int, PlayerController> players = new Dictionary<int, PlayerController>();

    private void Awake()
    {
        SwitchBoard.gm = this;
    }

    private void OnDisable()
    {
        SwitchBoard.gm = null;
    }

    public void Register(PlayerController pc)
    {
        if (pc.isLocalPlayer && !scoresContainID(pc.playerControllerId))
        {
            Score registrantScore = new Score(pc.playerControllerId);
            CmdRegister(registrantScore);
        }
    }

    public void Deregister(PlayerController pc)
    {
        Score registrantScore = new Score();
        if (pc.isLocalPlayer && scoresContainID(pc.playerControllerId, registrantScore))
        {
            CmdRegister(registrantScore);
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

    //public PlayerController GetPC(int id)
    //{
    //    PlayerController pc = null;
    //    players.TryGetValue(id, out pc);
    //
    //    return pc;
    //}

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

    private bool scoresContainID(int id, Score result)
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
}
