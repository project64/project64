/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
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

    class YourAdapter extends ArrayAdapter<String>
    {
        public YourAdapter(Context context, String[] values)
        {
            super(context, R.layout.two_lines_list_preference_row, values);
        }

        class ViewHolder
        {
            TextView title;
            TextView subTitle;
            RadioButton radioBtn;
        }

        ViewHolder holder;
        ViewHolderClickListener listener = new ViewHolderClickListener();

        @Override
        public View getView(int position, View convertView, ViewGroup parent)
        {
            final LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            if (convertView == null)
            {
                convertView = inflater.inflate(R.layout.two_lines_list_preference_row, null);
                holder = new ViewHolder();
                holder.title = (TextView) convertView.findViewById(R.id.two_lines_list_view_row_text);
                holder.subTitle = (TextView) convertView.findViewById(R.id.two_lines_list_view_row_subtext);
                holder.radioBtn = (RadioButton) convertView.findViewById(R.id.two_lines_list_view_row_radiobtn);
                convertView.setTag(holder);

                holder.title.setOnClickListener(listener);
                holder.subTitle.setOnClickListener(listener);
                holder.radioBtn.setOnClickListener(listener);
            }
            else
            {
                holder = (ViewHolder) convertView.getTag();
            }
            final TwoLinesListPreference preference = getTwoLinesListPreference();

            holder.title.setText(preference.getEntries()[position]);
            holder.title.setTag(position);
            holder.subTitle.setText(preference.getEntriesSubtitles()[position]);
            holder.subTitle.setTag(position);

            holder.radioBtn.setChecked(preference.getValueIndex() == position);
            holder.radioBtn.setTag(position);

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

        ListAdapter adapter = new YourAdapter(builder.getContext(), values);
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
            preference.setValue(preference.getEntries()[EntryIndex].toString());
            TwoLinesListPreferenceDialogFragmentCompat.this.getDialog().dismiss();
        }
    }
}
