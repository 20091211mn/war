using UnityEngine;

// أصوات مولَّدة بالكود (بلا ملفات): طلقات لكل سلاح، تعبئة، طقطقة فارغ، إصابة، خطوات. تُشغَّل عبر مجمّع AudioSource ثلاثي الأبعاد.
public static class Snd
{
    static AudioClip[] shot = new AudioClip[3]; static AudioClip reload, hit, dry, step, hurt;
    static AudioSource[] pool; static int pi; const int Rate = 22050;

    static AudioClip Make(string name, float secs, System.Func<float, float, float> f)
    {
        int n = Mathf.Max(1, (int)(Rate * secs)); var d = new float[n];
        for (int i = 0; i < n; i++) d[i] = Mathf.Clamp(f((float)i / Rate, (float)i / n), -1f, 1f);
        var c = AudioClip.Create(name, n, 1, Rate, false); c.SetData(d, 0); return c;
    }
    static float Click(float t, float at, float freq, float decay) { float x = t - at; if (x < 0f) return 0f; return Mathf.Sin(6.2832f * freq * x) * Mathf.Exp(-x * decay); }

    static AudioClip Shot(int w)
    {
        if (shot[w] != null) return shot[w];
        var rnd = new System.Random(7 + w); float lp = 0f;
        float crackK = w == 1 ? 110f : w == 2 ? 55f : 80f, boomF = w == 2 ? 38f : w == 1 ? 70f : 52f, boomK = w == 2 ? 8f : 12f, tailK = w == 2 ? 5f : 9f, dur = w == 2 ? 0.7f : 0.45f, amp = w == 2 ? 0.95f : 0.8f;
        shot[w] = Make("shot" + w, dur, (t, u) =>
        {
            float nz = (float)(rnd.NextDouble() * 2 - 1); lp += (nz - lp) * 0.3f;
            float crack = (nz - lp * 0.4f) * Mathf.Exp(-t * crackK), boom = Mathf.Sin(6.2832f * (boomF + 70f * Mathf.Exp(-t * 30f)) * t) * Mathf.Exp(-t * boomK), tail = lp * Mathf.Exp(-t * tailK) * 0.8f;
            return (crack + boom + tail) * amp;
        });
        return shot[w];
    }
    static AudioClip Reload()
    {
        if (reload != null) return reload; var rnd = new System.Random(3);
        reload = Make("reload", 1.5f, (t, u) =>
        {
            float nz = (float)(rnd.NextDouble() * 2 - 1);
            float v = Click(t, 0.05f, 900f, 60f) * 0.7f + Click(t, 0.07f, 220f, 40f) * 0.8f;   // المخزن يخرج
            v += Click(t, 0.62f, 1400f, 70f) * 0.6f + Click(t, 0.64f, 300f, 45f) * 0.9f;        // مخزن جديد
            v += (Click(t, 1.05f, 700f, 45f) + Click(t, 1.17f, 1200f, 60f)) * 0.8f;             // سحب الترباس
            return v * 0.6f + (t > 0.3f && t < 0.5f ? nz * 0.02f : 0f);
        });
        return reload;
    }
    static AudioClip Hit() { if (hit == null) hit = Make("hit", 0.07f, (t, u) => Mathf.Sin(6.2832f * 1700f * t) * Mathf.Exp(-t * 70f) * 0.6f); return hit; }
    static AudioClip Dry() { if (dry == null) dry = Make("dry", 0.06f, (t, u) => (Click(t, 0f, 1500f, 120f) + Click(t, 0.012f, 600f, 90f)) * 0.6f); return dry; }
    static AudioClip Step() { if (step == null) { var rnd = new System.Random(11); float lp = 0; step = Make("step", 0.14f, (t, u) => { lp += ((float)(rnd.NextDouble() * 2 - 1) - lp) * 0.12f; return lp * Mathf.Exp(-t * 28f) * 1.4f; }); } return step; }
    static AudioClip Hurt() { if (hurt == null) hurt = Make("hurt", 0.25f, (t, u) => Mathf.Sin(6.2832f * (160f - 80f * u) * t) * Mathf.Exp(-t * 12f) * 0.7f); return hurt; }

    static void Ensure()
    {
        if (pool != null && pool[0] != null) return;
        var go = new GameObject("Snd"); pool = new AudioSource[16];
        for (int i = 0; i < pool.Length; i++) { var a = new GameObject("a" + i).AddComponent<AudioSource>(); a.transform.SetParent(go.transform); a.spatialBlend = 1f; a.rolloffMode = AudioRolloffMode.Linear; a.minDistance = 4f; a.maxDistance = 90f; a.playOnAwake = false; pool[i] = a; }
    }
    static void Play(AudioClip c, Vector3 pos, float vol, float pitch = 1f)
    {
        Ensure(); var a = pool[pi]; pi = (pi + 1) % pool.Length;
        a.transform.position = pos; a.pitch = pitch; a.volume = vol; a.clip = c; a.Play();
    }
    public static void PlayShot(int w, Vector3 p, float vol) { Play(Shot(Mathf.Clamp(w, 0, 2)), p, vol, Random.Range(0.94f, 1.06f)); }
    public static void PlayReload(Vector3 p) { Play(Reload(), p, 0.7f); }
    public static void PlayHit(Vector3 p) { Play(Hit(), p, 0.8f); }
    public static void PlayDry(Vector3 p) { Play(Dry(), p, 0.6f); }
    public static void PlayStep(Vector3 p) { Play(Step(), p, 0.35f, Random.Range(0.85f, 1.15f)); }
    public static void PlayHurt(Vector3 p) { Play(Hurt(), p, 0.8f); }
}
