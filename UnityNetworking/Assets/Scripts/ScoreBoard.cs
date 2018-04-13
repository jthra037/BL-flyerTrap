using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Score
{
    public int ID;
    public int Value;

    Score(int id)
    {
        ID = id;
    }
}


static class ScoreBoard {
    static public List<Score> Scores = new List<Score>();
}
