package com.wulala.myyolov5rtspthreadpool;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import com.wulala.myyolov5rtspthreadpool.entities.Camera;

/**
 * Manages the settings persistence for YOLOv5 RTSP project
 * Adapted from ojo project to support YOLOv5 configuration
 */
public class Settings implements Serializable {
    private static final long serialVersionUID = 1081285022445419696L;
    private static final String FILENAME = "yolov5_settings.bin";
    private static final String TAG = "Settings";

    private volatile String settingsFilePath;
    private List<Camera> cameras = new ArrayList<>();
    private String currentRtspUrl = "";
    private float modelConfidence = 0.5f;

    public static Settings fromDisk(Context context) {
        String filePath = context.getFilesDir() + File.separator + FILENAME;
        Settings s = new Settings();
        try {
            FileInputStream fin = new FileInputStream(filePath);
            ObjectInputStream ois = new ObjectInputStream(fin);
            s = (Settings) ois.readObject();
        } catch (FileNotFoundException e) {
            Log.d(TAG, "No saved settings found, will create a new one");
            // 添加默认摄像头配置
            s.cameras.add(new Camera("主摄像头", "rtsp://192.168.31.22:8554/unicast"));
            s.cameras.add(new Camera("副摄像头", "rtsp://192.168.31.64:8554/unicast"));
        } catch (IOException e) {
            Log.e(TAG, "Unable to load settings from disk: " + e.toString());
        } catch (ClassNotFoundException e) {
            Log.e(TAG, e.toString());
        }
        s.settingsFilePath = filePath;
        return s;
    }

    public List<Camera> getCameras() {
        return cameras;
    }

    public void setCameras(List<Camera> cameras) {
        this.cameras = cameras;
    }

    public void addCamera(Camera camera) {
        this.cameras.add(camera);
    }

    public String getCurrentRtspUrl() {
        if (!cameras.isEmpty() && currentRtspUrl.isEmpty()) {
            return cameras.get(0).getRtspUrl();
        }
        return currentRtspUrl;
    }

    public void setCurrentRtspUrl(String currentRtspUrl) {
        this.currentRtspUrl = currentRtspUrl;
    }

    public float getModelConfidence() {
        return modelConfidence;
    }

    public void setModelConfidence(float modelConfidence) {
        this.modelConfidence = modelConfidence;
    }

    public boolean save() {
        try {
            FileOutputStream fout = new FileOutputStream(settingsFilePath);
            ObjectOutputStream oos = new ObjectOutputStream(fout);
            oos.writeObject(this);
            fout.close();
            oos.close();
            return true;
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Unable to create file " + settingsFilePath + ": " + e.toString());
        } catch (IOException e) {
            Log.e(TAG, "Unable to save settings: " + e.toString());
        }
        return false;
    }
}