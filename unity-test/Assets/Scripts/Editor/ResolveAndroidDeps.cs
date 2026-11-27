#if UNITY_EDITOR
using System;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEngine;

// يجبر أداة External Dependency Manager على حلّ مكتبات أندرويد الأصلية (LevelPlay) قبل كل بناء أندرويد،
// لأن الحلّ التلقائي لا يعمل داخل بناء Unity Cloud (وضع batch).
public class ResolveAndroidDeps : IPreprocessBuildWithReport
{
    public int callbackOrder => -1000;
    public void OnPreprocessBuild(BuildReport report)
    {
        if (report.summary.platform != BuildTarget.Android) return;
        try
        {
            var t = Type.GetType("GooglePlayServices.PlayServicesResolver, Google.JarResolver");
            if (t == null) { Debug.LogWarning("[ResolveAndroidDeps] PlayServicesResolver غير موجود"); return; }
            var m = t.GetMethod("ResolveSync", new[] { typeof(bool) });
            if (m == null) { Debug.LogWarning("[ResolveAndroidDeps] ResolveSync غير موجودة"); return; }
            Debug.Log("[ResolveAndroidDeps] بدء حلّ تبعيات أندرويد...");
            var ok = m.Invoke(null, new object[] { true });
            Debug.Log("[ResolveAndroidDeps] النتيجة: " + ok);
            AssetDatabase.Refresh();
        }
        catch (Exception e) { Debug.LogWarning("[ResolveAndroidDeps] " + e); }
    }
}
#endif
