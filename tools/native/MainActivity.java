package com.warzone.lan;

import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.webkit.WebView;

import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import com.getcapacitor.BridgeActivity;

public class MainActivity extends BridgeActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        registerPlugin(LanHostPlugin.class);
        super.onCreate(savedInstanceState);
        Window w = getWindow();
        w.setBackgroundDrawable(new ColorDrawable(Color.BLACK));
        w.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (Build.VERSION.SDK_INT >= 28) {
            WindowManager.LayoutParams lp = w.getAttributes();
            lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            w.setAttributes(lp);
        }
        try {
            WebView wv = getBridge().getWebView();
            wv.setBackgroundColor(Color.BLACK);
            wv.setOverScrollMode(View.OVER_SCROLL_NEVER);
            wv.setLongClickable(false);
            wv.setHapticFeedbackEnabled(false);
        } catch (Exception ignored) { }
        hideSystemUi();
        maxRefreshRate();
    }

    // يطلب أعلى معدل تحديث تدعمه الشاشة (90/120Hz) لإطارات أسلس
    private void maxRefreshRate() {
        try {
            if (Build.VERSION.SDK_INT >= 23) {
                android.view.Display d = getWindowManager().getDefaultDisplay();
                android.view.Display.Mode best = d.getMode();
                for (android.view.Display.Mode m : d.getSupportedModes()) {
                    if (m.getPhysicalWidth() == best.getPhysicalWidth() && m.getPhysicalHeight() == best.getPhysicalHeight() && m.getRefreshRate() > best.getRefreshRate()) best = m;
                }
                WindowManager.LayoutParams lp = getWindow().getAttributes();
                lp.preferredDisplayModeId = best.getModeId();
                if (Build.VERSION.SDK_INT >= 30) lp.preferredRefreshRate = best.getRefreshRate();
                getWindow().setAttributes(lp);
            }
        } catch (Exception ignored) { }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemUi();
    }

    @Override
    public void onResume() {
        super.onResume();
        hideSystemUi();
    }

    // وضع غامر كامل: إخفاء شريط الحالة وأزرار التنقل (تظهر مؤقتًا بالسحب من الحافة)
    private void hideSystemUi() {
        Window w = getWindow();
        WindowCompat.setDecorFitsSystemWindows(w, false);
        WindowInsetsControllerCompat c = WindowCompat.getInsetsController(w, w.getDecorView());
        if (c != null) {
            c.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            c.hide(WindowInsetsCompat.Type.systemBars());
        }
        w.setStatusBarColor(Color.TRANSPARENT);
        w.setNavigationBarColor(Color.TRANSPARENT);
    }
}
