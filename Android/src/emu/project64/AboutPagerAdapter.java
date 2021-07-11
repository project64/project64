package emu.project64;

import android.database.DataSetObserver;
import android.support.v4.view.PagerAdapter;
import android.view.View;
import android.view.ViewGroup;

import java.util.List;

public class AboutPagerAdapter extends PagerAdapter 
{
    private List<View> mLists;
    private String[] mTitles;

    public AboutPagerAdapter(List<View> lists, String[] titles)
    {
        mLists = lists;
        mTitles = titles;
    }

    @Override
    public int getCount()
    {
        return mLists == null ? 0 : mLists.size();
    }

    @Override
    public boolean isViewFromObject(View view, Object object)
    {
        return view == object;
    }

    @Override
    public Object instantiateItem(ViewGroup container, int position)
    {
        View page = mLists.get(position);
        container.addView(page);
        return page;
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object)
    {
        container.removeView((View) object);
        mLists.remove(position);
    }

    @Override
    public CharSequence getPageTitle(int position)
    {
        if (position < 0 || position >= mTitles.length)
        {
            return "";
        }
        else
        {
            return mTitles[position];
        }
    }

    @Override
    public void unregisterDataSetObserver(DataSetObserver observer)
    {
        if (observer != null)
        {
            super.unregisterDataSetObserver(observer);
        }
    }
}
