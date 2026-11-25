using UnityEngine;

// مواد آمنة للبناء: تجرّب عدة Shaders (تبقى موجودة بعد الـstripping) وتلوّن بدون الاعتماد على الافتراضي
public static class Mat
{
    static Shader sh; static readonly string[] names = { "Standard", "Universal Render Pipeline/Lit", "Unlit/Color", "Sprites/Default" };
    static Shader Get()
    {
        if (sh != null) return sh;
        foreach (var n in names) { var s = Shader.Find(n); if (s != null && s.isSupported) { sh = s; break; } }
        return sh;
    }
    public static void Set(GameObject go, Color c)
    {
        var r = go.GetComponent<Renderer>(); if (r == null) return;
        var m = new Material(Get()); m.color = c;
        if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", c);
        if (m.HasProperty("_Glossiness")) m.SetFloat("_Glossiness", 0.1f);
        r.sharedMaterial = m;
    }
}
