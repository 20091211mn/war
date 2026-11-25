#if UNITY_EDITOR
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

// Warzone > Setup: ينشئ مشهداً فارغاً ويضيفه لإعدادات البناء ويضبط الجوال
[InitializeOnLoad]
public static class Setup
{
    // يعمل تلقائياً عند فتح المشروع (يشمل Unity Cloud): يضمن وجود المشهد في إعدادات البناء ونظام الإدخال Both
    static Setup()
    {
        EditorApplication.delayCall += () =>
        {
            if (!System.IO.File.Exists("Assets/Scenes/Main.unity")) { CreateScene(); return; }
            if (EditorBuildSettings.scenes.Length == 0) EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene("Assets/Scenes/Main.unity", true) };
            var ps = AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/ProjectSettings.asset");
            if (ps != null && ps.Length > 0) { var o = new SerializedObject(ps[0]); var p = o.FindProperty("activeInputHandler"); if (p != null && p.intValue != 2) { p.intValue = 2; o.ApplyModifiedProperties(); AssetDatabase.SaveAssets(); } }
        };
    }

    [MenuItem("Warzone/Setup Scene and Android")]
    public static void Run() { CreateScene(); EditorUtility.DisplayDialog("Warzone", "Done. Press Play. (If you get an Input error: Edit > Project Settings > Player > Active Input Handling = Both)", "OK"); }
    public static void CreateScene()
    {
        // إبقاء الـShaders في البناء (وإلا تظهر المجسمات وردية)
        var gsa = AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/GraphicsSettings.asset");
        if (gsa != null && gsa.Length > 0) AddShaders(gsa[0]);
        // نظام الإدخال القديم + الجديد معاً (Unity 6 يجعل الجديد فقط افتراضياً فيفشل Input.GetAxis)
        var ps = AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/ProjectSettings.asset");
        if (ps != null && ps.Length > 0) { var pso = new SerializedObject(ps[0]); var p = pso.FindProperty("activeInputHandler"); if (p != null) { p.intValue = 2; pso.ApplyModifiedProperties(); } }
        CreateSceneOnly();
    }
    static void AddShaders(Object gs)
    {
        var so = new SerializedObject(gs); var arr = so.FindProperty("m_AlwaysIncludedShaders");
        foreach (var n in new[] { "Standard", "Unlit/Color", "Sprites/Default", "Legacy Shaders/Diffuse" })
        {
            var s = Shader.Find(n); if (s == null) continue; bool has = false;
            for (int i = 0; i < arr.arraySize; i++) if (arr.GetArrayElementAtIndex(i).objectReferenceValue == s) has = true;
            if (!has) { arr.arraySize++; arr.GetArrayElementAtIndex(arr.arraySize - 1).objectReferenceValue = s; }
        }
        so.ApplyModifiedProperties(); AssetDatabase.SaveAssets();
    }
    static void CreateSceneOnly()
    {
        var sc = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
        System.IO.Directory.CreateDirectory("Assets/Scenes");
        EditorSceneManager.SaveScene(sc, "Assets/Scenes/Main.unity");
        EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene("Assets/Scenes/Main.unity", true) };
        PlayerSettings.companyName = "Warzone"; PlayerSettings.productName = "Warzone Unity Test";
        PlayerSettings.defaultInterfaceOrientation = UIOrientation.LandscapeLeft;
        PlayerSettings.SetApplicationIdentifier(BuildTargetGroup.Android, "com.warzone.unitytest");
    }
}
#endif
