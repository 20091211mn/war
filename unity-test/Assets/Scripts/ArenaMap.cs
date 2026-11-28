using System.Collections.Generic;
using UnityEngine;

// خرائط الساحة: 1 صحراء، 2 ثلج، 3 مدينة مدمّرة. مجسمات بسيطة بمصادم لكل صلب، وتُدمج بالـStatic Batching لتقليل عدد الرسم.
public static class ArenaMap
{
    static Transform root; static System.Random rnd;
    public static List<Renderer> Rends = new List<Renderer>();
    static float R(float a, float b) { return a + (float)rnd.NextDouble() * (b - a); }
    static Color Tint(Color c, float v) { float f = 1f + R(-1, 1) * v; return new Color(Mathf.Clamp01(c.r * f), Mathf.Clamp01(c.g * f), Mathf.Clamp01(c.b * f)); }
    static GameObject Prim(PrimitiveType t, Vector3 p, Vector3 s, Color c, bool solid, float ry = 0)
    {
        var g = GameObject.CreatePrimitive(t); g.transform.SetParent(root, false); g.transform.position = p; g.transform.localScale = s; g.transform.rotation = Quaternion.Euler(0, ry, 0);
        Mat.Set(g, c); if (!solid) Object.Destroy(g.GetComponent<Collider>()); Rends.Add(g.GetComponent<Renderer>()); return g;
    }
    static void Box(Vector3 c, Vector3 s, Color col, float ry = 0) { Prim(PrimitiveType.Cube, c, s, col, true, ry); }
    static void Deco(Vector3 c, Vector3 s, Color col, float ry = 0) { Prim(PrimitiveType.Cube, c, s, col, false, ry); }
    static void Wall(Vector3 a, Vector3 b, float h, Color c, float th = 0.6f)
    {
        Vector3 m = (a + b) / 2; float len = Vector3.Distance(a, b); float ang = Mathf.Atan2(b.x - a.x, b.z - a.z) * Mathf.Rad2Deg;
        Box(m + Vector3.up * h / 2, new Vector3(th, h, len), c, ang);
    }
    static void Container(Vector3 p, float ry)
    {
        Color[] cols = { new Color(0.65f, 0.2f, 0.15f), new Color(0.2f, 0.35f, 0.55f), new Color(0.75f, 0.55f, 0.15f), new Color(0.3f, 0.4f, 0.3f) };
        Color c = Tint(cols[rnd.Next(cols.Length)], 0.1f);
        Box(p + Vector3.up * 1.3f, new Vector3(2.5f, 2.6f, 6f), c, ry);
        for (int i = -2; i <= 2; i++) Deco(p + Quaternion.Euler(0, ry, 0) * new Vector3(1.27f, 1.3f, i * 1.1f), new Vector3(0.05f, 2.4f, 0.12f), c * 0.75f, ry);
    }
    static void Sandbag(Vector3 p)
    {
        float s = R(1.0f, 1.5f); Color c = Tint(new Color(0.62f, 0.56f, 0.4f), 0.1f); float ry = R(0, 180);
        Box(p + Vector3.up * s * 0.3f, new Vector3(s * 2f, s * 0.6f, s * 0.8f), c, ry); Box(p + Vector3.up * s * 0.8f, new Vector3(s * 1.5f, s * 0.5f, s * 0.7f), c * 0.95f, ry);
    }
    static void Crate(Vector3 p) { float s = R(1.0f, 1.8f); Box(p + Vector3.up * s / 2, new Vector3(s, s, s), Tint(new Color(0.42f, 0.3f, 0.18f), 0.12f), R(0, 90)); }
    static void Mud(Vector3 p, float w, float d, float h, Color wall)
    {
        Box(p + Vector3.up * h / 2, new Vector3(w, h, d), Tint(wall, 0.08f));
        Deco(p + Vector3.up * (h + 0.15f), new Vector3(w + 0.4f, 0.3f, d + 0.4f), wall * 0.82f);
        Deco(p + new Vector3(0, 1.1f, d / 2 + 0.02f), new Vector3(1.4f, 2.2f, 0.06f), new Color(0.25f, 0.17f, 0.1f));
        Deco(p + new Vector3(w * 0.3f, 2.2f, d / 2 + 0.02f), new Vector3(1.1f, 0.9f, 0.06f), new Color(0.15f, 0.2f, 0.28f));
    }
    static void Palm(Vector3 p)
    {
        float h = R(5f, 8f);
        Prim(PrimitiveType.Cylinder, p + Vector3.up * h / 2, new Vector3(0.45f, h / 2, 0.45f), new Color(0.45f, 0.32f, 0.2f), true);
        for (int i = 0; i < 6; i++) { var l = Prim(PrimitiveType.Cube, p + Vector3.up * h + Quaternion.Euler(0, i * 60, 0) * new Vector3(0, -0.1f, 1.2f), new Vector3(0.35f, 0.08f, 2.6f), new Color(0.2f, 0.45f, 0.18f), false, i * 60); l.transform.rotation = Quaternion.Euler(18, i * 60, 0); }
    }
    static void Pine(Vector3 p)
    {
        float h = R(6f, 10f);
        Prim(PrimitiveType.Cylinder, p + Vector3.up * 1.2f, new Vector3(0.5f, 1.2f, 0.5f), new Color(0.3f, 0.2f, 0.12f), true);
        Color g = Tint(new Color(0.13f, 0.32f, 0.2f), 0.15f);
        for (int i = 0; i < 4; i++) { float k = 1f - i * 0.22f; Prim(PrimitiveType.Sphere, p + Vector3.up * (2.2f + i * h * 0.2f), new Vector3(4f * k, 2.6f, 4f * k), i == 3 ? new Color(0.9f, 0.93f, 0.96f) : g, false); }
    }
    static void Tower(Vector3 p, Color c)
    {
        Box(p + Vector3.up * 4f, new Vector3(4f, 8f, 4f), c);
        Box(p + Vector3.up * 8.3f, new Vector3(5.2f, 0.6f, 5.2f), c * 0.8f);
        Deco(p + Vector3.up * 9.5f, new Vector3(4.6f, 1.8f, 0.3f), c * 0.9f);
    }
    static void Bunker(Vector3 p, float ry)
    {
        Color c = new Color(0.55f, 0.57f, 0.6f);
        Box(p + Vector3.up * 1.4f, new Vector3(8f, 2.8f, 5f), c, ry);
        Deco(p + Vector3.up * 2.95f, new Vector3(8.4f, 0.3f, 5.4f), c * 0.7f, ry);
        Deco(p + Quaternion.Euler(0, ry, 0) * new Vector3(0, 1.0f, 2.52f), new Vector3(1.6f, 2f, 0.1f), new Color(0.12f, 0.12f, 0.14f), ry);
    }
    static void Building(Vector3 p, float w, float d, float h, Color wall)
    {
        Box(p + Vector3.up * h / 2, new Vector3(w, h, d), wall);
        Deco(p + Vector3.up * (h + 0.2f), new Vector3(w + 0.3f, 0.4f, d + 0.3f), wall * 0.7f);
        Color win = new Color(0.1f, 0.13f, 0.18f);
        for (int r = 0; r < Mathf.FloorToInt((h - 2f) / 3f); r++)
            for (int i = -1; i <= 1; i++)
            {
                float y = 2.5f + r * 3f; if (w > 6) { Deco(p + new Vector3(i * w * 0.28f, y, d / 2 + 0.03f), new Vector3(1.3f, 1.4f, 0.06f), win); Deco(p + new Vector3(i * w * 0.28f, y, -d / 2 - 0.03f), new Vector3(1.3f, 1.4f, 0.06f), win); }
            }
    }
    static void Wreck(Vector3 p, float ry)
    {
        Color c = Tint(new Color(0.35f, 0.3f, 0.28f), 0.2f);
        Box(p + Vector3.up * 0.5f, new Vector3(1.9f, 0.8f, 4.2f), c, ry); Box(p + Quaternion.Euler(0, ry, 0) * new Vector3(0, 1.1f, -0.2f), new Vector3(1.6f, 0.6f, 2.0f), c * 0.8f, ry);
    }

    // style: 1 صحراء، 2 ثلج، 3 مدينة
    public static Renderer Build(Transform parent, int style)
    {
        root = parent; rnd = new System.Random(2025 + style); Rends.Clear();
        Color groundC = style == 1 ? new Color(0.82f, 0.7f, 0.46f) : style == 2 ? new Color(0.9f, 0.93f, 0.96f) : new Color(0.36f, 0.37f, 0.38f);
        Color wallC = style == 1 ? new Color(0.7f, 0.6f, 0.42f) : style == 2 ? new Color(0.45f, 0.48f, 0.52f) : new Color(0.5f, 0.48f, 0.45f);
        var ground = GameObject.CreatePrimitive(PrimitiveType.Cube); ground.transform.SetParent(root, false); ground.name = "Ground";
        ground.transform.position = new Vector3(0, -0.5f, 0); ground.transform.localScale = new Vector3(260, 1, 260); Mat.Set(ground, groundC);
        Wall(new Vector3(-85, 0, -85), new Vector3(85, 0, -85), 4, wallC); Wall(new Vector3(-85, 0, 85), new Vector3(85, 0, 85), 4, wallC);
        Wall(new Vector3(-85, 0, -85), new Vector3(-85, 0, 85), 4, wallC); Wall(new Vector3(85, 0, -85), new Vector3(85, 0, 85), 4, wallC);

        if (style == 1)
        {
            for (int i = 0; i < 18; i++) { Vector3 p = new Vector3(R(-65, 65), 0, R(-30, 30)); if (Mathf.Abs(p.x) < 6 && Mathf.Abs(p.z) < 6) continue; if (i % 3 == 0) Mud(p, R(6, 10), R(6, 9), R(3.2f, 4.2f), new Color(0.78f, 0.64f, 0.42f)); else Container(p, rnd.Next(2) * 90 + R(-8, 8)); }
            for (int i = 0; i < 40; i++) { Vector3 p = new Vector3(R(-75, 75), 0, R(-75, 75)); if (rnd.Next(2) == 0) Sandbag(p); else Crate(p); }
            for (int i = 0; i < 10; i++) { float z = R(-60, 60), x = R(-60, 55); Wall(new Vector3(x, 0, z), new Vector3(x + R(8, 14), 0, z + R(-3, 3)), 1.6f, wallC); }
            Tower(new Vector3(-40, 0, 0), new Color(0.55f, 0.5f, 0.42f)); Tower(new Vector3(40, 0, 0), new Color(0.55f, 0.5f, 0.42f)); Tower(new Vector3(0, 0, -20), new Color(0.55f, 0.5f, 0.42f)); Tower(new Vector3(0, 0, 20), new Color(0.55f, 0.5f, 0.42f));
            for (int i = 0; i < 30; i++) Palm(new Vector3(R(-80, 80), 0, R(-80, 80)));
        }
        else if (style == 2)
        {
            for (int i = 0; i < 9; i++) Bunker(new Vector3(R(-65, 65), 0, R(-28, 28)), rnd.Next(2) * 90);
            for (int i = 0; i < 14; i++) Container(new Vector3(R(-70, 70), 0, R(-35, 35)), rnd.Next(2) * 90 + R(-10, 10));
            for (int i = 0; i < 34; i++) Sandbag(new Vector3(R(-75, 75), 0, R(-75, 75)));
            for (int i = 0; i < 14; i++) Crate(new Vector3(R(-75, 75), 0, R(-60, 60)));
            for (int i = 0; i < 10; i++) { float z = R(-60, 60), x = R(-60, 55); Wall(new Vector3(x, 0, z), new Vector3(x + R(8, 14), 0, z + R(-3, 3)), 1.8f, wallC, 0.8f); }
            for (int i = 0; i < 36; i++) Pine(new Vector3(R(-80, 80), 0, R(-80, 80)));
            for (int i = 0; i < 25; i++) Deco(new Vector3(R(-78, 78), 0.05f, R(-78, 78)), new Vector3(R(3, 7), 0.12f, R(3, 7)), new Color(1f, 1f, 1f), R(0, 180));
            Tower(new Vector3(-45, 0, 8), new Color(0.4f, 0.43f, 0.48f)); Tower(new Vector3(45, 0, -8), new Color(0.4f, 0.43f, 0.48f));
        }
        else
        {
            // شوارع شبكية: مباني عالية بينها ممرات، وسيارات محطمة وحواجز
            for (int gx = -3; gx <= 3; gx++)
                for (int gz = -1; gz <= 1; gz++)
                {
                    if (rnd.Next(5) == 0) continue;
                    Vector3 p = new Vector3(gx * 22f + R(-2, 2), 0, gz * 24f + R(-2, 2));
                    Building(p, R(10, 14), R(10, 14), R(7, 16), Tint(new Color(0.55f, 0.5f, 0.45f), 0.18f));
                }
            for (int i = 0; i < 16; i++) Wreck(new Vector3(R(-75, 75), 0, R(-75, 75)), R(0, 180));
            for (int i = 0; i < 30; i++) Sandbag(new Vector3(R(-75, 75), 0, R(-75, 75)));
            for (int i = 0; i < 12; i++) Crate(new Vector3(R(-75, 75), 0, R(-75, 75)));
            for (int i = 0; i < 12; i++) { float z = R(-70, 70), x = R(-70, 62); Wall(new Vector3(x, 0, z), new Vector3(x + R(6, 12), 0, z + R(-2, 2)), 1.7f, wallC); }
            for (int i = 0; i < 8; i++) { var p = new Vector3(R(-80, 80), 0, R(-80, 80)); Prim(PrimitiveType.Cylinder, p + Vector3.up * 3.5f, new Vector3(0.25f, 3.5f, 0.25f), new Color(0.25f, 0.25f, 0.27f), true); }
        }
        try { StaticBatchingUtility.Combine(root.gameObject); } catch { }
        return ground.GetComponent<Renderer>();
    }
}
