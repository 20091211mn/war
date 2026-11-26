using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

// يبني العالم كله بالكود: أرض ومبانٍ وبوتات واجهة HUD (OnGUI) وأزرار لمس
public class Game : MonoBehaviour
{
    public const string Version = "0.2.0";
    const string UpdateUrl = "https://raw.githubusercontent.com/20091211mn/war/main/unity-test/version.json";
    public static Game I;
    string updMsg = "", updLink = "";
    List<KeyValuePair<string, float>> feed = new List<KeyValuePair<string, float>>();
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
        RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Flat; RenderSettings.ambientLight = new Color(0.5f, 0.52f, 0.56f);
        RenderSettings.fog = true; RenderSettings.fogMode = FogMode.Linear; RenderSettings.fogColor = new Color(0.62f, 0.76f, 0.92f); RenderSettings.fogStartDistance = 70f; RenderSettings.fogEndDistance = 260f;
        foreach (var c in FindObjectsOfType<Camera>()) Destroy(c.gameObject);
        foreach (var l in FindObjectsOfType<Light>()) Destroy(l.gameObject);
        var sun = new GameObject("Sun").AddComponent<Light>(); sun.type = LightType.Directional; sun.transform.rotation = Quaternion.Euler(50, -30, 0); sun.intensity = 1.1f; sun.shadows = LightShadows.Soft;
        Build();
        StartCoroutine(CheckUpdate());
    }

    public void Feed(string t) { feed.Add(new KeyValuePair<string, float>(t, Time.time + 5f)); if (feed.Count > 5) feed.RemoveAt(0); }

    // تحديث: يقرأ version.json من GitHub؛ إن كان الإصدار أحدث يظهر شريط تحديث يفتح رابط التنزيل
    [System.Serializable] class VInfo { public string version; public string url; public string notes; }
    IEnumerator CheckUpdate()
    {
        yield return new WaitForSeconds(2f);
        using (var rq = UnityWebRequest.Get(UpdateUrl + "?t=" + (int)Time.realtimeSinceStartup))
        {
            rq.timeout = 8; yield return rq.SendWebRequest();
            if (rq.result != UnityWebRequest.Result.Success) yield break;
            VInfo v = null; try { v = JsonUtility.FromJson<VInfo>(rq.downloadHandler.text); } catch { }
            if (v == null || string.IsNullOrEmpty(v.version) || !IsNewer(v.version, Version)) yield break;
            updMsg = "Update " + v.version + (string.IsNullOrEmpty(v.notes) ? "" : ": " + v.notes); updLink = v.url;
        }
    }
    static bool IsNewer(string a, string b)
    {
        var x = a.Split('.'); var y = b.Split('.');
        for (int i = 0; i < 3; i++) { int p, q; int.TryParse(i < x.Length ? x[i] : "0", out p); int.TryParse(i < y.Length ? y[i] : "0", out q); if (p != q) return p > q; }
        return false;
    }

    void Build()
    {
        var g = GameObject.CreatePrimitive(PrimitiveType.Cube); g.name = "Ground"; g.transform.position = new Vector3(0, -0.5f, 0); g.transform.localScale = new Vector3(160, 1, 160);
        Mat.Set(g, new Color(0.62f, 0.55f, 0.42f));
        var rnd = new System.Random(7);
        for (int i = 0; i < 26; i++)
        {
            float x = (float)(rnd.NextDouble() * 120 - 60), z = (float)(rnd.NextDouble() * 120 - 60);
            if (Mathf.Abs(x) < 8 && Mathf.Abs(z) < 8) continue;
            float w = 4 + (float)rnd.NextDouble() * 9, d = 4 + (float)rnd.NextDouble() * 9, h = 2.5f + (float)rnd.NextDouble() * 8;
            var b = GameObject.CreatePrimitive(PrimitiveType.Cube); b.transform.position = new Vector3(x, h / 2, z); b.transform.localScale = new Vector3(w, h, d);
            float k = 0.45f + (float)rnd.NextDouble() * 0.3f; Mat.Set(b, new Color(k, k * 0.95f, k * 0.85f));
            boxes.Add(new Vector3(x, 0, z));
        }
        // جدران الحدود
        for (int i = 0; i < 4; i++)
        {
            var w = GameObject.CreatePrimitive(PrimitiveType.Cube); bool ns = i < 2; float s = (i % 2 == 0 ? 1 : -1) * 80;
            w.transform.position = ns ? new Vector3(0, 3, s) : new Vector3(s, 3, 0); w.transform.localScale = ns ? new Vector3(162, 6, 1) : new Vector3(1, 6, 162);
            Mat.Set(w, new Color(0.35f, 0.33f, 0.3f));
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
    Rect SwitchRect() => new Rect(Screen.width - 250, Screen.height - 400, 100, 100);
    Rect UpdRect() => new Rect(Screen.width / 2f - 260, 70, 520, 70);
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
                if (updLink != "" && UpdRect().Contains(p)) { Application.OpenURL(updLink); continue; }
                if (SwitchRect().Contains(p)) { player.NextWeapon(); continue; }
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

    GUIStyle st, sm;
    void OnGUI()
    {
        if (player == null) return;
        int fs = Mathf.Max(18, Screen.height / 36);
        if (st == null || st.fontSize != fs)
        {
            st = new GUIStyle(GUI.skin.label) { fontSize = fs, fontStyle = FontStyle.Bold, wordWrap = false }; st.normal.textColor = Color.white;
            sm = new GUIStyle(st) { fontSize = Mathf.RoundToInt(fs * 0.8f) };
        }
        float lh = fs * 1.7f, cx = Screen.width / 2f, cy = Screen.height / 2f;
        var old = GUI.color;
        // وميض الضرر
        float hurt = Mathf.Clamp01(1f - (Time.time - player.lastHit) / 0.4f);
        if (hurt > 0) { GUI.color = new Color(1, 0, 0, hurt * 0.35f); GUI.DrawTexture(new Rect(0, 0, Screen.width, Screen.height), Texture2D.whiteTexture); }
        GUI.color = hitMark > 0 ? Color.red : Color.white;
        GUI.DrawTexture(new Rect(cx - 1, cy - 10, 2, 7), Texture2D.whiteTexture); GUI.DrawTexture(new Rect(cx - 1, cy + 3, 2, 7), Texture2D.whiteTexture);
        GUI.DrawTexture(new Rect(cx - 10, cy - 1, 7, 2), Texture2D.whiteTexture); GUI.DrawTexture(new Rect(cx + 3, cy - 1, 7, 2), Texture2D.whiteTexture); GUI.color = old;
        GUI.Label(new Rect(20, 12, Screen.width * 0.6f, lh), "RED " + score[0] + "  -  BLUE " + score[1] + "   (" + limit + ")", st);
        GUI.Label(new Rect(20, Screen.height - lh * 2.1f, Screen.width * 0.6f, lh), "HP " + Mathf.CeilToInt(Mathf.Max(0, player.hp)), st);
        GUI.Label(new Rect(20, Screen.height - lh * 1.1f, Screen.width * 0.7f, lh), player.WeaponName + "   " + (player.Reloading ? "RELOADING" : player.ammo + " / " + player.reserve), st);
        // سجل القتل
        for (int i = feed.Count - 1; i >= 0; i--) if (feed[i].Value < Time.time) feed.RemoveAt(i);
        for (int i = 0; i < feed.Count; i++) GUI.Label(new Rect(Screen.width - Screen.width * 0.4f - 20, 12 + i * lh * 0.8f, Screen.width * 0.4f, lh), feed[i].Key, new GUIStyle(sm) { alignment = TextAnchor.MiddleRight, normal = { textColor = Color.white } });
        if (!player.Alive) GUI.Label(new Rect(cx - 200, cy - lh, 400, lh * 1.5f), "YOU DIED - respawning...", st);
        if (Time.time < msgUntil) GUI.Label(new Rect(cx - 200, 12 + lh, 500, lh), msg, st);
        GUI.Label(new Rect(Screen.width - 120, Screen.height - lh, 110, lh), "v" + Version, sm);
        if (updLink != "")
        {
            var r = UpdRect(); GUI.color = new Color(0.1f, 0.5f, 0.2f, 0.9f); GUI.DrawTexture(r, Texture2D.whiteTexture); GUI.color = old;
            GUI.Label(new Rect(r.x + 12, r.y + 10, r.width - 24, r.height - 20), updMsg + "  (tap)", sm);
            if (!Application.isMobilePlatform && Event.current.type == EventType.MouseDown && r.Contains(Event.current.mousePosition)) Application.OpenURL(updLink);
        }
        if (Application.isMobilePlatform)
        {
            GUI.color = new Color(1, 1, 1, 0.35f); GUI.Box(FireRect(), "FIRE"); GUI.Box(ReloadRect(), "R"); GUI.Box(SwitchRect(), "SW"); GUI.color = old;
            if (stickId >= 0) { GUI.color = new Color(1, 1, 1, 0.3f); GUI.DrawTexture(new Rect(stickOrigin.x - 55, stickOrigin.y - 55, 110, 110), Texture2D.whiteTexture); GUI.color = old; }
        }
        else if (Cursor.lockState != CursorLockMode.Locked) GUI.Label(new Rect(cx - 330, cy + 60, 700, lh * 2), "Click to play | WASD, Shift, Space | 1-3 / Q weapons | R reload", st);
    }
}
