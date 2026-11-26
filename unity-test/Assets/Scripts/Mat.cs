using UnityEngine;

// مواد آمنة للبناء: مادة Standard محفوظة في Resources (فيبقى الـShader داخل البناء دائماً)
public static class Mat
{
    static Material baseMat; static Shader sh;
    static readonly string[] names = { "Standard", "Universal Render Pipeline/Lit", "Unlit/Color", "Sprites/Default" };
    static Material Make()
    {
        if (baseMat == null) baseMat = Resources.Load<Material>("Base");
        if (baseMat != null && baseMat.shader != null && baseMat.shader.isSupported) return new Material(baseMat);
        if (sh == null) foreach (var n in names) { var s = Shader.Find(n); if (s != null && s.isSupported) { sh = s; break; } }
        return new Material(sh);
    }
    public static void Set(GameObject go, Color c)
    {
        var r = go.GetComponent<Renderer>(); if (r == null) return;
        var m = Make(); m.color = c;
        if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", c);
        r.sharedMaterial = m;
    }
}
