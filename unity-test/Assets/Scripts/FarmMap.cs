using System.Collections.Generic;
using UnityEngine;

// خريطة مزرعة: حقول قمح وذرة وتربة، حظيرة ومنزل مع سقوف، صوامع، جرّار وشاحنة، أسوار وأشجار وبركة وبرج مياه.
// كل المجسمات الزخرفية تُدمج في ميش واحد لكل لون (عدد قليل من عمليات الرسم)، والصلب منها له Collider.
public static class FarmMap
{
    static Transform root; static System.Random rnd;
    static Dictionary<int, List<CombineInstance>> batches; static Dictionary<int, Color> colors;
    static Mesh cubeM, cylM, sphM;
    public static List<Renderer> Rends = new List<Renderer>();

    // ---------- مولّدات ميش ----------
    static void Tri(List<Vector3> v, List<Vector3> n, List<int> t, Vector3 a, Vector3 b, Vector3 c, Vector3 na, Vector3 nb, Vector3 nc, Vector3 outward)
    {
        int i = v.Count; v.Add(a); v.Add(b); v.Add(c); n.Add(na); n.Add(nb); n.Add(nc);
        if (Vector3.Dot(Vector3.Cross(b - a, c - a), outward) >= 0) { t.Add(i); t.Add(i + 1); t.Add(i + 2); } else { t.Add(i); t.Add(i + 2); t.Add(i + 1); }
    }
    static Mesh Build(List<Vector3> v, List<Vector3> n, List<int> t) { var m = new Mesh(); m.SetVertices(v); m.SetNormals(n); m.SetTriangles(t, 0); m.RecalculateBounds(); return m; }
    static Mesh MakeCube()
    {
        var v = new List<Vector3>(); var n = new List<Vector3>(); var t = new List<int>();
        Vector3[] ns = { Vector3.up, Vector3.down, Vector3.left, Vector3.right, Vector3.forward, Vector3.back };
        foreach (var nr in ns)
        {
            Vector3 u = Mathf.Abs(nr.y) > 0.5f ? Vector3.right : Vector3.up; Vector3 w = Vector3.Cross(nr, u).normalized; u = Vector3.Cross(w, nr).normalized;
            Vector3 c = nr * 0.5f; Vector3 p0 = c - u * .5f - w * .5f, p1 = c + u * .5f - w * .5f, p2 = c + u * .5f + w * .5f, p3 = c - u * .5f + w * .5f;
            Tri(v, n, t, p0, p1, p2, nr, nr, nr, nr); Tri(v, n, t, p0, p2, p3, nr, nr, nr, nr);
        }
        return Build(v, n, t);
    }
    static Mesh MakeCyl(int seg = 14)
    {
        var v = new List<Vector3>(); var n = new List<Vector3>(); var t = new List<int>();
        for (int i = 0; i < seg; i++)
        {
            float a0 = Mathf.PI * 2 * i / seg, a1 = Mathf.PI * 2 * (i + 1) / seg;
            Vector3 d0 = new Vector3(Mathf.Cos(a0), 0, Mathf.Sin(a0)), d1 = new Vector3(Mathf.Cos(a1), 0, Mathf.Sin(a1));
            Vector3 b0 = d0 * .5f + Vector3.down * .5f, b1 = d1 * .5f + Vector3.down * .5f, t0 = d0 * .5f + Vector3.up * .5f, t1 = d1 * .5f + Vector3.up * .5f;
            Vector3 mid = (d0 + d1).normalized;
            Tri(v, n, t, b0, b1, t1, d0, d1, d1, mid); Tri(v, n, t, b0, t1, t0, d0, d1, d0, mid);
            Tri(v, n, t, Vector3.up * .5f, t0, t1, Vector3.up, Vector3.up, Vector3.up, Vector3.up);
            Tri(v, n, t, Vector3.down * .5f, b0, b1, Vector3.down, Vector3.down, Vector3.down, Vector3.down);
        }
        return Build(v, n, t);
    }
    static Mesh MakeSphere(int lat = 8, int lon = 12)
    {
        var v = new List<Vector3>(); var n = new List<Vector3>(); var t = new List<int>();
        Vector3 P(int j, int i) { float th = Mathf.PI * j / lat, ph = Mathf.PI * 2 * i / lon; return new Vector3(Mathf.Sin(th) * Mathf.Cos(ph), Mathf.Cos(th), Mathf.Sin(th) * Mathf.Sin(ph)); }
        for (int j = 0; j < lat; j++) for (int i = 0; i < lon; i++)
        {
            Vector3 a = P(j, i), b = P(j, i + 1), c = P(j + 1, i + 1), d = P(j + 1, i); Vector3 mid = (a + b + c + d).normalized;
            Tri(v, n, t, a * .5f, b * .5f, c * .5f, a, b, c, mid); Tri(v, n, t, a * .5f, c * .5f, d * .5f, a, c, d, mid);
        }
        return Build(v, n, t);
    }

    // ---------- أدوات ----------
    static int Key(Color c) { var k = (Color32)c; return (k.r << 16) | (k.g << 8) | k.b; }
    static Color Tint(Color c, float v) { float f = 1f + ((float)rnd.NextDouble() * 2f - 1f) * v; return new Color(Mathf.Clamp01(c.r * f), Mathf.Clamp01(c.g * f), Mathf.Clamp01(c.b * f)); }
    static float R(float a, float b) { return a + (float)rnd.NextDouble() * (b - a); }
    static void D(Mesh m, Vector3 p, Vector3 s, Color c, float ry = 0, float rx = 0, float rz = 0)
    {
        int k = Key(c); if (!batches.ContainsKey(k)) { batches[k] = new List<CombineInstance>(); colors[k] = c; }
        batches[k].Add(new CombineInstance { mesh = m, transform = Matrix4x4.TRS(p, Quaternion.Euler(rx, ry, rz), s) });
    }
    static void Col(Vector3 p, Vector3 size, float ry = 0)
    {
        var g = new GameObject("col"); g.transform.SetParent(root, false); g.transform.SetPositionAndRotation(p, Quaternion.Euler(0, ry, 0)); g.AddComponent<BoxCollider>().size = size;
    }
    static void B(Vector3 center, Vector3 size, Color c, bool solid = true, float ry = 0) { D(cubeM, center, size, c, ry); if (solid) Col(center, size, ry); }
    static void Cyl(Vector3 baseP, float r, float h, Color c, bool solid = true) { D(cylM, baseP + Vector3.up * h / 2, new Vector3(r * 2, h, r * 2), c); if (solid) Col(baseP + Vector3.up * h / 2, new Vector3(r * 1.6f, h, r * 1.6f)); }
    static void Flush()
    {
        foreach (var kv in batches)
        {
            var go = new GameObject("batch"); go.transform.SetParent(root, false);
            var mesh = new Mesh(); mesh.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32; mesh.CombineMeshes(kv.Value.ToArray(), true, true);
            go.AddComponent<MeshFilter>().sharedMesh = mesh; var mr = go.AddComponent<MeshRenderer>(); mr.sharedMaterial = Mat.Get(colors[kv.Key]);
            mr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.On; mr.receiveShadows = true; Rends.Add(mr);
        }
    }

    // ---------- عناصر ----------
    // سقف جملوني (محور الحافة على Z). width على X
    static void Roof(Vector3 center, float w, float len, float rise, Color roof, Color gable)
    {
        float half = w / 2f; float ang = Mathf.Atan2(rise, half) * Mathf.Rad2Deg; float ls = Mathf.Sqrt(half * half + rise * rise) + 0.7f;
        D(cubeM, center + new Vector3(-half / 2f, rise / 2f, 0), new Vector3(ls, 0.3f, len + 1.2f), Tint(roof, .05f), 0, 0, ang);
        D(cubeM, center + new Vector3(half / 2f, rise / 2f, 0), new Vector3(ls, 0.3f, len + 1.2f), Tint(roof, .05f), 0, 0, -ang);
        for (int i = 0; i < 5; i++) { float ww = w * (1f - i / 5f); float hh = rise / 5f; foreach (float sz in new[] { -len / 2f, len / 2f }) D(cubeM, center + new Vector3(0, -0.15f + hh * (i + 0.5f), sz), new Vector3(ww, hh + 0.02f, 0.35f), gable); }
    }
    // مبنى بجدران ببوابة أمامية (قابل للدخول). doorW: عرض فتحة الباب على الجدار الأمامي (z+)
    static void Building(Vector3 c, float w, float len, float h, Color wall, float doorW, float rise, Color roof, Color trim, bool windows)
    {
        float t = 0.4f; Color wc = wall;
        B(c + new Vector3(-w / 2f, h / 2f, 0), new Vector3(t, h, len), Tint(wc, .04f)); B(c + new Vector3(w / 2f, h / 2f, 0), new Vector3(t, h, len), Tint(wc, .04f));
        B(c + new Vector3(0, h / 2f, -len / 2f), new Vector3(w, h, t), Tint(wc, .04f));
        float side = (w - doorW) / 2f;
        B(c + new Vector3(-(doorW / 2f + side / 2f), h / 2f, len / 2f), new Vector3(side, h, t), Tint(wc, .04f)); B(c + new Vector3(doorW / 2f + side / 2f, h / 2f, len / 2f), new Vector3(side, h, t), Tint(wc, .04f));
        B(c + new Vector3(0, h - 0.6f + 0.3f, len / 2f), new Vector3(doorW, 1.2f, t), Tint(wc, .04f), false); // عتبة
        // إطار الباب وزخارف
        B(c + new Vector3(-doorW / 2f, h / 2f, len / 2f + 0.25f), new Vector3(0.35f, h, 0.2f), trim, false); B(c + new Vector3(doorW / 2f, h / 2f, len / 2f + 0.25f), new Vector3(0.35f, h, 0.2f), trim, false);
        B(c + new Vector3(0, 0.15f, 0), new Vector3(w - 0.2f, 0.12f, len - 0.2f), new Color(0.35f, 0.3f, 0.25f), false); // أرضية
        if (windows)
        {
            Color glass = new Color(0.12f, 0.18f, 0.25f);
            for (int i = -1; i <= 1; i += 2) for (float z = -len / 3f; z <= len / 3f; z += len / 3f) { B(c + new Vector3(i * (w / 2f + 0.22f), h * 0.6f, z), new Vector3(0.12f, 1.3f, 1.2f), glass, false); B(c + new Vector3(i * (w / 2f + 0.26f), h * 0.6f, z), new Vector3(0.1f, 0.12f, 1.5f), trim, false); }
        }
        Roof(c + new Vector3(0, h, 0), w, len, rise, roof, wall);
    }
    static void HayRound(Vector3 p) { D(cylM, p + Vector3.up * 0.9f, new Vector3(1.8f, 1.5f, 1.8f), Tint(new Color(0.78f, 0.65f, 0.28f), .1f), 0, 90, R(-8, 8)); Col(p + Vector3.up * 0.9f, new Vector3(1.6f, 1.6f, 1.6f)); }
    static void HayBlock(Vector3 p) { B(p + Vector3.up * 0.4f, new Vector3(1.4f, 0.8f, 0.7f), Tint(new Color(0.8f, 0.68f, 0.3f), .1f), true, R(0, 180)); }
    static void Crate(Vector3 p, float s = 1.2f) { B(p + Vector3.up * s / 2f, new Vector3(s, s, s), Tint(new Color(0.45f, 0.32f, 0.18f), .12f), true, R(0, 90)); }
    static void Tree(Vector3 p, float sc = 1f)
    {
        Cyl(p, 0.35f * sc, 3.2f * sc, Tint(new Color(0.3f, 0.2f, 0.12f), .1f), true);
        for (int i = 0; i < 4; i++) D(sphM, p + new Vector3(R(-1.3f, 1.3f) * sc, (4.2f + R(-0.2f, 1.6f)) * sc, R(-1.3f, 1.3f) * sc), new Vector3(R(3.6f, 5.2f), R(3f, 4.2f), R(3.6f, 5.2f)) * sc, Tint(new Color(0.15f, 0.38f, 0.12f), .22f));
    }
    static void Wheel(Vector3 p, float r, float w) { D(cylM, p, new Vector3(r * 2, w, r * 2), new Color(0.08f, 0.08f, 0.08f), 0, 0, 90); D(cylM, p + new Vector3(Mathf.Sign(p.x) * 0.02f, 0, 0), new Vector3(r * 1.1f, w + 0.05f, r * 1.1f), new Color(0.7f, 0.65f, 0.2f), 0, 0, 90); }
    static void Tractor(Vector3 p, float ry)
    {
        Color green = new Color(0.12f, 0.5f, 0.15f), dark = new Color(0.1f, 0.1f, 0.1f);
        B(p + new Vector3(0, 1.1f, 1.2f), new Vector3(1.5f, 1.0f, 2.6f), green); // غطاء المحرك
        B(p + new Vector3(0, 2.0f, -0.6f), new Vector3(1.7f, 2.0f, 1.7f), green, false); // كابينة
        B(p + new Vector3(0, 2.15f, -0.6f), new Vector3(1.78f, 1.0f, 1.5f), new Color(0.15f, 0.2f, 0.25f), false);
        B(p + new Vector3(0, 3.1f, -0.6f), new Vector3(2.0f, 0.15f, 2.0f), green, false);
        Wheel(p + new Vector3(-1.05f, 0.95f, -0.9f), 0.95f, 0.6f); Wheel(p + new Vector3(1.05f, 0.95f, -0.9f), 0.95f, 0.6f);
        Wheel(p + new Vector3(-0.95f, 0.55f, 1.8f), 0.55f, 0.35f); Wheel(p + new Vector3(0.95f, 0.55f, 1.8f), 0.55f, 0.35f);
        Cyl(p + new Vector3(0.5f, 1.55f, 1.9f), 0.07f, 1.0f, dark, false);
        Col(p + new Vector3(0, 1.3f, 0.3f), new Vector3(2.8f, 2.6f, 4.8f));
    }
    static void Pickup(Vector3 p)
    {
        Color body = new Color(0.55f, 0.18f, 0.12f), dark = new Color(0.08f, 0.08f, 0.08f);
        B(p + new Vector3(0, 0.85f, 0), new Vector3(2.0f, 0.8f, 5.0f), body, false); B(p + new Vector3(0, 1.6f, 0.9f), new Vector3(1.9f, 0.8f, 2.0f), body, false);
        B(p + new Vector3(0, 1.62f, 0.9f), new Vector3(1.95f, 0.5f, 1.7f), new Color(0.15f, 0.2f, 0.25f), false);
        for (int i = -1; i <= 1; i += 2) { Wheel(p + new Vector3(i * 1.0f, 0.45f, 1.6f), 0.45f, 0.35f); Wheel(p + new Vector3(i * 1.0f, 0.45f, -1.5f), 0.45f, 0.35f); }
        Col(p + new Vector3(0, 1.1f, 0), new Vector3(2.1f, 2.0f, 5.0f));
    }
    static void Fence(Vector3 a, Vector3 b, bool solid = false)
    {
        Vector3 d = b - a; float len = d.magnitude; Vector3 dir = d / len; float ry = Mathf.Atan2(dir.x, dir.z) * Mathf.Rad2Deg; Color wood = new Color(0.4f, 0.3f, 0.2f);
        for (float s = 0; s <= len + 0.01f; s += 3f) B(a + dir * s + Vector3.up * 0.6f, new Vector3(0.16f, 1.2f, 0.16f), Tint(wood, .1f), false);
        Vector3 mid = (a + b) / 2f; D(cubeM, mid + Vector3.up * 1.0f, new Vector3(0.08f, 0.12f, len), wood, ry); D(cubeM, mid + Vector3.up * 0.55f, new Vector3(0.08f, 0.12f, len), wood, ry);
        if (solid) Col(mid + Vector3.up * 0.6f, new Vector3(0.3f, 1.2f, len), ry);
    }
    static void Wheat(float x0, float x1, float z0, float z1)
    {
        Color baseC = new Color(0.78f, 0.66f, 0.25f);
        B(new Vector3((x0 + x1) / 2f, 0.03f, (z0 + z1) / 2f), new Vector3(x1 - x0, 0.06f, z1 - z0), new Color(0.32f, 0.24f, 0.14f), false);
        for (float x = x0 + 0.5f; x < x1; x += 0.9f) for (float z = z0; z < z1 - 0.1f; z += 4f)
        { float h = R(0.75f, 1.0f); D(cubeM, new Vector3(x, h / 2f, Mathf.Min(z + 2f, z1 - 0.5f)), new Vector3(0.55f, h, Mathf.Min(4f, z1 - z) - 0.1f), Tint(baseC, .14f)); }
    }
    static void Corn(float x0, float x1, float z0, float z1)
    {
        Color leaf = new Color(0.22f, 0.45f, 0.12f), tassel = new Color(0.75f, 0.62f, 0.3f);
        B(new Vector3((x0 + x1) / 2f, 0.03f, (z0 + z1) / 2f), new Vector3(x1 - x0, 0.06f, z1 - z0), new Color(0.3f, 0.22f, 0.13f), false);
        for (float x = x0 + 0.6f; x < x1; x += 1.5f) for (float z = z0; z < z1 - 0.1f; z += 2.6f)
        { float h = R(1.7f, 2.2f); float zc = Mathf.Min(z + 1.3f, z1 - 0.4f); D(cubeM, new Vector3(x, h / 2f, zc), new Vector3(0.45f, h, 2.3f), Tint(leaf, .18f)); D(cubeM, new Vector3(x, h + 0.1f, zc), new Vector3(0.3f, 0.25f, 2.2f), Tint(tassel, .1f)); }
    }
    static void Tilled(float x0, float x1, float z0, float z1)
    {
        B(new Vector3((x0 + x1) / 2f, 0.03f, (z0 + z1) / 2f), new Vector3(x1 - x0, 0.06f, z1 - z0), new Color(0.38f, 0.27f, 0.17f), false);
        for (float x = x0 + 0.6f; x < x1; x += 1.3f) B(new Vector3(x, 0.14f, (z0 + z1) / 2f), new Vector3(0.55f, 0.22f, z1 - z0 - 1f), Tint(new Color(0.3f, 0.21f, 0.13f), .15f), false);
    }

    // ---------- بناء الخريطة ----------
    public static Renderer Build(Transform parent)
    {
        root = new GameObject("Farm").transform; root.SetParent(parent, false); rnd = new System.Random(11);
        batches = new Dictionary<int, List<CombineInstance>>(); colors = new Dictionary<int, Color>(); Rends.Clear();
        cubeM = MakeCube(); cylM = MakeCyl(); sphM = MakeSphere();

        // الأرض: كتلة كبيرة بنسيج عشب/تراب مولّد
        var tex = new Texture2D(128, 128, TextureFormat.RGB24, true); tex.wrapMode = TextureWrapMode.Repeat;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++)
        { float n = Mathf.PerlinNoise(x * 0.09f, y * 0.09f) * 0.6f + Mathf.PerlinNoise(x * 0.4f + 9, y * 0.4f) * 0.4f; Color c = Color.Lerp(new Color(0.34f, 0.4f, 0.16f), new Color(0.5f, 0.5f, 0.24f), n); tex.SetPixel(x, y, c); }
        tex.Apply();
        var gnd = GameObject.CreatePrimitive(PrimitiveType.Cube); gnd.name = "Ground"; gnd.transform.SetParent(root, false);
        gnd.transform.position = new Vector3(0, -0.5f, 0); gnd.transform.localScale = new Vector3(170, 1, 170);
        var gr = gnd.GetComponent<Renderer>(); gr.sharedMaterial = Mat.Textured(Color.white, tex, new Vector2(34, 34));

        // طرق ترابية
        Color dirt = new Color(0.55f, 0.45f, 0.3f);
        B(new Vector3(0, 0.025f, 0), new Vector3(5.5f, 0.05f, 165f), dirt, false); B(new Vector3(0, 0.03f, 0), new Vector3(165f, 0.05f, 5.5f), dirt, false);
        for (int i = -1; i <= 1; i += 2) { B(new Vector3(i * 1.4f, 0.06f, 0), new Vector3(0.4f, 0.03f, 165f), new Color(0.42f, 0.34f, 0.22f), false); B(new Vector3(0, 0.07f, i * 1.4f), new Vector3(165f, 0.03f, 0.4f), new Color(0.42f, 0.34f, 0.22f), false); }

        // حقول
        Wheat(-70, -30, -70, -40); Corn(24, 70, -70, -38); Wheat(-70, -30, 40, 72); Tilled(24, 70, 40, 72);
        // أسوار الحقول
        Fence(new Vector3(-72, 0, -38), new Vector3(-6, 0, -38)); Fence(new Vector3(6, 0, -38), new Vector3(72, 0, -38));
        Fence(new Vector3(-72, 0, 38), new Vector3(-6, 0, 38)); Fence(new Vector3(6, 0, 38), new Vector3(72, 0, 38));

        // المزرعة
        Building(new Vector3(-24, 0, 4), 14f, 22f, 6.2f, new Color(0.55f, 0.13f, 0.1f), 6f, 4.6f, new Color(0.32f, 0.32f, 0.34f), new Color(0.92f, 0.92f, 0.88f), false); // الحظيرة
        Building(new Vector3(24, 0, -8), 9f, 11f, 3.6f, new Color(0.85f, 0.8f, 0.68f), 2.4f, 3f, new Color(0.45f, 0.18f, 0.12f), new Color(0.95f, 0.95f, 0.92f), true); // المنزل
        Cyl(new Vector3(-40, 0, 14), 3.2f, 13f, new Color(0.62f, 0.66f, 0.7f)); D(sphM, new Vector3(-40, 13f, 14), new Vector3(6.4f, 4f, 6.4f), new Color(0.5f, 0.54f, 0.58f));
        Cyl(new Vector3(-34, 0, 18), 3.2f, 11f, new Color(0.62f, 0.66f, 0.7f)); D(sphM, new Vector3(-34, 11f, 18), new Vector3(6.4f, 4f, 6.4f), new Color(0.5f, 0.54f, 0.58f));
        for (float y = 3; y < 12; y += 3.2f) D(cylM, new Vector3(-40, y, 14), new Vector3(6.6f, 0.2f, 6.6f), new Color(0.4f, 0.42f, 0.45f));
        Tractor(new Vector3(8, 0, 10), 0); Pickup(new Vector3(14, 0, -16));
        // بقايا مركبة محطمة كغطاء
        B(new Vector3(-8, 0.7f, -14), new Vector3(2.0f, 1.2f, 4.4f), new Color(0.3f, 0.3f, 0.28f), true, 25); B(new Vector3(-8.2f, 1.45f, -14.2f), new Vector3(1.8f, 0.5f, 2.0f), new Color(0.2f, 0.2f, 0.2f), false, 25);
        // برج المياه
        for (int i = -1; i <= 1; i += 2) for (int j = -1; j <= 1; j += 2) B(new Vector3(38 + i * 1.3f, 4f, 6 + j * 1.3f), new Vector3(0.3f, 8f, 0.3f), new Color(0.35f, 0.25f, 0.18f));
        Cyl(new Vector3(38, 8f, 6), 2.2f, 3f, new Color(0.45f, 0.35f, 0.25f), false); D(sphM, new Vector3(38, 11f, 6), new Vector3(4.4f, 2.4f, 4.4f), new Color(0.4f, 0.3f, 0.22f));
        // حظيرة الحيوانات (سياج) وحوض
        Fence(new Vector3(30, 0, 18), new Vector3(46, 0, 18), true); Fence(new Vector3(46, 0, 18), new Vector3(46, 0, 30), true); Fence(new Vector3(46, 0, 30), new Vector3(30, 0, 30), true); Fence(new Vector3(30, 0, 30), new Vector3(30, 0, 18), true);
        B(new Vector3(38, 0.4f, 24), new Vector3(3.5f, 0.8f, 1f), new Color(0.35f, 0.3f, 0.25f));
        // بركة
        D(cylM, new Vector3(-48, 0.05f, -12), new Vector3(15, 0.1f, 15), new Color(0.16f, 0.32f, 0.38f));
        // قش وصناديق وجدران حجرية للغطاء
        for (int i = 0; i < 12; i++) HayRound(new Vector3(R(-60, -32), 0, R(-34, -4)) + (i % 2 == 0 ? Vector3.zero : new Vector3(60, 0, 40)));
        for (int i = 0; i < 10; i++) HayBlock(new Vector3(-24 + R(-4, 4), 0, 6 + R(-8, 8)));
        for (int i = 0; i < 8; i++) Crate(new Vector3(R(-18, 18), 0, R(-26, 26)));
        Color stone = new Color(0.5f, 0.48f, 0.44f);
        for (int i = 0; i < 9; i++) B(new Vector3(R(-50, 50), 0.55f, R(-30, 30)), new Vector3(R(4, 8), 1.1f, 0.7f), Tint(stone, .1f), true, R(0, 180));
        for (int i = 0; i < 22; i++) { float r = R(0.5f, 1.4f); D(sphM, new Vector3(R(-72, 72), r * 0.25f, R(-72, 72)), new Vector3(r * 2, r, r * 1.8f), Tint(new Color(0.46f, 0.45f, 0.42f), .15f)); }
        // أشجار: خط حدودي + مجموعات
        for (float s = -74; s <= 74; s += 8f) { Tree(new Vector3(-76 + R(-1.5f, 1.5f), 0, s + R(-2, 2)), R(0.9f, 1.3f)); Tree(new Vector3(76 + R(-1.5f, 1.5f), 0, s + R(-2, 2)), R(0.9f, 1.3f)); }
        for (float s = -66; s <= 66; s += 9f) { Tree(new Vector3(s + R(-2, 2), 0, -76 + R(-1.5f, 1.5f)), R(0.9f, 1.3f)); Tree(new Vector3(s + R(-2, 2), 0, 76 + R(-1.5f, 1.5f)), R(0.9f, 1.3f)); }
        foreach (var tp in new[] { new Vector3(32, 0, -22), new Vector3(36, 0, -26), new Vector3(30, 0, 2), new Vector3(-10, 0, 20), new Vector3(-6, 0, 26), new Vector3(10, 0, 30), new Vector3(-58, 0, 12), new Vector3(-55, 0, 20) }) Tree(tp, R(1f, 1.4f));
        // سياج نباتي حدودي (صلب)
        Color hedge = new Color(0.16f, 0.34f, 0.12f);
        B(new Vector3(0, 1.2f, 82), new Vector3(168, 2.4f, 1.6f), Tint(hedge, .1f)); B(new Vector3(0, 1.2f, -82), new Vector3(168, 2.4f, 1.6f), Tint(hedge, .1f));
        B(new Vector3(82, 1.2f, 0), new Vector3(1.6f, 2.4f, 168), Tint(hedge, .1f)); B(new Vector3(-82, 1.2f, 0), new Vector3(1.6f, 2.4f, 168), Tint(hedge, .1f));
        // سحب
        for (int i = 0; i < 16; i++) D(sphM, new Vector3(R(-150, 150), R(85, 120), R(-150, 150)), new Vector3(R(25, 50), R(6, 10), R(15, 30)), new Color(0.97f, 0.97f, 1f));

        Flush(); return gr;
    }
}
