using System.Collections.Generic;
using UnityEngine;

// خريطة "موقع صحراوي": حاويات ومباني طينية وأسوار ومنصات ونخيل. تُبنى من مجسمات بسيطة بمصادم لكل صلب.
public static class DesertMap
{
    static Transform root; static System.Random rnd;
    public static List<Renderer> Rends = new List<Renderer>();
    static float R(float a, float b) { return a + (float)rnd.NextDouble() * (b - a); }
    static Color Tint(Color c, float v) { float f = 1f + (R(-1, 1)) * v; return new Color(Mathf.Clamp01(c.r * f), Mathf.Clamp01(c.g * f), Mathf.Clamp01(c.b * f)); }
    static GameObject Prim(PrimitiveType t, Vector3 p, Vector3 s, Color c, bool solid, float ry = 0)
    {
        var g = GameObject.CreatePrimitive(t); g.transform.SetParent(root, false); g.transform.position = p; g.transform.localScale = s; g.transform.rotation = Quaternion.Euler(0, ry, 0);
        Mat.Set(g, c); if (!solid) Object.Destroy(g.GetComponent<Collider>()); else g.isStatic = true; Rends.Add(g.GetComponent<Renderer>()); return g;
    }
    static void Box(Vector3 c, Vector3 s, Color col, float ry = 0) { Prim(PrimitiveType.Cube, c, s, col, true, ry); }
    static void Container(Vector3 p, float ry)
    {
        Color[] cols = { new Color(0.65f, 0.2f, 0.15f), new Color(0.2f, 0.35f, 0.55f), new Color(0.75f, 0.55f, 0.15f), new Color(0.3f, 0.4f, 0.3f) };
        Box(p + Vector3.up * 1.3f, new Vector3(2.5f, 2.6f, 6f), Tint(cols[rnd.Next(cols.Length)], 0.1f), ry);
    }
    static void Mud(Vector3 p, float w, float d, float h)
    {
        Color wall = Tint(new Color(0.78f, 0.64f, 0.42f), 0.08f);
        Box(p + Vector3.up * h / 2, new Vector3(w, h, d), wall);
        Prim(PrimitiveType.Cube, p + Vector3.up * (h + 0.15f), new Vector3(w + 0.4f, 0.3f, d + 0.4f), new Color(0.62f, 0.5f, 0.32f), false);
        Prim(PrimitiveType.Cube, p + new Vector3(0, 1.1f, d / 2 + 0.02f), new Vector3(1.4f, 2.2f, 0.06f), new Color(0.25f, 0.17f, 0.1f), false);
        Prim(PrimitiveType.Cube, p + new Vector3(w * 0.3f, 2.2f, d / 2 + 0.02f), new Vector3(1.1f, 0.9f, 0.06f), new Color(0.15f, 0.2f, 0.28f), false);
    }
    static void Palm(Vector3 p)
    {
        float h = R(5f, 8f);
        Prim(PrimitiveType.Cylinder, p + Vector3.up * h / 2, new Vector3(0.45f, h / 2, 0.45f), new Color(0.45f, 0.32f, 0.2f), true);
        for (int i = 0; i < 6; i++) { var l = Prim(PrimitiveType.Cube, p + Vector3.up * h + Quaternion.Euler(0, i * 60, 0) * new Vector3(0, -0.1f, 1.2f), new Vector3(0.35f, 0.08f, 2.6f), new Color(0.2f, 0.45f, 0.18f), false, i * 60); l.transform.rotation = Quaternion.Euler(18, i * 60, 0); }
    }
    static void Wall(Vector3 a, Vector3 b, float h, Color c)
    {
        Vector3 m = (a + b) / 2; float len = Vector3.Distance(a, b); float ang = Mathf.Atan2(b.x - a.x, b.z - a.z) * Mathf.Rad2Deg;
        Box(m + Vector3.up * h / 2, new Vector3(0.6f, h, len), c, ang);
    }
    static void Tower(Vector3 p)
    {
        Color c = new Color(0.55f, 0.5f, 0.42f);
        Box(p + Vector3.up * 4f, new Vector3(4f, 8f, 4f), c);
        Box(p + Vector3.up * 8.3f, new Vector3(5.2f, 0.6f, 5.2f), new Color(0.4f, 0.36f, 0.3f));
        Prim(PrimitiveType.Cube, p + Vector3.up * 9.5f, new Vector3(4.6f, 1.8f, 0.3f), new Color(0.5f, 0.46f, 0.38f), false);
    }

    public static Renderer Build(Transform parent)
    {
        root = parent; rnd = new System.Random(2025); Rends.Clear();
        var ground = GameObject.CreatePrimitive(PrimitiveType.Cube); ground.transform.SetParent(root, false); ground.name = "Ground";
        ground.transform.position = new Vector3(0, -0.5f, 0); ground.transform.localScale = new Vector3(260, 1, 260); Mat.Set(ground, new Color(0.82f, 0.7f, 0.46f));
        // حدود الخريطة
        Color wallC = new Color(0.7f, 0.6f, 0.42f);
        Wall(new Vector3(-85, 0, -85), new Vector3(85, 0, -85), 4, wallC); Wall(new Vector3(-85, 0, 85), new Vector3(85, 0, 85), 4, wallC);
        Wall(new Vector3(-85, 0, -85), new Vector3(-85, 0, 85), 4, wallC); Wall(new Vector3(85, 0, -85), new Vector3(85, 0, 85), 4, wallC);
        // قلب الخريطة: مباني طينية وحاويات
        for (int i = 0; i < 18; i++)
        {
            Vector3 p = new Vector3(R(-65, 65), 0, R(-30, 30)); if (Mathf.Abs(p.x) < 6 && Mathf.Abs(p.z) < 6) continue;
            if (i % 3 == 0) Mud(p, R(6, 10), R(6, 9), R(3.2f, 4.2f)); else Container(p, rnd.Next(2) * 90 + R(-8, 8));
        }
        // أكياس رمل وصناديق قرب خطوط التمركز
        for (int i = 0; i < 40; i++)
        {
            Vector3 p = new Vector3(R(-75, 75), 0, R(-75, 75)); float s = R(1.1f, 1.8f);
            if (rnd.Next(2) == 0) Box(p + Vector3.up * s / 2, new Vector3(s * 1.8f, s, s), Tint(new Color(0.65f, 0.58f, 0.4f), 0.1f), R(0, 180));
            else Box(p + Vector3.up * s / 2, new Vector3(s, s, s), Tint(new Color(0.4f, 0.3f, 0.2f), 0.12f), R(0, 90));
        }
        // جدران قصيرة للتغطية
        for (int i = 0; i < 10; i++) { float z = R(-60, 60), x = R(-60, 55); Wall(new Vector3(x, 0, z), new Vector3(x + R(8, 14), 0, z + R(-3, 3)), 1.6f, wallC); }
        Tower(new Vector3(-40, 0, 0)); Tower(new Vector3(40, 0, 0)); Tower(new Vector3(0, 0, -20)); Tower(new Vector3(0, 0, 20));
        for (int i = 0; i < 30; i++) Palm(new Vector3(R(-80, 80), 0, R(-80, 80)));
        return ground.GetComponent<Renderer>();
    }
}
