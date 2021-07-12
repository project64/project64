package emu.project64;

import java.io.File;
import java.util.Comparator;
import java.util.List;

import emu.project64.R;
import android.content.Context;
import android.graphics.Color;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

public class GalleryItem
{
    public final String goodName;
    public final String fileName;
    public final File romFile;
    public final int textColor;
    public final Context context;
    public final boolean isHeading;

    public GalleryItem( Context context, String goodName, String fileName, String romPath, int textColor )
    {
        this.goodName = goodName;
        this.fileName = fileName;
        this.context = context;
        this.textColor = textColor;
        this.isHeading = false;
        this.romFile = TextUtils.isEmpty( romPath ) ? null : new File( romPath );
    }

    public GalleryItem( Context context, String headingName )
    {
        this.goodName = headingName;
        this.fileName = null;
        this.context = context;
        this.isHeading = true;
        this.romFile = null;
        this.textColor = 0;
    }

    @Override
    public String toString()
    {
        if( !TextUtils.isEmpty( goodName ) )
        {
            return goodName;
        }
        else if( romFile != null && !TextUtils.isEmpty( romFile.getName() ) )
        {
            return romFile.getName();
        }
        else
        {
            return "unknown file";
        }
    }

    public static class NameComparator implements Comparator<GalleryItem>
    {
        @Override
        public int compare( GalleryItem item1, GalleryItem item2 )
        {
            return item1.toString().compareToIgnoreCase( item2.toString() );
        }
    }

    public static class ViewHolder extends RecyclerView.ViewHolder implements OnClickListener,
        OnLongClickListener
    {
        public GalleryItem item;
        private Context mContext;

        public ViewHolder( Context context, View view )
        {
            super( view );
            mContext = context;
            view.setOnClickListener( this );
            view.setOnLongClickListener( this );
        }

        @Override
        public String toString()
        {
            return item.toString();
        }

        @Override
        public void onClick( View view )
        {
            if( mContext instanceof GalleryActivity )
            {
                GalleryActivity activity = (GalleryActivity) mContext;
                activity.onGalleryItemClick( item );
            }
        }

        @Override
        public boolean onLongClick( View view )
        {
            if( mContext instanceof GalleryActivity )
            {
                GalleryActivity activity = (GalleryActivity) mContext;
                return activity.onGalleryItemLongClick( item );
            }
            return false;
        }
    }

    public static class Adapter extends RecyclerView.Adapter<ViewHolder>
    {
        private final Context mContext;
        private final List<GalleryItem> mObjects;

        public Adapter( Context context, List<GalleryItem> objects )
        {
            mContext = context;
            mObjects = objects;
        }

        @Override
        public int getItemCount()
        {
            return mObjects.size();
        }

        @Override
        public long getItemId( int position )
        {
            return 0;
        }

        @Override
        public int getItemViewType( int position )
        {
            return mObjects.get( position ).isHeading ? 1 : 0;
        }

        public void onBindViewHolder( ViewHolder holder, int position )
        {
            // Called by RecyclerView to display the data at the specified position.
            View view = holder.itemView;
            GalleryItem item = mObjects.get( position );
            holder.item = item;

            if( item != null )
            {
                TextView tv1 = (TextView) view.findViewById( R.id.text1 );
                tv1.setText( item.toString() );
                tv1.setTextColor(Color.rgb((item.textColor >> 16) & 0xff,(item.textColor >> 8) & 0xff,item.textColor & 0xff));

                LinearLayout linearLayout = (LinearLayout) view.findViewById( R.id.galleryItem );
                GalleryActivity activity = (GalleryActivity) item.context;

                if( item.isHeading )
                {
                    view.setClickable( false );
                    view.setLongClickable( false );
                    linearLayout.setPadding( 0, 0, 0, 0 );
                    tv1.setPadding( 5, 10, 0, 0 );
                    tv1.setTextSize( TypedValue.COMPLEX_UNIT_DIP, 18.0f );
                }
                else
                {
                    view.setClickable( true );
                    view.setLongClickable( true );
                    linearLayout.setPadding( activity.galleryHalfSpacing,
                            activity.galleryHalfSpacing, activity.galleryHalfSpacing,
                            activity.galleryHalfSpacing );
                    tv1.setPadding( 0, 0, 0, 0 );
                    tv1.setTextSize( TypedValue.COMPLEX_UNIT_DIP, 13.0f );
                }
                LinearLayout layout = (LinearLayout) view.findViewById( R.id.info );
                layout.getLayoutParams().width = activity.galleryWidth;
            }
        }

        public ViewHolder onCreateViewHolder( ViewGroup parent, int viewType )
        {
            LayoutInflater inflater = (LayoutInflater) mContext
                    .getSystemService( Context.LAYOUT_INFLATER_SERVICE );
            View view = inflater.inflate( R.layout.gallery_item_adapter, parent, false );
            return new ViewHolder( mContext, view );
        }
    }
}
