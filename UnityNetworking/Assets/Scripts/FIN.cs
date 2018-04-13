using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public static class FIN
{
    /// <summary>
    /// Re-arrange of formula Vf^2 = Vi^2 + 2AD
    /// Which assumes Vf is 0 (at peak of projectile motion) 
    /// </summary>
    /// <param name="D">Peak height displacement</param>
    /// <returns>Magnitude of Viy to achieve height.</returns>
    public static float FindViForPeak(float D, bool isCharacter = false)
    {
        // Gravity is tripled for character
        float modifier = isCharacter ? 3 : 1;
        return Mathf.Sqrt(-2 * (modifier * Physics.gravity.y) * Mathf.Abs(D));
    }

    public static float findAtoStopFromVByD(float V, float D)
    {
        return -(V * V) / (2 * D);
    }

    public static Vector3 FindAccelerationToStopAt(Vector3 velocity, Vector3 displacement)
    {
        float x = findAtoStopFromVByD(velocity.x, displacement.x);
        float y = findAtoStopFromVByD(velocity.y, displacement.y);
        float z = findAtoStopFromVByD(velocity.z, displacement.z);

        return new Vector3(x, y, z);
    }

    /// <summary>
    /// With constant acceleration, find the time to cover displacement
    /// given Vi and with Vf == 0
    /// </summary>
    /// <param name="D">Maximum vertical displacement</param>
    /// <param name="Vi">Initial vertical velocity</param>
    /// <returns>Time to reach peak height</returns>
    public static float FindPeakTime(float D, float Vi)
    {
        return (2 * D) / Vi;
    }

    /// <summary>
    /// Assuming constant acceleration, find velocity
    /// using displacement and time
    /// </summary>
    /// <param name="D">Displacement</param>
    /// <param name="T">Time</param>
    /// <returns>Velocity</returns>
    public static float FindV(float D, float T)
    {
        return D / T;
    }

    /// <summary>
    /// Find determinant of the quadratic formula
    /// x = (-b +- sqrt(b^2 - 4ac))/2a
    /// </summary>
    /// <param name="B">Linear component coefficient</param>
    /// <param name="A">Quadratic component coefficient</param>
    /// <param name="C">Constant</param>
    /// <returns>Quadratic Determinant</returns>
    public static float FindDet(float A, float B, float C)
    {
        return (B * B) - (4 * A * C);
    }

    public static bool IsNegative(float x)
    {
        return x < 0;
    }

    /// <summary>
    /// Solves quadratic formula x = (-b +- sqrt(b^2 - 4ac))/2a
    /// </summary>
    /// <param name="B">Linear component coefficient</param>
    /// <param name="A">Quadratic component coefficient</param>
    /// <param name="C">Constant</param>
    /// <returns>Vector 2 holding roots, or empty vector 2 in case of no roots.</returns>
    public static Vector2 SolveQuadratic(float A, float B, float C)
    {
        float det = FindDet(A, B, C);

        if (!IsNegative(det))
        {
            float X1;
            float X2;
            X1 = -(B + Mathf.Sqrt(det)) / (2 * A);
            X2 = (-B + Mathf.Sqrt(det)) / (2 * A);
            return new Vector2(X1, X2);
        }

        return new Vector2();
    }

    /// <summary>
    /// Pick the better of two options above a certain benchmark time.
    /// </summary>
    /// <param name="choices">Vector 2 holding two float values</param>
    /// <param name="benchmark">Minimum reachable time</param>
    /// <returns>Smallest value larger than benchmark, or infinity</returns>
    public static float PickAppropriateTime(Vector2 choices, float benchmark = 0)
    {
        float lesser;
        float greater;

        if (choices.x <= choices.y)
        {
            lesser = choices.x;
            greater = choices.y;
        }
        else
        {
            lesser = choices.y;
            greater = choices.x;
        }

        if(lesser > benchmark)
        {
            return lesser;
        }
        else
        {
            return greater >= benchmark ? greater : Mathf.Infinity;
        }
    }

    /// <summary>
    /// Given a peak height and a displacement from currect height, find the time at which 
    /// the projectile will land at the correct displacement having risen to the peak already.
    /// </summary>
    /// <param name="peakHeight">Maximum vertical displacement</param>
    /// <param name="heightDisplacement">Final vertical displacement</param>
    /// <returns></returns>
    public static float FindLandingTime(float peakHeight, float heightDisplacement)
    {
        float Viy = FindViForPeak(peakHeight);
        float peakTime = FindPeakTime(peakHeight, Viy);

        Debug.Log("Peak Height: " + peakHeight);

        // Re-arrange D = ViT + (1/2)AT^2 into Quadratic Form: 0 = (1/2)AT^2 + ViT - D
        Vector2 timeOptions = SolveQuadratic((0.5f * Physics.gravity.y), Viy, -heightDisplacement);

        return timeOptions.x;

        //if (timeOptions.Equals(new Vector2()))
        //{
        //    return Mathf.Infinity;
        //}
        //else
        //{
        //    return PickAppropriateTime(timeOptions, peakTime);
        //}
    }

    /// <summary>
    /// Find initial velocity required to hit a moving platform
    /// (as long as relative Y velocity is 0)
    /// and assuming constant relative velocity.
    /// </summary>
    /// <param name="startingPosition">Position our projectile begins at</param>
    /// <param name="finalPosition">Target for our projectile</param>
    /// <param name="relativeVelocity">Use Vector.zero if both starting and ending positions are stationary</param>
    /// <returns>Muzzle velocity to arrive at destination</returns>
    public static Vector3 FindTrajectoryVelocity(Vector3 startingPosition, Vector3 finalPosition, Vector3 relativeVelocity, float arc = 3.5f)
    {
        // Correct for weird arc values
        if (arc < 1)
        {
            arc = 1;
        }

        Debug.Log(arc);

        Vector3 displacement = finalPosition - startingPosition;
        float arcMod = displacement.y <= -5 ? arc/11 : arc; // heuristic for how arced the flight path should be

        float peakHeight = Mathf.Abs(displacement.y) < 1 ? 1 * arcMod : Mathf.Abs(displacement.y * arcMod);

        float time = FindLandingTime(peakHeight, displacement.y);
        float Viy = FindViForPeak(peakHeight);

        Debug.Log(Viy);

        //Debug.Log(time);
        float Vix = FindV(displacement.x + (relativeVelocity.x * time), time);
        float Viz = FindV(displacement.z + (relativeVelocity.z * time), time);

        return new Vector3(Vix, Viy, Viz);
    }
}
