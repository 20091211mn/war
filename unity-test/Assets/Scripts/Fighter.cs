using UnityEngine;

// أساس اللاعب والبوتات: فريق وصحة وضرر
public abstract class Fighter : MonoBehaviour
{
    public int team;          // 0 أحمر، 1 أزرق
    public float hp = 100f;
    public int kills, deaths;
    public bool Alive => hp > 0f;

    public void TakeDamage(float d, Fighter src)
    {
        if (!Alive) return;
        hp -= d;
        if (hp <= 0f)
        {
            deaths++;
            if (src != null && src != this) { src.kills++; Game.I.score[src.team]++; }
            OnDeath();
        }
    }
    protected abstract void OnDeath();
}
