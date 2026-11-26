using System.Collections.Generic;
using UnityEngine;

// مواد آمنة للبناء: مادة Standard محفوظة في Resources (فيبقى الـShader داخل البناء دائماً) + كاش لكل لون
public static class Mat
{
    static Material baseMat; static Shader sh; static Dictionary<int, Material> cache = new Dictionary<int, Material>();
    static readonly string[] names = { "Standard", "Universal Render Pipeline/Lit", "Unlit/Color", "Sprites/Default" };
    static Material Make()
    {
        if (baseMat == null) baseMat = Resources.Load<Material>("Base");
        if (baseMat != null && baseMat.shader != null && baseMat.shader.isSupported) return new Material(baseMat);
        if (sh == null) foreach (var n in names) { var s = Shader.Find(n); if (s != null && s.isSupported) { sh = s; break; } }
        return new Material(sh);
    }
    public static Material Get(Color c, float gloss = 0.1f)
    {
        var k = (Color32)c; int key = (k.r << 16) | (k.g << 8) | k.b | ((int)(gloss * 10) << 24);
        Material m; if (cache.TryGetValue(key, out m)) return m;
        m = Make(); m.color = c; if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", c);
        if (m.HasProperty("_Glossiness")) m.SetFloat("_Glossiness", gloss);
        cache[key] = m; return m;
    }
    public static void Set(GameObject go, Color c) { var r = go.GetComponent<Renderer>(); if (r != null) r.sharedMaterial = Get(c); }
    public static Material Textured(Color c, Texture2D tex, Vector2 tiling)
    {
        var m = Make(); m.color = c; m.mainTexture = tex; m.mainTextureScale = tiling;
        if (m.HasProperty("_Glossiness")) m.SetFloat("_Glossiness", 0f); return m;
    }
}
