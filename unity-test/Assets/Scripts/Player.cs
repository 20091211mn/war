using UnityEngine;

// لاعب منظور أول: WASD + ماوس على الحاسوب، وعصا افتراضية + سحب + أزرار على الجوال. 3 أسلحة (1-3 أو زر SW)
[RequireComponent(typeof(CharacterController))]
public class Player : Fighter
{
    public struct W { public string name; public float dmg, interval, spread, range, reload; public int mag, reserve, pellets; public Vector3 size; }
    public static readonly W[] Ws = {
        new W { name = "RIFLE",   dmg = 28, interval = 0.10f, spread = 0.012f, range = 150, reload = 1.6f, mag = 30, reserve = 120, pellets = 1, size = new Vector3(0.07f, 0.10f, 0.60f) },
        new W { name = "SMG",     dmg = 17, interval = 0.065f, spread = 0.030f, range = 90, reload = 1.4f, mag = 40, reserve = 160, pellets = 1, size = new Vector3(0.07f, 0.10f, 0.42f) },
        new W { name = "SHOTGUN", dmg = 12, interval = 0.85f, spread = 0.075f, range = 45, reload = 2.0f, mag = 6, reserve = 30, pellets = 8, size = new Vector3(0.09f, 0.10f, 0.70f) },
    };
    CharacterController cc; public Camera cam; Transform gun;
    float yaw, pitch, vy, nextShot, reloadEnd, respawnAt;
    public int cur;
    int[] ammoA = new int[3], resA = new int[3];
    public int ammo => ammoA[cur]; public int reserve => resA[cur];
    public Vector2 touchMove; public bool touchFire; public Vector2 touchLook;
    public float lastHit = -10f;
    public string WeaponName => Ws[cur].name;

    void Awake()
    {
        label = "YOU";
        cc = GetComponent<CharacterController>(); cc.height = 1.8f; cc.radius = 0.35f; cc.center = new Vector3(0, 0.9f, 0);
        var go = new GameObject("Cam"); go.transform.SetParent(transform); go.transform.localPosition = new Vector3(0, 1.6f, 0);
        cam = go.AddComponent<Camera>(); cam.nearClipPlane = 0.05f; cam.farClipPlane = 400f; go.tag = "MainCamera";
        cam.clearFlags = CameraClearFlags.SolidColor; cam.backgroundColor = new Color(0.55f, 0.72f, 0.92f);
        go.AddComponent<AudioListener>();
        var g = GameObject.CreatePrimitive(PrimitiveType.Cube); Destroy(g.GetComponent<Collider>());
        g.transform.SetParent(go.transform); g.transform.localPosition = new Vector3(0.25f, -0.22f, 0.5f); gun = g.transform;
        Mat.Set(g, new Color(0.12f, 0.12f, 0.14f));
        for (int i = 0; i < 3; i++) { ammoA[i] = Ws[i].mag; resA[i] = Ws[i].reserve; }
        Equip(0);
    }
    void Equip(int i) { cur = i; reloadEnd = 0; gun.localScale = Ws[i].size; }
    public void NextWeapon() { Equip((cur + 1) % Ws.Length); }

    void Update()
    {
        if (!Alive) { if (Time.time > respawnAt) Respawn(); return; }
        Vector2 look = touchLook; touchLook = Vector2.zero;
#if !UNITY_ANDROID && !UNITY_IOS
        if (Cursor.lockState != CursorLockMode.Locked && Input.GetMouseButtonDown(0)) { Cursor.lockState = CursorLockMode.Locked; Cursor.visible = false; }
        if (Cursor.lockState == CursorLockMode.Locked) look += new Vector2(Input.GetAxis("Mouse X"), Input.GetAxis("Mouse Y")) * 2.2f;
#endif
        yaw += look.x * 0.2f; pitch = Mathf.Clamp(pitch - look.y * 0.2f, -85f, 85f);
        transform.rotation = Quaternion.Euler(0, yaw, 0); cam.transform.localRotation = Quaternion.Euler(pitch, 0, 0);
        Vector2 mv = touchMove;
#if !UNITY_ANDROID && !UNITY_IOS
        mv += new Vector2(Input.GetAxisRaw("Horizontal"), Input.GetAxisRaw("Vertical"));
        if (Input.GetKeyDown(KeyCode.Alpha1)) Equip(0); if (Input.GetKeyDown(KeyCode.Alpha2)) Equip(1); if (Input.GetKeyDown(KeyCode.Alpha3)) Equip(2);
        if (Input.GetKeyDown(KeyCode.Q)) NextWeapon();
#endif
        mv = Vector2.ClampMagnitude(mv, 1f);
        float sp = Input.GetKey(KeyCode.LeftShift) ? 8f : 5.5f;
        Vector3 v = (transform.right * mv.x + transform.forward * mv.y) * sp;
        if (cc.isGrounded) { vy = -1f; if (Input.GetKeyDown(KeyCode.Space)) vy = 6.5f; } else vy -= 20f * Time.deltaTime;
        v.y = vy; cc.Move(v * Time.deltaTime);
        var w = Ws[cur];
        if (reloadEnd > 0 && Time.time >= reloadEnd) { int t = Mathf.Min(w.mag - ammoA[cur], resA[cur]); ammoA[cur] += t; resA[cur] -= t; reloadEnd = 0; }
        if (Input.GetKeyDown(KeyCode.R)) Reload();
        bool fire = touchFire || (Input.GetMouseButton(0) && Cursor.lockState == CursorLockMode.Locked);
        if (fire && reloadEnd == 0 && Time.time >= nextShot)
        {
            if (ammoA[cur] <= 0) Reload();
            else
            {
                ammoA[cur]--; nextShot = Time.time + w.interval;
                for (int i = 0; i < w.pellets; i++)
                {
                    Vector3 d = cam.transform.forward + Random.insideUnitSphere * w.spread;
                    Game.I.Shoot(this, cam.transform.position, d.normalized, w.dmg, w.range);
                }
                pitch -= w.pellets > 1 ? 2.5f : 0.35f; // ارتداد بسيط
                gun.localPosition = new Vector3(0.25f, -0.22f, 0.42f);
            }
        }
        gun.localPosition = Vector3.Lerp(gun.localPosition, new Vector3(0.25f, -0.22f, 0.5f), Time.deltaTime * 14f);
    }
    public void Reload() { if (reloadEnd == 0 && ammoA[cur] < Ws[cur].mag && resA[cur] > 0) reloadEnd = Time.time + Ws[cur].reload; }
    public bool Reloading => reloadEnd > 0;
    protected override void OnHurt(Fighter src) { lastHit = Time.time; }
    protected override void OnDeath() { respawnAt = Time.time + 3f; }
    public void ReviveNow() { respawnAt = 0f; }
    void Respawn()
    {
        hp = 100; for (int i = 0; i < 3; i++) { ammoA[i] = Ws[i].mag; resA[i] = Ws[i].reserve; }
        reloadEnd = 0; cc.enabled = false; transform.position = Game.I.SpawnPoint(team); cc.enabled = true;
    }
}
