package emu.project64;

import emu.project64.R;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import androidx.appcompat.view.menu.MenuBuilder;

/* ExpandableListView which stores its data set as a Menu hierarchy */

public class MenuListView extends ExpandableListView
{
    private MenuListAdapter mAdapter;
    private OnClickListener mListener;
    private Menu mListData;
    
    public MenuListView( Context context, AttributeSet attrs )
    {
        super( context, attrs );
        mAdapter = null;
        mListener = null;
        mListData = null;
    }
    
    public void setMenuResource( int menuResource )
    {
        Context context = getContext();
        @SuppressLint("RestrictedApi") Menu menu = new MenuBuilder( context );
        Activity activity = (Activity) context;
        activity.getMenuInflater().inflate( menuResource, menu );
        setMenu( menu );
    }
    
    public void setMenu( Menu menu )
    {
        mListData = menu;
        mAdapter = new MenuListAdapter( this, menu );
        setAdapter( mAdapter );
        setChoiceMode( ListView.CHOICE_MODE_SINGLE );
        
        // MenuListView uses its own group indicators
        setGroupIndicator( null );
        
        // Update the expand/collapse group indicators as needed
        setOnGroupExpandListener( new OnGroupExpandListener()
        {
            @Override
            public void onGroupExpand( int groupPosition )
            {
                reload();
            }
        } );
        
        setOnGroupCollapseListener( new OnGroupCollapseListener()
        {
            @Override
            public void onGroupCollapse( int groupPosition )
            {
                reload();
            }
        });
        
        setOnGroupClickListener( new OnGroupClickListener()
        {
            @Override
            public boolean onGroupClick( ExpandableListView parent, View view, int groupPosition, long itemId )
            {
                MenuItem menuItem = mListData.getItem( groupPosition );
                SubMenu submenu = menuItem.getSubMenu();
                if( submenu == null )
                {
                    if( mListener != null )
                    {
                        mListener.onClick( menuItem );
                    }
                }
                return false;
            }
        });
        
        setOnChildClickListener( new OnChildClickListener()
        {
            @Override
            public boolean onChildClick( ExpandableListView parent, View view, int groupPosition, int childPosition, long itemId )
            {
                MenuItem menuItem = mListData.getItem( groupPosition ).getSubMenu() .getItem( childPosition );
                if (mListener != null)
                {
                    mListener.onClick( menuItem );
                }
                return false;
            }
        });
    }
    
    public Menu getMenu()
    {
        return mListData;
    }
    
    public void reload()
    {
        mAdapter.notifyDataSetChanged();
    }
    
    public void setOnClickListener( OnClickListener listener )
    {
        mListener = listener;
    }
    
    public OnClickListener getOnClickListener()
    {
        return mListener;
    }
    
    public static class MenuListAdapter extends BaseExpandableListAdapter
    {
        private MenuListView mListView;
        private Menu mListData;
        
        public MenuListAdapter( MenuListView listView, Menu listData )
        {
            mListView = listView;
            mListData = listData;
        }
        
        @Override
        public boolean isChildSelectable( int groupPosition, int childPosition )
        {
            return getChild( groupPosition, childPosition ).isEnabled();
        }
        
        @Override
        public MenuItem getChild( int groupPosition, int childPosition )
        {
            return getGroup( groupPosition ).getSubMenu().getItem( childPosition );
        }
        
        @Override
        public long getChildId( int groupPosition, int childPosition )
        {
            return getChild( groupPosition, childPosition ).getItemId();
        }
        
        @Override
        public int getChildrenCount( int groupPosition )
        {
            SubMenu submenu = mListData.getItem( groupPosition ).getSubMenu();
            return ( submenu != null ) ? submenu.size() : 0;
        }
        
        @Override
        public View getChildView( int groupPosition, final int childPosition, boolean isLastChild, View convertView, ViewGroup parent )
        {
            LayoutInflater inflater = (LayoutInflater) mListView.getContext().getSystemService( Context.LAYOUT_INFLATER_SERVICE );
            View view = convertView;
            if( view == null )
            {
                view = inflater.inflate( R.layout.list_item_menu, null );
            }

            MenuItem item = getChild( groupPosition, childPosition );
            if( item != null )
            {
                TextView text1 = (TextView) view.findViewById( R.id.text1 );
                TextView text2 = (TextView) view.findViewById( R.id.text2 );
                ImageView icon = (ImageView) view.findViewById( R.id.icon );
                ImageView indicator = (ImageView) view.findViewById( R.id.indicator );
                
                text1.setText( item.getTitle() );
                text2.setVisibility( View.GONE );
                icon.setImageDrawable( item.getIcon() );
                
                view.setBackgroundColor( 0x0 );
                
                // Indent child views by 15 points
                DisplayMetrics metrics = new DisplayMetrics();
                ( (Activity) mListView.getContext() ).getWindowManager().getDefaultDisplay().getMetrics( metrics );
                
                view.setPadding((int)(15 * metrics.density), view.getPaddingTop(), view.getPaddingRight(), view.getPaddingBottom() );
                
                if( !item.isCheckable() )
                    indicator.setImageResource( 0x0 );
                else if( item.isChecked() )
                    indicator.setImageResource( R.drawable.ic_check );
                else
                    indicator.setImageResource( R.drawable.ic_box );
            }
            return view;
        }
        
        @Override
        public MenuItem getGroup( int groupPosition )
        {
            return mListData.getItem( groupPosition );
        }
        
        @Override
        public long getGroupId( int groupPosition )
        {
            return getGroup( groupPosition ).getItemId();
        }
        
        @Override
        public int getGroupCount()
        {
            return mListData.size();
        }
        
        @Override
        public View getGroupView( int groupPosition, boolean isExpanded, View convertView, ViewGroup parent )
        {
            LayoutInflater inflater = (LayoutInflater) mListView.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE );
            View view = convertView;
            if( view == null )
            {
                view = inflater.inflate( R.layout.list_item_menu, null );
            }

            MenuItem item = getGroup( groupPosition );
            if( item != null )
            {
                TextView text1 = (TextView) view.findViewById( R.id.text1 );
                ImageView icon = (ImageView) view.findViewById( R.id.icon );
                ImageView indicator = (ImageView) view.findViewById( R.id.indicator );

                text1.setText(item.getTitle());
                icon.setImageDrawable( item.getIcon() );
                
                if( item.isChecked() )
                    view.setBackgroundColor( 0x44FFFFFF );
                else
                    view.setBackgroundColor( 0x0 );
                
                if( item.getSubMenu() == null )
                    indicator.setImageResource( 0x0 );
                else if( isExpanded )
                    indicator.setImageResource( R.drawable.ic_arrow_u );
                else
                    indicator.setImageResource( R.drawable.ic_arrow_d );
            }
            return view;
        }
        
        @Override
        public boolean hasStableIds()
        {
            return true;
        }
    }
    
    public static class OnClickListener
    {
        OnClickListener()
        {
        }
        
        public void onClick( MenuItem menuItem )
        {
        }
    }
}
