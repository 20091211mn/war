using System.Collections.Generic;
using UnityEngine;

// يبني العالم كله بالكود: أرض ومبانٍ وبوتات واجهة HUD (OnGUI) وأزرار لمس
public class Game : MonoBehaviour
{
    public static Game I;
    public int[] score = new int[2]; public int limit = 30;
    public LayerMask worldMask = ~0;
    Player player; List<Fighter> all = new List<Fighter>(); List<Vector3> boxes = new List<Vector3>();
    float msgUntil; string msg = ""; float hitMark;
    // أدوات لمس
    int stickId = -1, lookId = -1; Vector2 stickOrigin, lookLast;

    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
    static void Boot() { if (FindObjectOfType<Game>() == null) new GameObject("Game").AddComponent<Game>(); }

    void Awake()
    {
        I = this; Application.targetFrameRate = 60; QualitySettings.vSyncCount = 0;
        RenderSettings.ambientLight = new Color(0.55f, 0.57f, 0.62f); RenderSettings.fog = true; RenderSettings.fogColor = new Color(0.72f, 0.78f, 0.86f); RenderSettings.fogDensity = 0.006f;
        foreach (var c in FindObjectsOfType<Camera>()) Destroy(c.gameObject);
        foreach (var l in FindObjectsOfType<Light>()) Destroy(l.gameObject);
        var sun = new GameObject("Sun").AddComponent<Light>(); sun.type = LightType.Directional; sun.transform.rotation = Quaternion.Euler(50, -30, 0); sun.intensity = 1.1f; sun.shadows = LightShadows.Soft;
        Build();
    }

    void Build()
    {
        var g = GameObject.CreatePrimitive(PrimitiveType.Cube); g.name = "Ground"; g.transform.position = new Vector3(0, -0.5f, 0); g.transform.localScale = new Vector3(160, 1, 160);
        g.GetComponent<Renderer>().material.color = new Color(0.62f, 0.55f, 0.42f);
        var rnd = new System.Random(7);
        for (int i = 0; i < 26; i++)
        {
            float x = (float)(rnd.NextDouble() * 120 - 60), z = (float)(rnd.NextDouble() * 120 - 60);
            if (Mathf.Abs(x) < 8 && Mathf.Abs(z) < 8) continue;
            float w = 4 + (float)rnd.NextDouble() * 9, d = 4 + (float)rnd.NextDouble() * 9, h = 2.5f + (float)rnd.NextDouble() * 8;
            var b = GameObject.CreatePrimitive(PrimitiveType.Cube); b.transform.position = new Vector3(x, h / 2, z); b.transform.localScale = new Vector3(w, h, d);
            float k = 0.45f + (float)rnd.NextDouble() * 0.3f; b.GetComponent<Renderer>().material.color = new Color(k, k * 0.95f, k * 0.85f);
            boxes.Add(new Vector3(x, 0, z));
        }
        // جدران الحدود
        for (int i = 0; i < 4; i++)
        {
            var w = GameObject.CreatePrimitive(PrimitiveType.Cube); bool ns = i < 2; float s = (i % 2 == 0 ? 1 : -1) * 80;
            w.transform.position = ns ? new Vector3(0, 3, s) : new Vector3(s, 3, 0); w.transform.localScale = ns ? new Vector3(162, 6, 1) : new Vector3(1, 6, 162);
            w.GetComponent<Renderer>().material.color = new Color(0.35f, 0.33f, 0.3f);
        }
        var pg = new GameObject("Player"); pg.transform.position = SpawnPoint(1); player = pg.AddComponent<Player>(); player.team = 1; all.Add(player);
        for (int i = 0; i < 8; i++) AddBot(i % 2 == 0 ? 0 : 1);
        for (int i = 0; i < 2; i++) AddBot(0);
    }
    void AddBot(int t) { var go = new GameObject("Bot"); go.transform.position = SpawnPoint(t); var b = go.AddComponent<Bot>(); b.Init(t); all.Add(b); }

    public Vector3 SpawnPoint(int t)
    {
        for (int tries = 0; tries < 30; tries++)
        {
            var p = new Vector3(Random.Range(-70f, 70f), 0.1f, t == 0 ? Random.Range(40f, 72f) : Random.Range(-72f, -40f));
            if (!Physics.CheckSphere(p + Vector3.up * 1f, 1.2f)) return p;
        }
        return new Vector3(0, 0.1f, t == 0 ? 60 : -60);
    }
    public Fighter NearestEnemy(Fighter me)
    {
        Fighter best = null; float bd = 1e9f;
        foreach (var f in all) { if (f == null || f.team == me.team || !f.Alive) continue; float d = (f.transform.position - me.transform.position).sqrMagnitude; if (d < bd) { bd = d; best = f; } }
        return best;
    }
    public void Shoot(Fighter shooter, Vector3 o, Vector3 d, float dmg, float range)
    {
        Debug.DrawRay(o, d * Mathf.Min(range, 40f), shooter is Player ? Color.yellow : Color.red, 0.1f);
        var hits = Physics.RaycastAll(o, d, range); System.Array.Sort(hits, (a, b) => a.distance.CompareTo(b.distance));
        foreach (var h in hits)
        {
            if (h.collider.transform.IsChildOf(shooter.transform)) continue;
            Fighter f = h.collider.GetComponentInParent<Fighter>();
            var p = h.collider.GetComponent<HitProxy>(); if (p != null) f = p.owner;
            if (f != null) { if (f.team != shooter.team) { f.TakeDamage(dmg, shooter); if (shooter is Player) hitMark = 0.15f; } }
            break;
        }
    }

    void Update()
    {
        hitMark = Mathf.Max(0, hitMark - Time.deltaTime);
        if (score[0] >= limit || score[1] >= limit) { msg = score[1] >= limit ? "فاز الفريق الأزرق" : "فاز الفريق الأحمر"; msgUntil = Time.time + 5; score[0] = score[1] = 0; foreach (var f in all) { f.kills = f.deaths = 0; } }
        HandleTouch();
    }

    // لمس: النصف الأيسر عصا حركة، الأيمن سحب للنظر، وزرّان: إطلاق وتعبئة
    Rect FireRect() => new Rect(Screen.width - 220, Screen.height - 260, 170, 170);
    Rect ReloadRect() => new Rect(Screen.width - 130, Screen.height - 400, 100, 100);
    void HandleTouch()
    {
        if (player == null || !Application.isMobilePlatform) return;
        bool fire = false;
        foreach (var t in Input.touches)
        {
            Vector2 p = new Vector2(t.position.x, Screen.height - t.position.y);
            if (t.phase == TouchPhase.Began)
            {
                if (ReloadRect().Contains(p)) { player.Reload(); continue; }
                if (FireRect().Contains(p)) continue;
                if (p.x < Screen.width * 0.45f && stickId < 0) { stickId = t.fingerId; stickOrigin = p; }
                else if (lookId < 0) { lookId = t.fingerId; lookLast = p; }
            }
            if (t.fingerId == stickId) { if (t.phase == TouchPhase.Ended || t.phase == TouchPhase.Canceled) { stickId = -1; player.touchMove = Vector2.zero; } else { Vector2 d = (p - stickOrigin) / 110f; player.touchMove = Vector2.ClampMagnitude(new Vector2(d.x, -d.y), 1f); } }
            else if (t.fingerId == lookId) { if (t.phase == TouchPhase.Ended || t.phase == TouchPhase.Canceled) lookId = -1; else { player.touchLook += (p - lookLast) * new Vector2(1f, -1f) * 0.6f; lookLast = p; } }
            else if (t.phase != TouchPhase.Ended && FireRect().Contains(p)) fire = true;
        }
        player.touchFire = fire;
    }

    GUIStyle st;
    void OnGUI()
    {
        if (player == null) return;
        if (st == null) { st = new GUIStyle(GUI.skin.label) { fontSize = Mathf.Max(18, Screen.height / 32), fontStyle = FontStyle.Bold }; st.normal.textColor = Color.white; }
        float cx = Screen.width / 2f, cy = Screen.height / 2f;
        Color c = hitMark > 0 ? Color.red : Color.white; var old = GUI.color; GUI.color = c;
        GUI.DrawTexture(new Rect(cx - 1, cy - 10, 2, 7), Texture2D.whiteTexture); GUI.DrawTexture(new Rect(cx - 1, cy + 3, 2, 7), Texture2D.whiteTexture);
        GUI.DrawTexture(new Rect(cx - 10, cy - 1, 7, 2), Texture2D.whiteTexture); GUI.DrawTexture(new Rect(cx + 3, cy - 1, 7, 2), Texture2D.whiteTexture); GUI.color = old;
        GUI.Label(new Rect(20, 10, 600, 40), "RED " + score[0] + "  -  BLUE " + score[1] + "   (limit " + limit + ")", st);
        GUI.Label(new Rect(20, Screen.height - 60, 600, 40), "HP " + Mathf.CeilToInt(Mathf.Max(0, player.hp)) + "     " + (player.Reloading ? "RELOADING" : player.ammo + " / " + player.reserve), st);
        if (!player.Alive) GUI.Label(new Rect(cx - 120, cy - 20, 400, 40), "YOU DIED - respawning...", st);
        if (Time.time < msgUntil) GUI.Label(new Rect(cx - 150, 80, 500, 40), msg, st);
        if (Application.isMobilePlatform)
        {
            GUI.color = new Color(1, 1, 1, 0.35f); GUI.Box(FireRect(), "FIRE"); GUI.Box(ReloadRect(), "R"); GUI.color = old;
            if (stickId >= 0) { GUI.color = new Color(1, 1, 1, 0.3f); GUI.DrawTexture(new Rect(stickOrigin.x - 55, stickOrigin.y - 55, 110, 110), Texture2D.whiteTexture); GUI.color = old; }
        }
        else if (Cursor.lockState != CursorLockMode.Locked) GUI.Label(new Rect(cx - 220, cy + 60, 600, 40), "Click to play  |  WASD move, Shift run, Space jump, R reload", st);
    }
}
