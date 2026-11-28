using UnityEngine;

// لاعب منظور أول: WASD + ماوس على الحاسوب، وعصا افتراضية + سحب + أزرار على الجوال. 3 أسلحة بمجسمات مختلفة (1-3 أو زر SW)، تصويب (AIM)، ارتداد وتأرجح وتعافٍ تدريجي
[RequireComponent(typeof(CharacterController))]
public class Player : Fighter
{
    public struct W { public string name; public float dmg, interval, spread, range, reload; public int mag, reserve, pellets; public float recoil; }
    public static readonly W[] Ws = {
        new W { name = "RIFLE",   dmg = 28, interval = 0.10f, spread = 0.012f, range = 150, reload = 1.6f, mag = 30, reserve = 120, pellets = 1, recoil = 0.45f },
        new W { name = "SMG",     dmg = 17, interval = 0.065f, spread = 0.030f, range = 90, reload = 1.4f, mag = 40, reserve = 160, pellets = 1, recoil = 0.3f },
        new W { name = "SHOTGUN", dmg = 12, interval = 0.85f, spread = 0.075f, range = 45, reload = 2.0f, mag = 6, reserve = 30, pellets = 8, recoil = 3.0f },
    };
    CharacterController cc; public Camera cam; Transform gun; Transform[] gunRoot = new Transform[3]; GameObject flash;
    float yaw, pitch, vy, nextShot, reloadEnd, reloadStart, respawnAt, bobT, stepT, flashEnd, kick;
    public int cur; public bool aim;
    int[] ammoA = new int[3], resA = new int[3];
    public int ammo => ammoA[cur]; public int reserve => resA[cur];
    public Vector2 touchMove; public bool touchFire; public Vector2 touchLook;
    public float lastHit = -10f;
    public string WeaponName => Ws[cur].name;
    static readonly Vector3 HipPos = new Vector3(0.25f, -0.22f, 0.5f), AimPos = new Vector3(0f, -0.15f, 0.42f);

    void Awake()
    {
        label = "YOU";
        cc = GetComponent<CharacterController>(); cc.height = 1.8f; cc.radius = 0.35f; cc.center = new Vector3(0, 0.9f, 0);
        var go = new GameObject("Cam"); go.transform.SetParent(transform); go.transform.localPosition = new Vector3(0, 1.6f, 0);
        cam = go.AddComponent<Camera>(); cam.nearClipPlane = 0.05f; cam.farClipPlane = 400f; go.tag = "MainCamera"; cam.fieldOfView = 70f;
        cam.clearFlags = CameraClearFlags.SolidColor; cam.backgroundColor = Game.I != null ? Game.I.skyCol : new Color(0.55f, 0.72f, 0.92f);
        go.AddComponent<AudioListener>();
        var g = new GameObject("Gun"); g.transform.SetParent(go.transform); g.transform.localPosition = HipPos; gun = g.transform;
        BuildGuns();
        for (int i = 0; i < 3; i++) { ammoA[i] = Ws[i].mag; resA[i] = Ws[i].reserve; }
        Equip(0);
    }
    void Part(int w, PrimitiveType pt, Vector3 pos, Vector3 scale, Color c, float rx = 0f, float rz = 0f)
    {
        var q = GameObject.CreatePrimitive(pt); Destroy(q.GetComponent<Collider>());
        q.transform.SetParent(gunRoot[w]); q.transform.localPosition = pos; q.transform.localScale = scale; q.transform.localRotation = Quaternion.Euler(rx, 0, rz); Mat.Set(q, c);
    }
    void BuildGuns()
    {
        Color dk = new Color(0.09f, 0.09f, 0.11f), mt = new Color(0.22f, 0.23f, 0.26f), lt = new Color(0.3f, 0.31f, 0.34f), tan = new Color(0.42f, 0.36f, 0.26f), wood = new Color(0.4f, 0.26f, 0.14f);
        for (int i = 0; i < 3; i++) { var r = new GameObject("g" + i).transform; r.SetParent(gun, false); gunRoot[i] = r; }
        // 0: بندقية (طراز M4): جسم علوي وسفلي، سكة، ماسورة وكاتم لهب، مقبض ومخزن مائل وكتف
        Part(0, PrimitiveType.Cube, new Vector3(0, 0.02f, 0.02f), new Vector3(0.06f, 0.075f, 0.32f), mt);
        Part(0, PrimitiveType.Cube, new Vector3(0, -0.03f, 0.0f), new Vector3(0.055f, 0.06f, 0.26f), dk);
        Part(0, PrimitiveType.Cube, new Vector3(0, 0.068f, 0.03f), new Vector3(0.03f, 0.018f, 0.3f), dk);
        Part(0, PrimitiveType.Cube, new Vector3(0, 0.02f, 0.26f), new Vector3(0.055f, 0.06f, 0.2f), lt);
        Part(0, PrimitiveType.Cylinder, new Vector3(0, 0.025f, 0.46f), new Vector3(0.022f, 0.12f, 0.022f), dk, 90f);
        Part(0, PrimitiveType.Cylinder, new Vector3(0, 0.025f, 0.58f), new Vector3(0.034f, 0.035f, 0.034f), dk, 90f);
        Part(0, PrimitiveType.Cube, new Vector3(0, 0.1f, 0.0f), new Vector3(0.03f, 0.04f, 0.05f), dk);
        Part(0, PrimitiveType.Cube, new Vector3(0, 0.095f, 0.2f), new Vector3(0.012f, 0.05f, 0.012f), dk);
        Part(0, PrimitiveType.Cube, new Vector3(0, -0.09f, 0.06f), new Vector3(0.04f, 0.15f, 0.07f), dk, 12f);
        Part(0, PrimitiveType.Cube, new Vector3(0, -0.09f, -0.1f), new Vector3(0.035f, 0.1f, 0.045f), dk, -15f);
        Part(0, PrimitiveType.Cube, new Vector3(0, -0.01f, -0.28f), new Vector3(0.05f, 0.09f, 0.2f), tan);
        // 1: رشاش خفيف قصير مع كاتم ومخزن طويل
        Part(1, PrimitiveType.Cube, new Vector3(0, 0.0f, 0.0f), new Vector3(0.06f, 0.09f, 0.28f), mt);
        Part(1, PrimitiveType.Cube, new Vector3(0, 0.055f, 0.02f), new Vector3(0.03f, 0.02f, 0.24f), dk);
        Part(1, PrimitiveType.Cylinder, new Vector3(0, 0.01f, 0.32f), new Vector3(0.04f, 0.13f, 0.04f), dk, 90f);
        Part(1, PrimitiveType.Cube, new Vector3(0, -0.13f, 0.05f), new Vector3(0.035f, 0.2f, 0.06f), dk);
        Part(1, PrimitiveType.Cube, new Vector3(0, -0.085f, -0.09f), new Vector3(0.035f, 0.09f, 0.045f), dk, -15f);
        Part(1, PrimitiveType.Cube, new Vector3(0, 0.0f, -0.2f), new Vector3(0.03f, 0.03f, 0.18f), lt);
        Part(1, PrimitiveType.Cube, new Vector3(0, 0.08f, 0.1f), new Vector3(0.04f, 0.035f, 0.05f), dk);
        // 2: شوتغن: ماسورة طويلة وأنبوب ذخيرة ومضخّة خشبية وكتف خشبي
        Part(1 + 1, PrimitiveType.Cube, new Vector3(0, 0.0f, 0.0f), new Vector3(0.065f, 0.09f, 0.3f), mt);
        Part(2, PrimitiveType.Cylinder, new Vector3(0, 0.025f, 0.45f), new Vector3(0.035f, 0.2f, 0.035f), dk, 90f);
        Part(2, PrimitiveType.Cylinder, new Vector3(0, -0.02f, 0.42f), new Vector3(0.03f, 0.17f, 0.03f), lt, 90f);
        Part(2, PrimitiveType.Cube, new Vector3(0, -0.035f, 0.3f), new Vector3(0.06f, 0.05f, 0.14f), wood);
        Part(2, PrimitiveType.Cube, new Vector3(0, -0.085f, -0.08f), new Vector3(0.035f, 0.09f, 0.045f), dk, -15f);
        Part(2, PrimitiveType.Cube, new Vector3(0, -0.02f, -0.27f), new Vector3(0.055f, 0.1f, 0.24f), wood);
        // وميض الفوهة
        flash = GameObject.CreatePrimitive(PrimitiveType.Cube); Destroy(flash.GetComponent<Collider>()); flash.transform.SetParent(gun, false);
        flash.transform.localScale = new Vector3(0.16f, 0.16f, 0.1f); Mat.Set(flash, new Color(1f, 0.85f, 0.4f)); flash.SetActive(false);
    }
    void Equip(int i)
    {
        cur = i; reloadEnd = 0; for (int k = 0; k < 3; k++) gunRoot[k].gameObject.SetActive(k == i);
        flash.transform.localPosition = new Vector3(0, 0.025f, i == 1 ? 0.48f : i == 2 ? 0.68f : 0.66f);
    }
    public void NextWeapon() { Equip((cur + 1) % Ws.Length); }
    public void ToggleAim() { aim = !aim; }

    void Update()
    {
        if (!Alive) { if (Time.time > respawnAt) Respawn(); return; }
        Vector2 look = touchLook; touchLook = Vector2.zero;
        bool aimNow = aim;
#if !UNITY_ANDROID && !UNITY_IOS
        if (Cursor.lockState != CursorLockMode.Locked && Input.GetMouseButtonDown(0)) { Cursor.lockState = CursorLockMode.Locked; Cursor.visible = false; }
        if (Cursor.lockState == CursorLockMode.Locked) look += new Vector2(Input.GetAxis("Mouse X"), Input.GetAxis("Mouse Y")) * 2.2f;
        aimNow = aim || Input.GetMouseButton(1);
#endif
        float sens = aimNow ? 0.11f : 0.2f;
        yaw += look.x * sens; pitch = Mathf.Clamp(pitch - look.y * sens, -85f, 85f);
        transform.rotation = Quaternion.Euler(0, yaw, 0); 
        Vector2 mv = touchMove; bool sprint = false;
#if !UNITY_ANDROID && !UNITY_IOS
        mv += new Vector2(Input.GetAxisRaw("Horizontal"), Input.GetAxisRaw("Vertical"));
        if (Input.GetKeyDown(KeyCode.Alpha1)) Equip(0); if (Input.GetKeyDown(KeyCode.Alpha2)) Equip(1); if (Input.GetKeyDown(KeyCode.Alpha3)) Equip(2);
        if (Input.GetKeyDown(KeyCode.Q)) NextWeapon();
        sprint = Input.GetKey(KeyCode.LeftShift);
#endif
        mv = Vector2.ClampMagnitude(mv, 1f);
        if (mv.magnitude > 0.9f && Application.isMobilePlatform) sprint = true; // دفع العصا للنهاية = ركض
        float sp = aimNow ? 3.2f : sprint ? 8f : 5.5f;
        Vector3 v = (transform.right * mv.x + transform.forward * mv.y) * sp;
        bool grounded = cc.isGrounded;
        if (grounded) { vy = -1f; if (Input.GetKeyDown(KeyCode.Space)) vy = 6.5f; } else vy -= 20f * Time.deltaTime;
        v.y = vy; cc.Move(v * Time.deltaTime);
        bool moving = mv.sqrMagnitude > 0.05f && grounded;
        if (moving) { bobT += Time.deltaTime * (sprint ? 12f : 8f); stepT -= Time.deltaTime; if (stepT <= 0f) { stepT = sprint ? 0.3f : 0.42f; Snd.PlayStep(transform.position); } }
        // هزّ الكاميرا والسلاح والتصويب
        kick = Mathf.Lerp(kick, 0f, Time.deltaTime * 12f);
        float bob = moving ? Mathf.Sin(bobT) * (aimNow ? 0.004f : 0.03f) : 0f;
        cam.transform.localPosition = new Vector3(0, 1.6f + bob, 0);
        cam.transform.localRotation = Quaternion.Euler(pitch - kick, 0, 0);
        cam.fieldOfView = Mathf.Lerp(cam.fieldOfView, aimNow ? 48f : sprint && moving ? 78f : 70f, Time.deltaTime * 10f);
        var w = Ws[cur];
        if (reloadEnd > 0 && Time.time >= reloadEnd) { int t = Mathf.Min(w.mag - ammoA[cur], resA[cur]); ammoA[cur] += t; resA[cur] -= t; reloadEnd = 0; }
        if (Input.GetKeyDown(KeyCode.R)) Reload();
        bool fire = touchFire || (Input.GetMouseButton(0) && Cursor.lockState == CursorLockMode.Locked);
        if (fire && reloadEnd == 0 && Time.time >= nextShot)
        {
            if (ammoA[cur] <= 0) { if (resA[cur] > 0) Reload(); else { nextShot = Time.time + 0.4f; Snd.PlayDry(cam.transform.position); } }
            else
            {
                ammoA[cur]--; nextShot = Time.time + w.interval;
                float sp2 = w.spread * (aimNow ? 0.45f : 1f) * (moving ? 1.5f : 1f);
                for (int i = 0; i < w.pellets; i++)
                {
                    Vector3 d = cam.transform.forward + Random.insideUnitSphere * sp2;
                    Game.I.Shoot(this, cam.transform.position, d.normalized, w.dmg, w.range, cur);
                }
                float rc = w.recoil * (aimNow ? 0.6f : 1f); kick += rc; pitch -= rc * 0.5f; yaw += Random.Range(-0.12f, 0.12f) * rc;
                gun.localPosition += new Vector3(0, 0.005f, -0.06f - (w.pellets > 1 ? 0.08f : 0f)); flashEnd = Time.time + 0.045f;
            }
        }
        flash.SetActive(Time.time < flashEnd);
        // موضع السلاح: وركٌ/تصويب + تأرجح + نزول أثناء التعبئة
        Vector3 target = aimNow ? AimPos : HipPos; target += new Vector3(moving && !aimNow ? Mathf.Sin(bobT * 0.5f) * 0.012f : 0f, bob * 0.5f, 0f);
        float rl = 0f; if (reloadEnd > 0f) { float u = (Time.time - reloadStart) / Mathf.Max(0.1f, reloadEnd - reloadStart); rl = Mathf.Sin(u * Mathf.PI); target += new Vector3(0.04f, -0.14f * rl, 0f); }
        gun.localPosition = Vector3.Lerp(gun.localPosition, target, Time.deltaTime * 14f);
        gun.localRotation = Quaternion.Euler(rl * 25f, 0, rl * -18f);
        // تعافٍ تدريجي بعد 6 ثوانٍ بلا ضرر
        if (hp < 100f && Time.time - lastHit > 6f) hp = Mathf.Min(100f, hp + 10f * Time.deltaTime);
    }
    public void Reload()
    {
        if (reloadEnd == 0 && ammoA[cur] < Ws[cur].mag && resA[cur] > 0)
        {
            reloadStart = Time.time; reloadEnd = Time.time + Ws[cur].reload; aim = false;
            if (cur != 2) Snd.PlayReload(cam.transform.position); else Snd.PlayDry(cam.transform.position);
        }
    }
    public bool Reloading => reloadEnd > 0;
    protected override void OnHurt(Fighter src) { lastHit = Time.time; Snd.PlayHurt(transform.position); }
    protected override void OnDeath() { respawnAt = Time.time + 3f; aim = false; }
    public void ReviveNow() { respawnAt = 0f; }
    void Respawn()
    {
        hp = 100; for (int i = 0; i < 3; i++) { ammoA[i] = Ws[i].mag; resA[i] = Ws[i].reserve; }
        reloadEnd = 0; cc.enabled = false; transform.position = Game.I.SpawnPoint(team); cc.enabled = true;
    }
}
