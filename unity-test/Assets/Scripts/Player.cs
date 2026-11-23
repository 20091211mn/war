using UnityEngine;

// لاعب منظور أول: WASD + ماوس على الحاسوب، وعصا افتراضية + سحب + أزرار على الجوال
[RequireComponent(typeof(CharacterController))]
public class Player : Fighter
{
    CharacterController cc; public Camera cam;
    float yaw, pitch, vy, nextShot, reloadEnd, respawnAt;
    public int ammo = 30, reserve = 120; const int Mag = 30;
    public Vector2 touchMove; public bool touchFire; public Vector2 touchLook;
    public float flash;

    void Awake()
    {
        cc = GetComponent<CharacterController>(); cc.height = 1.8f; cc.radius = 0.35f; cc.center = new Vector3(0, 0.9f, 0);
        var go = new GameObject("Cam"); go.transform.SetParent(transform); go.transform.localPosition = new Vector3(0, 1.6f, 0);
        cam = go.AddComponent<Camera>(); cam.nearClipPlane = 0.05f; cam.farClipPlane = 300f; go.tag = "MainCamera";
        go.AddComponent<AudioListener>();
        var gun = GameObject.CreatePrimitive(PrimitiveType.Cube); Destroy(gun.GetComponent<Collider>());
        gun.transform.SetParent(go.transform); gun.transform.localPosition = new Vector3(0.25f, -0.22f, 0.5f); gun.transform.localScale = new Vector3(0.07f, 0.1f, 0.6f);
        gun.GetComponent<Renderer>().material.color = new Color(0.12f, 0.12f, 0.14f);
    }

    void Update()
    {
        if (!Alive) { if (Time.time > respawnAt) Respawn(); return; }
        flash = Mathf.Max(0, flash - Time.deltaTime * 4f);
        // نظر
        Vector2 look = touchLook; touchLook = Vector2.zero;
#if !UNITY_ANDROID && !UNITY_IOS
        if (Cursor.lockState != CursorLockMode.Locked && Input.GetMouseButtonDown(0)) { Cursor.lockState = CursorLockMode.Locked; Cursor.visible = false; }
        if (Cursor.lockState == CursorLockMode.Locked) look += new Vector2(Input.GetAxis("Mouse X"), Input.GetAxis("Mouse Y")) * 2.2f;
#endif
        yaw += look.x * 0.2f * (Application.isMobilePlatform ? 1f : 1f); pitch = Mathf.Clamp(pitch - look.y * 0.2f, -85f, 85f);
        transform.rotation = Quaternion.Euler(0, yaw, 0); cam.transform.localRotation = Quaternion.Euler(pitch, 0, 0);
        // حركة
        Vector2 mv = touchMove;
#if !UNITY_ANDROID && !UNITY_IOS
        mv += new Vector2(Input.GetAxisRaw("Horizontal"), Input.GetAxisRaw("Vertical"));
#endif
        mv = Vector2.ClampMagnitude(mv, 1f);
        float sp = (Input.GetKey(KeyCode.LeftShift) ? 8f : 5.5f);
        Vector3 v = (transform.right * mv.x + transform.forward * mv.y) * sp;
        if (cc.isGrounded) { vy = -1f; if (Input.GetKeyDown(KeyCode.Space)) vy = 6.5f; } else vy -= 20f * Time.deltaTime;
        v.y = vy; cc.Move(v * Time.deltaTime);
        // تعبئة وإطلاق
        if (reloadEnd > 0 && Time.time >= reloadEnd) { int t = Mathf.Min(Mag - ammo, reserve); ammo += t; reserve -= t; reloadEnd = 0; }
        if (Input.GetKeyDown(KeyCode.R)) Reload();
        bool fire = touchFire || Input.GetMouseButton(0) && Cursor.lockState == CursorLockMode.Locked;
        if (fire && reloadEnd == 0 && Time.time >= nextShot)
        {
            if (ammo <= 0) Reload();
            else
            {
                ammo--; nextShot = Time.time + 0.1f; flash = 1f;
                Vector3 d = cam.transform.forward + Random.insideUnitSphere * 0.012f;
                Game.I.Shoot(this, cam.transform.position, d.normalized, 28f, 150f);
            }
        }
    }
    public void Reload() { if (reloadEnd == 0 && ammo < Mag && reserve > 0) reloadEnd = Time.time + 1.6f; }
    public bool Reloading => reloadEnd > 0;
    protected override void OnDeath() { respawnAt = Time.time + 3f; }
    void Respawn() { hp = 100; ammo = Mag; reserve = 120; reloadEnd = 0; cc.enabled = false; transform.position = Game.I.SpawnPoint(team); cc.enabled = true; }
}
