using System;
using UnityEngine;
#if LEVELPLAY_ADS
using Unity.Services.LevelPlay;
#endif

// إعلانات Unity LevelPlay. لا تعمل إلا بعد: 1) إضافة الحزمة وتفعيل الرمز LEVELPLAY_ADS  2) وضع المفاتيح هنا.
// بدون ذلك كل الدوال لا تفعل شيئاً (اللعبة تعمل كالمعتاد).
public static class Ads
{
    // ضع القيم من لوحة LevelPlay (Unity Cloud). فارغ = الإعلانات متوقفة.
    public const string AppKey = "";
    public const string InterstitialId = "";
    public const string RewardedId = "";

    static int deaths; static bool ready;
    public static bool Enabled
    {
        get
        {
#if LEVELPLAY_ADS
            return AppKey != "" && Application.isMobilePlatform;
#else
            return false;
#endif
        }
    }
    public static bool RewardedReady
    {
        get
        {
#if LEVELPLAY_ADS
            return ready && rewarded != null && rewarded.IsAdReady();
#else
            return false;
#endif
        }
    }

#if LEVELPLAY_ADS
    static LevelPlayInterstitialAd inter; static LevelPlayRewardedAd rewarded; static Action onReward;
#endif

    public static void Init()
    {
#if LEVELPLAY_ADS
        if (!Enabled) return;
        try
        {
            LevelPlay.OnInitSuccess += _ =>
            {
                ready = true;
                if (InterstitialId != "") { inter = new LevelPlayInterstitialAd(InterstitialId); inter.OnAdClosed += _2 => inter.LoadAd(); inter.LoadAd(); }
                if (RewardedId != "")
                {
                    rewarded = new LevelPlayRewardedAd(RewardedId);
                    rewarded.OnAdRewarded += (_2, _3) => { var a = onReward; onReward = null; if (a != null) a(); };
                    rewarded.OnAdClosed += _2 => rewarded.LoadAd();
                    rewarded.LoadAd();
                }
            };
            LevelPlay.OnInitFailed += e => Debug.Log("LevelPlay init failed: " + e);
            LevelPlay.Init(AppKey);
        }
        catch (Exception e) { Debug.Log("Ads init error: " + e.Message); }
#endif
    }

    // إعلان بيني بعد كل 3 وفيات للاعب (لا يزعج كثيراً)
    public static void PlayerDied()
    {
        deaths++;
#if LEVELPLAY_ADS
        if (Enabled && ready && inter != null && deaths % 3 == 0 && inter.IsAdReady()) inter.ShowAd();
#endif
    }

    // إعلان بمكافأة: يُنفَّذ onDone فقط إذا أكمل اللاعب المشاهدة
    public static void ShowRewarded(Action onDone)
    {
#if LEVELPLAY_ADS
        if (RewardedReady) { onReward = onDone; rewarded.ShowAd(); }
#endif
    }
}
