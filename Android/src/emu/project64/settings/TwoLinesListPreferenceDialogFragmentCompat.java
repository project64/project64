package emu.project64.settings;

import emu.project64.R;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v7.app.AlertDialog;
import android.support.v7.preference.PreferenceDialogFragmentCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListAdapter;
import android.widget.RadioButton;
import android.widget.TextView;

public class TwoLinesListPreferenceDialogFragmentCompat extends PreferenceDialogFragmentCompat
{
    private TwoLinesListPreference getTwoLinesListPreference()
    {
        return (TwoLinesListPreference)this.getPreference();
    }

    class TwoLinesListPreferenceAdapter extends ArrayAdapter<String>
    {
        public TwoLinesListPreferenceAdapter(Context context, String[] values)
        {
            super(context, R.layout.two_lines_list_preference_row, values);
        }

        class ViewHolder
        {
        }

        ViewHolder holder;
        ViewHolderClickListener listener = new ViewHolderClickListener();

        @Override
        public View getView(int position, View convertView, ViewGroup parent)
        {
            final TwoLinesListPreference preference = getTwoLinesListPreference();

            final LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflater.inflate(R.layout.two_lines_list_preference_row, null);

            TextView title = (TextView) convertView.findViewById(R.id.two_lines_list_view_row_text);
            title.setOnClickListener(listener);
            title.setText(preference.getEntries()[position]);
            title.setTag(position);

            TextView subTitle = (TextView) convertView.findViewById(R.id.two_lines_list_view_row_subtext);
            if (preference.getEntriesSubtitles()[position].length() == 0)
            {
                subTitle.setVisibility(View.GONE);
            }
            else
            {
                subTitle.setOnClickListener(listener);
                subTitle.setText(preference.getEntriesSubtitles()[position]);
                subTitle.setTag(position);
            }

            RadioButton radioBtn = (RadioButton) convertView.findViewById(R.id.two_lines_list_view_row_radiobtn);
            radioBtn.setOnClickListener(listener);
            radioBtn.setChecked(preference.getValueIndex() == position);
            radioBtn.setTag(position);

            return convertView;
        }
    }

    @Override
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder)
    {
        super.onPrepareDialogBuilder(builder);
        final TwoLinesListPreference preference = getTwoLinesListPreference();
        CharSequence[] entries = preference.getEntries();
        String[] values = new String[ entries.length ];
        for (int i = 0; i < entries.length; i ++)
        {
            values[i] = entries[i].toString();
        }

        ListAdapter adapter = new TwoLinesListPreferenceAdapter(builder.getContext(), values);
        builder.setAdapter(adapter, new DialogInterface.OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
            }
        });
    }

    public static TwoLinesListPreferenceDialogFragmentCompat newInstance(String key)
    {
        TwoLinesListPreferenceDialogFragmentCompat fragment = new TwoLinesListPreferenceDialogFragmentCompat();
        Bundle b = new Bundle(1);
        b.putString("key", key);
        fragment.setArguments(b);
        return fragment;
    }

    public void onDialogClosed(boolean positiveResult)
    {
    }

    private class ViewHolderClickListener implements OnClickListener
    {
        @Override
        public void onClick(View v)
        {
            final TwoLinesListPreference preference = getTwoLinesListPreference();
            int EntryIndex = (Integer) v.getTag();
            preference.setValue(preference.getEntryValues()[EntryIndex].toString());
            TwoLinesListPreferenceDialogFragmentCompat.this.getDialog().dismiss();
        }
    }
}
