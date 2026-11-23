#if UNITY_EDITOR
using UnityEditor;

// بناء APK من سطر الأوامر / CI: -executeMethod CIBuild.Android
public static class CIBuild
{
    public static void Android()
    {
        Setup.CreateScene();
        PlayerSettings.Android.targetArchitectures = AndroidArchitecture.ARM64;
        PlayerSettings.SetScriptingBackend(BuildTargetGroup.Android, ScriptingImplementation.IL2CPP);
        var r = BuildPipeline.BuildPlayer(new BuildPlayerOptions { scenes = new[] { "Assets/Scenes/Main.unity" }, locationPathName = "build/Warzone-Unity.apk", target = BuildTarget.Android, options = BuildOptions.None });
        if (r.summary.result != UnityEditor.Build.Reporting.BuildResult.Succeeded) EditorApplication.Exit(1);
    }
}
#endif
