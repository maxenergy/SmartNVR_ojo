package com.wulala.myyolov5rtspthreadpool.ui;

import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import androidx.appcompat.app.AppCompatActivity;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;

import com.wulala.myyolov5rtspthreadpool.R;
import com.wulala.myyolov5rtspthreadpool.SharedPreferencesManager;
import com.wulala.myyolov5rtspthreadpool.databinding.ActivitySettingsBinding;

public class SettingsActivity extends AppCompatActivity {
    private static final String TAG = "SettingsActivity";

    private ActivitySettingsBinding binding;
    private NavController navController;
    private OnBackButtonPressedListener onBackButtonPressedListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(TAG, "SettingsActivity onCreate started");

        try {
            binding = ActivitySettingsBinding.inflate(getLayoutInflater());
            setContentView(binding.getRoot());
            Log.d(TAG, "Layout inflated successfully");

            // 延迟初始化NavController，确保Fragment已经创建
            binding.getRoot().post(() -> {
                try {
                    navController = Navigation.findNavController(this, R.id.nav_host_fragment_content_settings);
                    Log.d(TAG, "NavController initialized successfully");
                } catch (Exception e) {
                    Log.e(TAG, "Failed to initialize NavController", e);
                    // 如果NavController初始化失败，仍然可以继续使用Activity
                }
            });

            Log.d(TAG, "SettingsActivity onCreate completed successfully");
        } catch (Exception e) {
            Log.e(TAG, "Error in SettingsActivity onCreate", e);
            finish(); // 如果有严重错误，关闭Activity
        }
    }

    public void setOnBackButtonPressedListener(OnBackButtonPressedListener onBackButtonPressedListener) {
        this.onBackButtonPressedListener = onBackButtonPressedListener;
    }

    @Override
    public void onBackPressed() {
        if (this.onBackButtonPressedListener != null && this.onBackButtonPressedListener.onBackPressed())
            return;
        super.onBackPressed();
    }

    public void navigateToFragment(int actionId) {
        navigateToFragment(actionId, null);
    }

    public void navigateToFragment(int actionId, Bundle bundle) {
        if (navController == null) {
            Log.e(TAG, "Not initialized");
            return;
        }

        try {
            if (bundle != null)
                navController.navigate(actionId, bundle);
            else
                navController.navigate(actionId);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Unable to navigate to fragment: " + e.getMessage());
        }
    }

    public void toggleRotationEnabledSetting() {
        SharedPreferencesManager.toggleRotationEnabled(this);
    }

    public boolean getRotationEnabledSetting() {
        return SharedPreferencesManager.loadRotationEnabled(this);
    }
}