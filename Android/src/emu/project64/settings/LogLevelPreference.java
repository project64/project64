package emu.project64.settings;

import emu.project64.R;
import android.content.Context;
import android.content.res.TypedArray;
import android.support.v7.preference.ListPreference;
import android.util.AttributeSet;

public class LogLevelPreference extends ListPreference
{
    private CharSequence[] mEntriesSubtitles;
    private int mValueIndex;

    public LogLevelPreference(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TwoLinesListPreference);
        mEntriesSubtitles = a.getTextArray(R.styleable.TwoLinesListPreference_entriesSubtitles);
        
        String[] TraceSeverityList=context.getResources().getStringArray(R.array.trace_severity_list);
        setEntries(TraceSeverityList);
        
        String[] TraceSeverityValues=context.getResources().getStringArray(R.array.trace_severity_values);        
        setEntryValues(TraceSeverityValues);
        a.recycle();
    }
        
    @Override
    protected void onSetInitialValue(boolean restoreValue, Object defaultValue)
    {
        super.onSetInitialValue(restoreValue, defaultValue);

        mEntriesSubtitles = getEntriesSubtitles();
        mValueIndex = getValueIndex();
    }

    @Override
    public void setValue(String value) 
    {
        super.setValue(value);
        mValueIndex = getValueIndex();
        updateSummary();
    }
    /**
    * Returns the index of the given value (in the entry values array).
    *
    * @param value The value whose index should be returned.
    * @return The index of the value, or -1 if not found.
    */
    public int findIndexOfValue(String value)
    {
        CharSequence[] EntryValues = getEntryValues();
        if (value != null && EntryValues != null)
        {
            for (int i = EntryValues.length - 1; i >= 0; i--)
            {
                if (EntryValues[i].equals(value))
                {
                    return i;
                }
            }
        }
        return -1;
    }

    public int getValueIndex()
    {
        return findIndexOfValue(getValue());
    }

    public CharSequence[] getEntriesSubtitles()
    {
        return mEntriesSubtitles;
    }

    @Override
    public void setEntries(CharSequence[] Entries)
    {
        super.setEntries(Entries);
        updateSummary();
    }

    @Override
    public void setEntryValues(CharSequence[] EntryValues)
    {
        super.setEntryValues(EntryValues);
        mValueIndex = getValueIndex();
        updateSummary();
    }

    public void setEntriesSubtitles(CharSequence[] mEntriesSubtitles)
    {
        this.mEntriesSubtitles = mEntriesSubtitles;
        updateSummary();
    }

    private void updateSummary()
    {
        if (mValueIndex < 0)
        {
            return;
        }
        CharSequence[] Entries = getEntries();
        String summary = Entries[mValueIndex].toString();
        if (mEntriesSubtitles != null && mEntriesSubtitles.length > mValueIndex)
        {
            String subtitle = mEntriesSubtitles[mValueIndex].toString();
            if (summary.length() > 0 && subtitle.length() > 0)
            {
                summary += " - " + subtitle;
            }            
        }
        setSummary( summary );
    }
}