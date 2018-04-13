using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Networking;
public class Health : NetworkBehaviour
{
    public const int maxHealth = 100;
    
    [SyncVar]
    public int currentHealth = maxHealth;
    public RectTransform HealthBar;

    public void TakeDamage(int amount)
    {
        if (!isServer)
        {
            return;
        }

        currentHealth -= amount;
        if (currentHealth <= 0)
        {
            currentHealth = 0;
            Debug.Log("Dead!");
        }

        HealthBar.sizeDelta = new Vector2(currentHealth, HealthBar.sizeDelta.y);
    }
}