package com.wulala.myyolov5rtspthreadpool.ui.adapters;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.wulala.myyolov5rtspthreadpool.R;
import com.wulala.myyolov5rtspthreadpool.ui.DetectionSettingsFragment;

import java.util.List;
import java.util.Set;

/**
 * 检测类别列表适配器
 * 显示可选择的YOLO检测类别
 */
public class DetectionClassAdapter extends RecyclerView.Adapter<DetectionClassAdapter.ViewHolder> {
    
    private List<DetectionSettingsFragment.DetectionClassItem> classItems;
    private OnClassToggleListener listener;
    
    public interface OnClassToggleListener {
        void onClassToggled(String className, boolean enabled);
    }
    
    public DetectionClassAdapter(List<DetectionSettingsFragment.DetectionClassItem> classItems,
                                OnClassToggleListener listener) {
        this.classItems = classItems;
        this.listener = listener;
    }
    
    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        android.util.Log.d("DetectionClassAdapter", "创建ViewHolder");
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_detection_class, parent, false);
        ViewHolder holder = new ViewHolder(view);
        android.util.Log.d("DetectionClassAdapter", "ViewHolder创建完成, Switch控件: " +
                          (holder.switchEnabled != null ? "找到" : "未找到"));
        return holder;
    }
    
    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        DetectionSettingsFragment.DetectionClassItem item = classItems.get(position);

        android.util.Log.d("DetectionClassAdapter", "绑定视图: position=" + position +
                          ", className=" + item.className + ", enabled=" + item.enabled);

        // 设置显示名称
        holder.textClassName.setText(item.displayName);
        holder.textClassNameEn.setText(item.className);

        // 设置开关状态（临时移除监听器避免递归调用）
        holder.switchEnabled.setOnCheckedChangeListener(null);
        holder.switchEnabled.setChecked(item.enabled);

        android.util.Log.d("DetectionClassAdapter", "Switch状态设置完成: " + item.className + " -> " + item.enabled);
        
        // 设置优先级样式
        if (item.isPriority) {
            holder.textClassName.setTextColor(holder.itemView.getContext()
                    .getResources().getColor(android.R.color.black));
            holder.textClassNameEn.setTextColor(holder.itemView.getContext()
                    .getResources().getColor(R.color.colorPrimary));
        } else {
            holder.textClassName.setTextColor(holder.itemView.getContext()
                    .getResources().getColor(android.R.color.darker_gray));
            holder.textClassNameEn.setTextColor(holder.itemView.getContext()
                    .getResources().getColor(android.R.color.darker_gray));
        }
        
        // 重新设置监听器
        android.util.Log.d("DetectionClassAdapter", "设置监听器: " + item.className);
        holder.switchEnabled.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                android.util.Log.d("DetectionClassAdapter", "类别切换触发: " + item.className + " -> " + isChecked);
                item.enabled = isChecked;
                if (listener != null) {
                    android.util.Log.d("DetectionClassAdapter", "调用监听器回调: " + item.className);
                    listener.onClassToggled(item.className, isChecked);
                } else {
                    android.util.Log.w("DetectionClassAdapter", "监听器为null!");
                }

                // 显示简短的保存提示
                String message = isChecked ? "已启用 " + item.displayName : "已禁用 " + item.displayName;
                Toast.makeText(holder.itemView.getContext(), message, Toast.LENGTH_SHORT).show();
            }
        });
        
        // 设置点击事件
        android.util.Log.d("DetectionClassAdapter", "设置点击事件: " + item.className);
        holder.itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                android.util.Log.d("DetectionClassAdapter", "项目点击: " + item.className);
                holder.switchEnabled.toggle();
            }
        });

        // 添加触摸事件调试
        holder.itemView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, android.view.MotionEvent event) {
                if (event.getAction() == android.view.MotionEvent.ACTION_DOWN) {
                    android.util.Log.d("DetectionClassAdapter", "触摸事件: " + item.className + " at (" + event.getX() + ", " + event.getY() + ")");
                }
                return false; // 不消费事件，让点击事件继续传递
            }
        });

        // 直接在Switch上添加触摸监听器
        holder.switchEnabled.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, android.view.MotionEvent event) {
                if (event.getAction() == android.view.MotionEvent.ACTION_DOWN) {
                    android.util.Log.d("DetectionClassAdapter", "Switch触摸事件: " + item.className);
                }
                return false;
            }
        });
    }
    
    @Override
    public int getItemCount() {
        return classItems.size();
    }
    
    /**
     * 更新启用的类别
     */
    public void updateEnabledClasses(Set<String> enabledClasses) {
        for (DetectionSettingsFragment.DetectionClassItem item : classItems) {
            item.enabled = enabledClasses.contains(item.className);
        }
        notifyDataSetChanged();
    }
    
    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final TextView textClassName;
        public final TextView textClassNameEn;
        public final Switch switchEnabled;
        
        public ViewHolder(View view) {
            super(view);
            textClassName = view.findViewById(R.id.text_class_name);
            textClassNameEn = view.findViewById(R.id.text_class_name_en);
            switchEnabled = view.findViewById(R.id.switch_class_enabled);
        }
    }
}
