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
package emu.project64.dialog;

import java.io.File;
import java.util.List;

import emu.project64.R;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

/**
 * A utility class that generates dialogs to prompt the user for information.
 */
public final class Prompt
{
    /**
     * An interface that simplifies the population of list items.
     * 
     * @param <T> The type of the data to be wrapped.
     * @see Prompt#createAdapter(Context, List, int, int, ListItemPopulator)
     */
    public interface ListItemPopulator<T>
    {
        public void onPopulateListItem( T item, int position, View view );
    }
    
    /**
     * An interface that simplifies the population of list items having two text fields and an icon.
     * 
     * @param <T> The type of the data to be wrapped.
     * @see Prompt#createAdapter(Context, List, ListItemTwoTextIconPopulator)
     */
    public interface ListItemTwoTextIconPopulator<T>
    {
        public void onPopulateListItem( T item, int position, TextView text1, TextView text2,
                ImageView icon );
    }
    
    /**
     * Create a {@link ListAdapter} where each list item has a specified layout.
     * 
     * @param <T>         The type of the data to be wrapped.
     * @param context     The current context.
     * @param items       The data source for the list items.
     * @param layoutResId The layout resource to be used for each list item.
     * @param textResId   The {@link TextView} resource within the layout to be populated by default.
     * @param populator   The object to populate the fields in each list item.
     * 
     * @return An adapter that can be used to create list dialogs.
     */
    public static <T> ArrayAdapter<T> createAdapter( Context context, List<T> items,
            final int layoutResId, final int textResId, final ListItemPopulator<T> populator )
    {
        return new ArrayAdapter<T>( context, layoutResId, textResId, items )
        {
            @Override
            public View getView( int position, View convertView, ViewGroup parent )
            {
                View row;
                if( convertView == null )
                {
                    LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(
                            Context.LAYOUT_INFLATER_SERVICE );
                    row = (View) inflater.inflate( layoutResId, null );
                }
                else
                {
                    row = (View) convertView;
                }
                
                populator.onPopulateListItem( getItem( position ), position, row );
                return row;
            }
        };
    }
    
    /**
     * Create a {@link ListAdapter} where each list item has two text fields and an icon.
     * 
     * @param <T>       The type of the data to be wrapped.
     * @param context   The activity context.
     * @param items     The data source for list items.
     * @param populator The object to populate the fields in each list item.
     * 
     * @return An adapter that can be used to create list dialogs.
     */
    public static <T> ArrayAdapter<T> createAdapter( Context context, List<T> items,
            final ListItemTwoTextIconPopulator<T> populator )
    {
        return createAdapter( context, items, R.layout.list_item_two_text_icon, R.id.text1,
                new ListItemPopulator<T>()
                {
                    @Override
                    public void onPopulateListItem( T item, int position, View view )
                    {
                        TextView text1 = (TextView) view.findViewById( R.id.text1 );
                        TextView text2 = (TextView) view.findViewById( R.id.text2 );
                        ImageView icon = (ImageView) view.findViewById( R.id.icon );
                        populator.onPopulateListItem( item, position, text1, text2, icon );
                    }
                } );
    }
    
    public static ArrayAdapter<String> createFilenameAdapter( Context context, List<String> paths,
            final List<CharSequence> names )
    {
        return createAdapter( context, paths, new ListItemTwoTextIconPopulator<String>()
        {
            @Override
            public void onPopulateListItem( String path, int position, TextView text1,
                    TextView text2, ImageView icon )
            {
                if( !TextUtils.isEmpty( path ) )
                {
                    String name = names.get( position ).toString();
                    if( name.equals( ".." ) )
                    {
                        text1.setText( R.string.pathPreference_parentFolder );
                        icon.setVisibility( View.VISIBLE );
                        icon.setImageResource( R.drawable.ic_arrow_u );
                    }
                    else
                    {
                        File file = new File( path );
                        text1.setText( name );
                        if( file.isDirectory() )
                        {
                            icon.setVisibility( View.VISIBLE );
                            icon.setImageResource( R.drawable.ic_folder );
                        }
                        else
                        {
                            icon.setVisibility( View.GONE );
                            icon.setImageResource( 0 );
                        }
                    }
                    text2.setVisibility( View.GONE );
                    text2.setText( null );
                }
            }
        } );
    }
}