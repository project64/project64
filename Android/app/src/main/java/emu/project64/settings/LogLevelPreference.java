package emu.project64.settings;

import emu.project64.R;
import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.text.TextUtils;
import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;
import android.content.DialogInterface;

public class LogLevelPreference extends Preference {
    private CharSequence[] mEntries;
    private CharSequence[] mEntryValues;
    private CharSequence[] mEntriesSubtitles;
    private String mValue;
    private int mValueIndex;
    private boolean mValueSet;

    public LogLevelPreference(Context context, AttributeSet attrs)
    {
        super(context, attrs, getAttr(context, R.attr.LogLevelPreferenceStyle, android.R.attr.preferenceStyle));

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TwoLinesListPreference);
        mEntriesSubtitles = a.getTextArray(R.styleable.TwoLinesListPreference_entriesSubtitles);
        mEntries = context.getResources().getStringArray(R.array.trace_severity_list);
        mEntryValues = context.getResources().getStringArray(R.array.trace_severity_values);
        a.recycle();

        updateSummary();
    }

    @Override
    protected void onClick() {
        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setTitle(getTitle())
                .setSingleChoiceItems(mEntries, mValueIndex, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int index) {
                        setValueIndex(index);
                        dialog.dismiss();
                    }
                })
                .setPositiveButton("Go", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int index) {
                        setValueIndex(index);
                        dialog.dismiss();
                    }
                });
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private void updateSummary()
    {
        if (mValueIndex < 0)
        {
            return;
        }
        setSummary(mEntries[mValueIndex].toString());
    }

    private void setValueIndex(int index)
    {
        String value = mEntryValues[index].toString();
        final boolean changed = !TextUtils.equals(mValue, value);
        if (changed || !mValueSet) {
            mValueIndex = index;
            mValue = value;
            mValueSet = true;
            persistString(value);
            if (changed) {
                notifyChanged();
            }
            updateSummary();
        }
    }

    private static int getAttr(Context context, int attr, int fallbackAttr)
    {
        TypedValue value = new TypedValue();
        context.getTheme().resolveAttribute(attr, value, true);
        if (value.resourceId != 0)
        {
            return attr;
        }
        return fallbackAttr;
    }

    @Override
    protected void onSetInitialValue(Object defaultValue)
    {
        if (getSharedPreferences().contains(getKey()))
        {
            String Value = getPersistedString( Integer.toString(mValueIndex));
            for (int i = 0, n = mEntryValues.length; i < n; i++)
            {
                if (mEntryValues[i].equals(Value))
                {
                    mValueIndex = i;
                    updateSummary();
                }
            }
        }
    }
}