using UnityEngine;

// بوت بسيط: يبحث عن أقرب عدو، يتحرك نحوه مع تفادي العوائق، ويطلق النار عند وجود خط رؤية
[RequireComponent(typeof(CharacterController))]
public class Bot : Fighter
{
    CharacterController cc; Fighter target; float nextThink, nextShot, respawnAt, vy, strafe;
    Renderer[] rs;

    void Awake() { cc = GetComponent<CharacterController>(); cc.height = 1.8f; cc.radius = 0.35f; cc.center = new Vector3(0, 0.9f, 0); }
    public void Init(int t)
    {
        team = t; var body = GameObject.CreatePrimitive(PrimitiveType.Capsule); Destroy(body.GetComponent<Collider>());
        body.transform.SetParent(transform); body.transform.localPosition = new Vector3(0, 0.9f, 0); body.transform.localScale = new Vector3(0.7f, 0.9f, 0.7f);
        body.GetComponent<Renderer>().material.color = t == 0 ? new Color(0.8f, 0.2f, 0.2f) : new Color(0.2f, 0.4f, 0.85f);
        var hit = body.AddComponent<CapsuleCollider>(); hit.height = 2f; hit.radius = 0.5f; body.layer = 0; body.name = "BotBody"; body.AddComponent<HitProxy>().owner = this;
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
        vy = cc.isGrounded ? -1f : vy - 20f * Time.deltaTime;
        cc.Move((move.normalized * 4.2f + Vector3.up * vy) * Time.deltaTime);
    }
    protected override void OnDeath() { respawnAt = Time.time + 3f; }
}

// يمرّر إصابات الـRaycast من مجسّم البوت إلى البوت نفسه
public class HitProxy : MonoBehaviour { public Fighter owner; }
