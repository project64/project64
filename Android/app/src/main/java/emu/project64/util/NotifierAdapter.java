package emu.project64.util;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import emu.project64.R;

public class NotifierAdapter extends BaseAdapter
{
    Context con;
    String[] data;
    Runnable DisplayMessager;

    public NotifierAdapter (Context context, Runnable DisplayMessager, String[] data)
    {
        this.con = context;
        this.data = data;
        this.DisplayMessager = DisplayMessager;
    }
    @Override
    public int getCount() {
        return data.length;
    }

    @Override
    public Object getItem(int position) {
        return data[position];
    }

    @Override
    public long getItemId(int position) {
        return 0;
    }

    @Override
    public View getView(int position, View view, ViewGroup parent) {
        LayoutInflater inflater = (LayoutInflater) con.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        view = inflater.inflate(R.layout.notifier_adapter, parent, false);
        TextView text = (TextView) view.findViewById(R.id.text);
        text.setText(data[position]);

        Button button = (Button) view.findViewById(R.id.button);
        button.setOnClickListener(
                new View.OnClickListener()
                {
                    public void onClick(View view)
                    {
                        synchronized(DisplayMessager)
                        {
                            DisplayMessager.notify();
                        };
                    }
                }
        );

        return view;
    }

}