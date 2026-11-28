using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

// يبني العالم كله بالكود: أرض ومبانٍ وبوتات واجهة HUD (OnGUI) وأزرار لمس
public class Game : MonoBehaviour
{
    public const string Version = "1.0.6";
    const string UpdateUrl = "https://raw.githubusercontent.com/20091211mn/war/main/unity-test/version.json";
    public static Game I; Light sun; public Color skyCol = new Color(0.55f, 0.72f, 0.92f);
    string updMsg = "", updLink = "";
    List<KeyValuePair<string, float>> feed = new List<KeyValuePair<string, float>>();
    public int[] score = new int[2]; public int limit = 30;
    public LayerMask worldMask = ~0;
    Transform world; Renderer groundR; List<Renderer> bldR = new List<Renderer>(); string dbg = ""; float dbgT;
    Player player; public List<Fighter> all = new List<Fighter>(); List<Vector3> boxes = new List<Vector3>();
    float msgUntil; string msg = ""; float hitMark;
    // أدوات لمس
    int stickId = -1, lookId = -1; Vector2 stickOrigin, lookLast;

    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
    static void Boot() { if (FindObjectOfType<Game>() == null) new GameObject("Game").AddComponent<Game>(); }

    void Awake()
    {
        I = this; Application.targetFrameRate = 60; QualitySettings.vSyncCount = 0;
        RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Trilight; RenderSettings.ambientSkyColor = new Color(0.55f, 0.7f, 0.95f); RenderSettings.ambientEquatorColor = new Color(0.55f, 0.58f, 0.52f); RenderSettings.ambientGroundColor = new Color(0.32f, 0.28f, 0.22f);
        RenderSettings.fog = false;
        foreach (var c in FindObjectsOfType<Camera>()) Destroy(c.gameObject);
        foreach (var l in FindObjectsOfType<Light>()) Destroy(l.gameObject);
        sun = new GameObject("Sun").AddComponent<Light>(); sun.type = LightType.Directional; sun.transform.rotation = Quaternion.Euler(38, -35, 0); sun.intensity = 1.25f; sun.color = new Color(1f, 0.95f, 0.84f); sun.shadows = LightShadows.Soft;
        mapIdx = PlayerPrefs.GetInt("map", 0); botCount = PlayerPrefs.GetInt("bots", 10); lowQ = PlayerPrefs.GetInt("lowq", Application.isMobilePlatform ? 1 : 0) == 1; ApplyQuality();
        ShowMenu();
        StartCoroutine(CheckUpdate());
        Ads.Init();
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
        var pg = new GameObject("Player"); pg.transform.position = SpawnPoint(1); player = pg.AddComponent<Player>(); player.team = 1; all.Add(player);
        for (int i = 0; i < botCount; i++) AddBot(i % 2 == 0 ? 0 : 1);
        for (int i = 0; i < 2; i++) AddBot(0);
        paused = false; Cursor.lockState = CursorLockMode.None;
    }
    void BuildWorld()
    {
        if (world != null) Destroy(world.gameObject);
        world = new GameObject("World").transform;
        if (mapIdx == 0) { groundR = FarmMap.Build(world); bldR = FarmMap.Rends; skyCol = new Color(0.55f, 0.72f, 0.92f); }
        else { groundR = ArenaMap.Build(world, mapIdx); bldR = ArenaMap.Rends; skyCol = mapIdx == 1 ? new Color(0.72f, 0.82f, 0.95f) : mapIdx == 2 ? new Color(0.74f, 0.8f, 0.88f) : new Color(0.58f, 0.64f, 0.74f); }
        RenderSettings.ambientSkyColor = skyCol;
        if (menuCam != null) menuCam.backgroundColor = skyCol;
    }
    void ApplyQuality()
    {
        if (sun != null) sun.shadows = lowQ ? LightShadows.None : LightShadows.Soft;
        QualitySettings.shadowDistance = lowQ ? 0f : 90f; QualitySettings.pixelLightCount = lowQ ? 1 : 2;
    }

    // ---------- القائمة الرئيسية ----------
    static readonly string[] MapNames = { "FARM HARVEST", "DESERT OUTPOST", "SNOW BASE", "URBAN RUINS" };
    static readonly int[] BotOpts = { 6, 10, 16 };
    int mapIdx = 0, botCount = 10; bool paused, lowQ; Camera menuCam; float menuT;
    void ShowMenu()
    {
        if (player != null) { Destroy(player.gameObject); player = null; }
        foreach (var f in all) if (f != null) Destroy(f.gameObject);
        all.Clear(); score[0] = score[1] = 0;
        foreach (var c in FindObjectsOfType<Camera>()) Destroy(c.gameObject);
        BuildWorld();
        var cg = new GameObject("MenuCam"); menuCam = cg.AddComponent<Camera>(); cg.tag = "MainCamera"; menuCam.farClipPlane = 400f;
        menuCam.clearFlags = CameraClearFlags.SolidColor; menuCam.backgroundColor = skyCol;
        Cursor.lockState = CursorLockMode.None; Cursor.visible = true; paused = true;
    }
    void StartMatch()
    {
        if (menuCam != null) Destroy(menuCam.gameObject);
        menuCam = null; Build();
    }
    void MenuCamUpdate()
    {
        if (menuCam == null) return;
        menuT += Time.unscaledDeltaTime * 0.08f;
        float r = 45f; menuCam.transform.position = new Vector3(Mathf.Cos(menuT) * r, 9f, Mathf.Sin(menuT) * r);
        menuCam.transform.LookAt(new Vector3(0, 2f, 0));
    }
    GUIStyle titleSt, btnSt, subSt;
    void MenuGUI()
    {
        float W = Screen.width, H = Screen.height; var old = GUI.color;
        int fs = Mathf.Max(20, Mathf.RoundToInt(H / 24f));
        if (titleSt == null || btnSt.fontSize != fs)
        {
            titleSt = new GUIStyle(GUI.skin.label) { fontSize = Mathf.RoundToInt(fs * 3f), fontStyle = FontStyle.Bold, alignment = TextAnchor.MiddleCenter }; titleSt.normal.textColor = Color.white;
            subSt = new GUIStyle(GUI.skin.label) { fontSize = Mathf.RoundToInt(fs * 0.85f), alignment = TextAnchor.MiddleCenter }; subSt.normal.textColor = new Color(1, 1, 1, 0.8f);
            btnSt = new GUIStyle(GUI.skin.button) { fontSize = fs, fontStyle = FontStyle.Bold, alignment = TextAnchor.MiddleCenter };
            btnSt.normal.textColor = Color.white; btnSt.hover.textColor = Color.white; btnSt.active.textColor = Color.white;
        }
        GUI.color = new Color(0, 0, 0, 0.45f); GUI.DrawTexture(new Rect(0, 0, W, H), Texture2D.whiteTexture);
        GUI.color = new Color(0.85f, 0.18f, 0.18f, 0.95f); GUI.DrawTexture(new Rect(W * 0.5f - fs * 6.2f, H * 0.2f + fs * 3.2f, fs * 6.0f, fs * 0.18f), Texture2D.whiteTexture);
        GUI.color = new Color(0.2f, 0.4f, 0.9f, 0.95f); GUI.DrawTexture(new Rect(W * 0.5f + fs * 0.2f, H * 0.2f + fs * 3.2f, fs * 6.0f, fs * 0.18f), Texture2D.whiteTexture);
        GUI.color = Color.white;
        GUI.Label(new Rect(0, H * 0.2f, W, fs * 3.2f), "WARZONE", titleSt);
        GUI.Label(new Rect(0, H * 0.2f + fs * 3.5f, W, fs * 1.4f), "RED  vs  BLUE   -   Team Shooter", subSt);
        float bw = Mathf.Min(W * 0.6f, fs * 16f), bh = fs * 2.3f, x = (W - bw) / 2f, y = H * 0.2f + fs * 5.6f;
        GUI.backgroundColor = new Color(0.15f, 0.6f, 0.25f);
        if (GUI.Button(new Rect(x, y, bw, bh * 1.2f), "PLAY", btnSt)) StartMatch();
        GUI.backgroundColor = new Color(0.25f, 0.3f, 0.45f); y += bh * 1.35f;
        if (GUI.Button(new Rect(x, y, bw, bh), "MAP:  " + MapNames[mapIdx] + "  >", btnSt)) { mapIdx = (mapIdx + 1) % MapNames.Length; PlayerPrefs.SetInt("map", mapIdx); BuildWorld(); }
        y += bh * 1.15f;
        if (GUI.Button(new Rect(x, y, bw, bh), "BOTS:  " + botCount + "  >", btnSt)) { int k = System.Array.IndexOf(BotOpts, botCount); botCount = BotOpts[(k + 1) % BotOpts.Length]; PlayerPrefs.SetInt("bots", botCount); }
        y += bh * 1.2f;
        if (GUI.Button(new Rect(x, y, bw, bh), "GRAPHICS:  " + (lowQ ? "SMOOTH (low)" : "HIGH") + "  >", btnSt)) { lowQ = !lowQ; PlayerPrefs.SetInt("lowq", lowQ ? 1 : 0); ApplyQuality(); }
        GUI.backgroundColor = Color.white;
        GUI.Label(new Rect(0, H - fs * 2.2f, W, fs * 1.4f), "Controls: touch (stick left, look right) or WASD + mouse   |   v" + Version, subSt);
        if (updLink != "")
        {
            var r = new Rect(W / 2f - bw / 2f, H - fs * 4.6f, bw, fs * 2f); GUI.color = new Color(0.1f, 0.5f, 0.2f, 0.95f); GUI.DrawTexture(r, Texture2D.whiteTexture); GUI.color = Color.white;
            if (GUI.Button(r, updMsg + "  (tap)", btnSt)) Application.OpenURL(updLink);
        }
        GUI.color = old;
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
    // خط رؤية/عائق يتجاهل المقاتلين (كان Linecast يصطدم بمجسّم الهدف نفسه فيظنه حاجزاً فلا تطلق البوتات)
    static bool IsFighterHit(RaycastHit h) { return h.collider.GetComponentInParent<Fighter>() != null || h.collider.GetComponent<HitProxy>() != null; }
    public bool LineClear(Vector3 a, Vector3 b)
    {
        Vector3 d = b - a; float len = d.magnitude; if (len < 0.01f) return true;
        foreach (var h in Physics.RaycastAll(a, d / len, len, worldMask)) if (!IsFighterHit(h)) return false;
        return true;
    }
    public bool WallAhead(Vector3 o, Vector3 dir, float dist)
    {
        if (dir.sqrMagnitude < 1e-4f) return false;
        foreach (var h in Physics.RaycastAll(o, dir.normalized, dist, worldMask)) if (!IsFighterHit(h)) return true;
        return false;
    }
    // مؤثرات: وميض فوهة، خط مقذوف، شرارة اصطدام
    void Fx(Vector3 pos, Vector3 scale, Color c, float life, Quaternion rot)
    {
        var p = GameObject.CreatePrimitive(PrimitiveType.Cube); Destroy(p.GetComponent<Collider>());
        p.transform.position = pos; p.transform.rotation = rot; p.transform.localScale = scale; Mat.Set(p, c); Destroy(p, life);
    }
    void ShotFx(Vector3 o, Vector3 d, Vector3 end, bool hitFighter)
    {
        Fx(o + d * 0.9f, new Vector3(0.14f, 0.14f, 0.2f), new Color(1f, 0.85f, 0.4f), 0.04f, Quaternion.LookRotation(d));
        float len = Vector3.Distance(o + d, end); if (len > 1.5f) Fx((o + d + end) * 0.5f, new Vector3(0.025f, 0.025f, len), new Color(1f, 0.9f, 0.5f), 0.05f, Quaternion.LookRotation(d));
        Fx(end, hitFighter ? new Vector3(0.18f, 0.18f, 0.18f) : new Vector3(0.12f, 0.12f, 0.12f), hitFighter ? new Color(0.7f, 0.05f, 0.05f) : new Color(0.75f, 0.7f, 0.6f), 0.12f, Random.rotation);
    }
    float lastSnd;
    public void Shoot(Fighter shooter, Vector3 o, Vector3 d, float dmg, float range, int wid = 0)
    {
        if (Time.time - lastSnd > 0.02f || wid == 2) { lastSnd = Time.time; Snd.PlayShot(wid, o, shooter is Player ? 0.9f : 0.7f); }
        Debug.DrawRay(o, d * Mathf.Min(range, 40f), shooter is Player ? Color.yellow : Color.red, 0.1f);
        var hits = Physics.RaycastAll(o, d, range); System.Array.Sort(hits, (a, b) => a.distance.CompareTo(b.distance));
        Vector3 end = o + d * Mathf.Min(range, 60f); bool hitF = false;
        foreach (var h in hits)
        {
            if (h.collider.transform.IsChildOf(shooter.transform)) continue;
            Fighter f = h.collider.GetComponentInParent<Fighter>();
            var p = h.collider.GetComponent<HitProxy>(); if (p != null) f = p.owner;
            end = h.point;
            if (f != null) { hitF = true; if (f.team != shooter.team) { f.TakeDamage(dmg, shooter); if (shooter is Player) { hitMark = 0.15f; Snd.PlayHit(o); } } }
            break;
        }
        if (shooter is Player || (player != null && (player.transform.position - o).sqrMagnitude < 60f * 60f)) ShotFx(o, d, end, hitF);
    }

    void Update()
    {
        if (player == null) { MenuCamUpdate(); return; }
        hitMark = Mathf.Max(0, hitMark - Time.deltaTime);
        if (score[0] >= limit || score[1] >= limit) { msg = score[1] >= limit ? "فاز الفريق الأزرق" : "فاز الفريق الأحمر"; msgUntil = Time.time + 5; score[0] = score[1] = 0; Ads.RoundEnded(); foreach (var f in all) { f.kills = f.deaths = 0; } }
        HandleTouch();
    }

    // لمس: النصف الأيسر عصا حركة، الأيمن سحب للنظر، وزرّان: إطلاق وتعبئة
    Rect FireRect() => new Rect(Screen.width - 220, Screen.height - 260, 170, 170);
    Rect SwitchRect() => new Rect(Screen.width - 250, Screen.height - 400, 100, 100);
    Rect UpdRect() => new Rect(Screen.width / 2f - 260, 70, 520, 70);
    Rect PauseRect() { float pb = Mathf.Max(18, Screen.height / 36) * 2.2f; return new Rect(Screen.width / 2f - pb / 2f, 8, pb, pb); }
    Rect AimRect() => new Rect(Screen.width - 250, Screen.height - 520, 100, 100);
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
                if (PauseRect().Contains(p)) continue;
                if (SwitchRect().Contains(p)) { player.NextWeapon(); continue; }
                if (AimRect().Contains(p)) { player.ToggleAim(); continue; }
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
        if (player == null) { MenuGUI(); return; }
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
        { float bw2 = Screen.width * 0.22f, hp01 = Mathf.Clamp01(player.hp / 100f); GUI.color = new Color(0, 0, 0, 0.5f); GUI.DrawTexture(new Rect(20, Screen.height - lh * 2.55f, bw2, lh * 0.3f), Texture2D.whiteTexture); GUI.color = Color.Lerp(new Color(0.9f, 0.15f, 0.1f), new Color(0.2f, 0.85f, 0.3f), hp01); GUI.DrawTexture(new Rect(20, Screen.height - lh * 2.55f, bw2 * hp01, lh * 0.3f), Texture2D.whiteTexture); GUI.color = old; }
        GUI.Label(new Rect(20, Screen.height - lh * 2.1f, Screen.width * 0.6f, lh), "HP " + Mathf.CeilToInt(Mathf.Max(0, player.hp)) + "     K " + player.kills + "  D " + player.deaths, st);
        GUI.Label(new Rect(20, Screen.height - lh * 1.1f, Screen.width * 0.7f, lh), player.WeaponName + "   " + (player.Reloading ? "RELOADING" : player.ammo + " / " + player.reserve), st);
        // سجل القتل
        for (int i = feed.Count - 1; i >= 0; i--) if (feed[i].Value < Time.time) feed.RemoveAt(i);
        for (int i = 0; i < feed.Count; i++) GUI.Label(new Rect(Screen.width - Screen.width * 0.4f - 20, 12 + i * lh * 0.8f, Screen.width * 0.4f, lh), feed[i].Key, new GUIStyle(sm) { alignment = TextAnchor.MiddleRight, normal = { textColor = Color.white } });
        if (!player.Alive)
        {
            GUI.Label(new Rect(cx - 200, cy - lh, 400, lh * 1.5f), "YOU DIED - respawning...", st);
            if (Ads.RewardedReady && GUI.Button(new Rect(cx - 220, cy + lh * 0.6f, 440, lh * 1.6f), "WATCH AD: respawn now + full ammo", sm)) Ads.ShowRewarded(() => player.ReviveNow());
        }
        { float pb = fs * 2.2f; if (GUI.Button(new Rect(Screen.width / 2f - pb / 2f, 8, pb, pb), "II", sm)) { ShowMenu(); return; } }
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
            GUI.color = new Color(1, 1, 1, 0.35f); GUI.Box(FireRect(), "FIRE"); GUI.Box(ReloadRect(), "R"); GUI.Box(SwitchRect(), "SW"); GUI.Box(AimRect(), player.aim ? "AIM*" : "AIM"); GUI.color = old;
            if (stickId >= 0) { GUI.color = new Color(1, 1, 1, 0.3f); GUI.DrawTexture(new Rect(stickOrigin.x - 55, stickOrigin.y - 55, 110, 110), Texture2D.whiteTexture); GUI.color = old; }
        }
        else if (Cursor.lockState != CursorLockMode.Locked) GUI.Label(new Rect(cx - 330, cy + 60, 700, lh * 2), "Click to play | WASD, Shift, Space | 1-3 / Q weapons | R reload", st);
    }
}
