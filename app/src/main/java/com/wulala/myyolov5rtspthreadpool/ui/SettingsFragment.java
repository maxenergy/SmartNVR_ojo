package com.wulala.myyolov5rtspthreadpool.ui;

import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import java.util.List;

import com.wulala.myyolov5rtspthreadpool.R;
import com.wulala.myyolov5rtspthreadpool.Settings;
import com.wulala.myyolov5rtspthreadpool.SharedPreferencesManager;
import com.wulala.myyolov5rtspthreadpool.databinding.FragmentSettingsItemListBinding;
import com.wulala.myyolov5rtspthreadpool.entities.Camera;
import com.wulala.myyolov5rtspthreadpool.ui.adapters.SettingsRecyclerViewAdapter;
import com.wulala.myyolov5rtspthreadpool.utils.ItemMoveCallback;

/**
 * A fragment representing a list of YOLOv5 RTSP stream settings
 * Adapted from ojo project for YOLOv5 configuration
 */
public class SettingsFragment extends Fragment {

    private FragmentSettingsItemListBinding binding;
    private Settings settings;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        binding = FragmentSettingsItemListBinding.inflate(inflater, container, false);

        return binding.getRoot();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        // Setup toolbar
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            binding.settingsToolbar.getOverflowIcon().setTint(Color.WHITE);
        }
        binding.settingsToolbar.inflateMenu(R.menu.settings_menu);
        MenuItem rotMenuItem = binding.settingsToolbar.getMenu().findItem(R.id.menuitem_allow_rotation);
        rotMenuItem.setTitle(((SettingsActivity)getActivity()).getRotationEnabledSetting() ? R.string.menuitem_deny_rotation : R.string.menuitem_allow_rotation);

        // Register for item click
        binding.settingsToolbar.setOnMenuItemClickListener(new Toolbar.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                int itemId = item.getItemId();
                if (itemId == R.id.menuitem_add_camera) {
                    ((SettingsActivity)getActivity()).navigateToFragment(R.id.action_settingsToCameraUrl);
                    return true;
                } else if (itemId == R.id.menuitem_detection_settings) {
                    ((SettingsActivity)getActivity()).navigateToFragment(R.id.action_settingsToDetectionSettings);
                    return true;
                } else if (itemId == R.id.menuitem_allow_rotation) {
                    ((SettingsActivity)getActivity()).toggleRotationEnabledSetting();
                    SharedPreferencesManager.saveRotationEnabled(getContext(), ((SettingsActivity)getActivity()).getRotationEnabledSetting());
                    item.setTitle(((SettingsActivity)getActivity()).getRotationEnabledSetting() ? R.string.menuitem_deny_rotation : R.string.menuitem_allow_rotation);
                    return true;
                } else if (itemId == R.id.menuitem_info) {
                    ((SettingsActivity)getActivity()).navigateToFragment(R.id.action_SettingsToInfoFragment);
                    return true;
                }
                return false;
            }
        });
    }

    @Override
    public void onResume() {
        super.onResume();

        // Load cameras
        settings = Settings.fromDisk(getContext());
        List<Camera> cams = settings.getCameras();

        // Set the adapter
        RecyclerView recyclerView = binding.list;
        recyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        SettingsRecyclerViewAdapter adapter = new SettingsRecyclerViewAdapter(cams);
        ItemTouchHelper.Callback callback =
                new ItemMoveCallback(adapter);
        ItemTouchHelper touchHelper = new ItemTouchHelper(callback);
        touchHelper.attachToRecyclerView(recyclerView);
        adapter.setOnDragListener(touchHelper::startDrag);
        recyclerView.setAdapter(adapter);
        // Onclick listener
        adapter.setOnClickListener(new SettingsRecyclerViewAdapter.OnClickListener() {
            @Override
            public void onItemClick(int pos) {
                Bundle b = new Bundle();
                b.putInt(StreamUrlFragment.ARG_CAMERA, pos);
                ((SettingsActivity)getActivity()).navigateToFragment(R.id.action_settingsToCameraUrl, b);
            }
        });
    }

    @Override
    public void onPause() {
        super.onPause();

        // Save cameras
        if (binding.list.getAdapter() != null) {
            List<Camera> cams = ((SettingsRecyclerViewAdapter)binding.list.getAdapter()).getItems();
            this.settings.setCameras(cams);
            this.settings.save();
        }
    }
}