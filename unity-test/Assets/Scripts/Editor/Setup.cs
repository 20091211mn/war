#if UNITY_EDITOR
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

// Warzone > Setup: ينشئ مشهداً فارغاً ويضيفه لإعدادات البناء ويضبط الجوال
public static class Setup
{
    [MenuItem("Warzone/Setup Scene and Android")]
    public static void Run() { CreateScene(); EditorUtility.DisplayDialog("Warzone", "Done. Press Play. (If you get an Input error: Edit > Project Settings > Player > Active Input Handling = Both)", "OK"); }
    public static void CreateScene()
    {
        // إبقاء الـShaders في البناء (وإلا تظهر المجسمات وردية)
        var gsa = AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/GraphicsSettings.asset");
        if (gsa != null && gsa.Length > 0) AddShaders(gsa[0]);
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
