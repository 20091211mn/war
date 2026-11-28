using System;
using UnityEngine;
#if LEVELPLAY_ADS
using Unity.Services.LevelPlay;
#endif

// إعلانات Unity LevelPlay (الحزمة com.unity.services.levelplay + الرمز LEVELPLAY_ADS من Assets/csc.rsp).
// تعمل على الجوال فقط؛ في المحرر وعلى الحاسوب كل الدوال بلا أثر.
public static class Ads
{
    // ضع القيم من لوحة LevelPlay (Unity Cloud). فارغ = الإعلانات متوقفة.
    public const string AppKey = "29f92b3b5";
    public const string InterstitialId = "oc9mqlyb2zdf2768";
    public const string RewardedId = "8e5p6rtwhs6lcsv5";

    public static string Status = "ADS: off";
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
        if (!Enabled) { Status = "ADS: disabled (not mobile / no key)"; return; }
        Status = "ADS: init...";
        try
        {
            LevelPlay.OnInitSuccess += _ =>
            {
                ready = true; Status = "ADS: init OK";
                if (InterstitialId != "") { inter = new LevelPlayInterstitialAd(InterstitialId); inter.OnAdClosed += _2 => inter.LoadAd(); inter.OnAdLoaded += _2 => Status = "ADS: interstitial loaded"; inter.OnAdLoadFailed += err => Status = "ADS: inter load failed: " + err; inter.LoadAd(); }
                if (RewardedId != "")
                {
                    rewarded = new LevelPlayRewardedAd(RewardedId);
                    rewarded.OnAdRewarded += (_2, _3) => { var a = onReward; onReward = null; if (a != null) a(); };
                    rewarded.OnAdClosed += _2 => rewarded.LoadAd(); rewarded.OnAdLoaded += _2 => Status = "ADS: rewarded loaded"; rewarded.OnAdLoadFailed += err => Status = "ADS: rewarded load failed: " + err;
                    rewarded.LoadAd();
                }
            };
            LevelPlay.OnInitFailed += e => { Status = "ADS: init FAILED: " + e; Debug.Log("LevelPlay init failed: " + e); };
            LevelPlay.Init(AppKey);
        }
        catch (Exception e) { Status = "ADS: init error: " + e.Message; Debug.Log("Ads init error: " + e.Message); }
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

    // إعلان بيني عند نهاية كل جولة (عندما يصل فريق لحدّ القتلى)
    public static void RoundEnded()
    {
#if LEVELPLAY_ADS
        if (Enabled && ready && inter != null && inter.IsAdReady()) inter.ShowAd();
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
