using UnityEngine;

// بوت بسيط: يبحث عن أقرب عدو، يتحرك نحوه مع تفادي العوائق، ويطلق النار عند وجود خط رؤية
[RequireComponent(typeof(CharacterController))]
public class Bot : Fighter
{
    CharacterController cc; Fighter target; float nextThink, nextShot, respawnAt, vy, strafe;
    Renderer[] rs;

    void Awake() { cc = GetComponent<CharacterController>(); cc.height = 1.8f; cc.radius = 0.35f; cc.center = new Vector3(0, 0.9f, 0); }
    Transform legL, legR; float walkT;
    Transform Part(PrimitiveType pt, Vector3 pos, Vector3 scale, Color c)
    {
        var p = GameObject.CreatePrimitive(pt); Destroy(p.GetComponent<Collider>());
        p.transform.SetParent(transform); p.transform.localPosition = pos; p.transform.localScale = scale; Mat.Set(p, c); return p.transform;
    }
    public void Init(int t)
    {
        team = t; label = (t == 0 ? "RED" : "BLUE") + Random.Range(1, 99);
        // مجسّم جندي من أجزاء: ساقان وجذع ورأس وخوذة وذراعان وبندقية (التصادم كبسولة واحدة)
        Color uni = t == 0 ? new Color(0.62f, 0.16f, 0.14f) : new Color(0.16f, 0.30f, 0.62f), vest = uni * 0.7f, pants = new Color(0.2f, 0.22f, 0.18f), skin = new Color(0.86f, 0.68f, 0.54f), dark = new Color(0.1f, 0.1f, 0.12f);
        legL = Part(PrimitiveType.Cube, new Vector3(-0.13f, 0.42f, 0), new Vector3(0.2f, 0.84f, 0.24f), pants);
        legR = Part(PrimitiveType.Cube, new Vector3(0.13f, 0.42f, 0), new Vector3(0.2f, 0.84f, 0.24f), pants);
        Part(PrimitiveType.Cube, new Vector3(0, 1.17f, 0), new Vector3(0.52f, 0.62f, 0.30f), uni);
        Part(PrimitiveType.Cube, new Vector3(0, 1.2f, 0.02f), new Vector3(0.56f, 0.4f, 0.34f), vest);
        Part(PrimitiveType.Sphere, new Vector3(0, 1.62f, 0), new Vector3(0.24f, 0.26f, 0.25f), skin);
        Part(PrimitiveType.Sphere, new Vector3(0, 1.68f, 0), new Vector3(0.29f, 0.2f, 0.3f), uni * 0.6f);
        Part(PrimitiveType.Cube, new Vector3(-0.34f, 1.18f, 0.12f), new Vector3(0.13f, 0.5f, 0.14f), uni);
        Part(PrimitiveType.Cube, new Vector3(0.30f, 1.2f, 0.22f), new Vector3(0.13f, 0.14f, 0.5f), uni);
        Part(PrimitiveType.Cube, new Vector3(0.12f, 1.18f, 0.5f), new Vector3(0.06f, 0.1f, 0.7f), dark);
        var hb = new GameObject("BotBody"); hb.transform.SetParent(transform); hb.transform.localPosition = new Vector3(0, 0.95f, 0);
        var hit = hb.AddComponent<CapsuleCollider>(); hit.height = 1.95f; hit.radius = 0.42f; hb.AddComponent<HitProxy>().owner = this;
        rs = GetComponentsInChildren<Renderer>();
    }

    void Update()
    {
        bool a = Alive; foreach (var r in rs) r.enabled = a;
        if (!a) { if (Time.time > respawnAt) { hp = 100; cc.enabled = false; transform.position = Game.I.SpawnPoint(team); cc.enabled = true; } return; }
        if (Time.time > nextThink) { nextThink = Time.time + 0.4f; target = Game.I.NearestEnemy(this); strafe = Random.Range(-1f, 1f); }
        Vector3 move = Vector3.zero;
        if (target != null)
        {
            Vector3 to = target.transform.position - transform.position; to.y = 0; float dist = to.magnitude;
            Vector3 dir = to.normalized;
            transform.rotation = Quaternion.Slerp(transform.rotation, Quaternion.LookRotation(dir), Time.deltaTime * 8f);
            bool los = !Physics.Linecast(transform.position + Vector3.up * 1.5f, target.transform.position + Vector3.up * 1.2f, Game.I.worldMask);
            if (dist > 14f || !los) move = dir; else move = transform.right * strafe * 0.6f;
            if (Physics.Raycast(transform.position + Vector3.up * 0.5f, move.normalized, 1.5f, Game.I.worldMask)) move = Quaternion.Euler(0, 70f * Mathf.Sign(strafe + 0.01f), 0) * move;
            if (los && dist < 45f && Time.time >= nextShot)
            {
                nextShot = Time.time + 0.45f; Vector3 o = transform.position + Vector3.up * 1.5f; Vector3 d = (target.transform.position + Vector3.up * 1.2f - o).normalized + Random.insideUnitSphere * 0.05f;
                Game.I.Shoot(this, o, d.normalized, 9f, 60f);
            }
        }
        // فصل البوتات عن بعضها (حتى لا تتداخل أجسامها)
        foreach (var f in Game.I.all)
        {
            if (f == null || f == this || !f.Alive) continue;
            Vector3 dd = transform.position - f.transform.position; dd.y = 0; float m = dd.magnitude;
            if (m < 1.5f && m > 0.001f) move += dd / m * (1.5f - m) * 2.5f;
        }
        if (move.sqrMagnitude > 0.01f && legL != null)
        {
            walkT += Time.deltaTime * 9f; float s = Mathf.Sin(walkT) * 0.16f;
            legL.localPosition = new Vector3(-0.13f, 0.42f, s); legR.localPosition = new Vector3(0.13f, 0.42f, -s);
        }
        vy = cc.isGrounded ? -1f : vy - 20f * Time.deltaTime;
        cc.Move((move.normalized * 4.2f + Vector3.up * vy) * Time.deltaTime);
    }
    protected override void OnDeath() { respawnAt = Time.time + 3f; }
}

// يمرّر إصابات الـRaycast من مجسّم البوت إلى البوت نفسه
public class HitProxy : MonoBehaviour { public Fighter owner; }
