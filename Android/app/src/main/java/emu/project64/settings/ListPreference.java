package emu.project64.settings;

import emu.project64.R;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.text.TextUtils;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.view.ContextThemeWrapper;
import androidx.core.content.res.TypedArrayUtils;
import androidx.preference.Preference;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class ListPreference extends Preference {
    private CharSequence[] mEntries;
    private CharSequence[] mEntryValues;
    private CharSequence[] mEntriesSubtitles;
    private String mValue;
    private String mTitle;
    private int mValueIndex;
    private boolean mValueSet;

    public ListPreference(Context context, AttributeSet attrs)
    {
        super(context, attrs, getAttr(context, R.attr.ListPreferenceStyle, android.R.attr.preferenceStyle));

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.ListPreference);
        mEntries = a.getTextArray(R.styleable.ListPreference_entries);
        mEntryValues = a.getTextArray(R.styleable.ListPreference_entryValues);
        mTitle = String.valueOf(getTitle());
        mValueIndex = 0;
        a.recycle();
        updateSummary();
    }

    @Override
    protected void onClick()
    {
        String[] mEntriesString = new String[mEntries.length];
        int i=0;
        for(CharSequence ch: mEntries)
        {
            mEntriesString[i++] = ch.toString();
        }

        final LayoutInflater inflater = (LayoutInflater) getContext().getSystemService( Context.LAYOUT_INFLATER_SERVICE );
        View layout = inflater.inflate( R.layout.dialog_menu, null );
        AlertDialog.Builder builder = new AlertDialog.Builder(new ContextThemeWrapper(getContext(),R.style.Theme_Project64_Dialog_Alert));
        TextView DlgTitle = (TextView)layout.findViewById(R.id.dlg_title);
        DlgTitle.setText(mTitle);
        ListView ListOptions = (ListView)layout.findViewById(R.id.list_options);
        ListAdapter adapter = new ArrayAdapter<String>(getContext(), R.layout.simple_list_item_single_choice,mEntriesString);
        ListOptions.setAdapter(adapter);
        ListOptions.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        ListOptions.setItemChecked(mValueIndex,true);
        builder.setView( layout);
        builder.setPositiveButton( null, null );
        builder.setNegativeButton( null, null );
        AlertDialog dialog = builder.create();
        ListOptions.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
            @Override
            public void onItemClick(AdapterView parent, View view, int position, long id)
            {
                setValueIndex(position);
                dialog.dismiss();
            }
        });
        dialog.show();
    }

    private void updateSummary()
    {
        if (mValueIndex < 0 && mValueIndex < mEntries.length)
        {
            return;
        }
        setSummary(mEntries[mValueIndex].toString());
    }

    private void setValueIndex(int index)
    {
        String value = mEntryValues[index].toString();
        final boolean changed = !TextUtils.equals(mValue, value);
        if (changed || !mValueSet)
        {
            mValueIndex = index;
            mValue = value;
            mValueSet = true;
            persistString(value);
            if (changed)
            {
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