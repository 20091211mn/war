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
