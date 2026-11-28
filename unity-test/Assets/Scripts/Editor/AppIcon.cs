#if UNITY_EDITOR
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEngine;

// يضبط أيقونة اللعبة من Assets/Icon.png قبل كل بناء (يعمل داخل Unity Cloud)
public class AppIcon : IPreprocessBuildWithReport
{
    public int callbackOrder => -900;
    public void OnPreprocessBuild(BuildReport report) { Apply(); }
    [InitializeOnLoadMethod] static void OnLoad() { EditorApplication.delayCall += Apply; }
    static void Apply()
    {
        try
        {
            AssetDatabase.ImportAsset("Assets/Icon.png");
            var t = AssetDatabase.LoadAssetAtPath<Texture2D>("Assets/Icon.png"); if (t == null) return;
            var tgt = NamedBuildTarget.Unknown;
            PlayerSettings.SetIcons(tgt, new[] { t }, IconKind.Any);
            var n = Mathf.Max(1, PlayerSettings.GetIcons(NamedBuildTarget.Android, IconKind.Any).Length);
            var arr = new Texture2D[n]; for (int i = 0; i < n; i++) arr[i] = t;
            PlayerSettings.SetIcons(NamedBuildTarget.Android, arr, IconKind.Any);
        }
        catch (System.Exception e) { Debug.LogWarning("[AppIcon] " + e.Message); }
    }
}
#endif
