package emu.project64;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import emu.project64.R;
import emu.project64.jni.LanguageStringID;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.util.FileUtil;
import emu.project64.util.Strings;
import android.content.Intent;
import android.content.Context;
import android.text.Html;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

public class ScanRomsActivity extends AppCompatActivity implements OnItemClickListener
{
    private static final String NAMESPACE = ScanRomsActivity.class.getCanonicalName() + ".";
    public static final String GAME_DIR_PATH = NAMESPACE + "GAME_DIR_PATH";
    public static final String GAME_DIR_RECURSIVELY = NAMESPACE + "GAME_DIR_RECURSIVELY";

    private List<CharSequence> mNames;
    private List<String> mPaths;
    private CheckBox mScanRecursively;
    private Button mCancelButton;
    private Button mOkButton;

    private File mCurrentPath = null;
    public final static String EXTERNAL_PUBLIC_DIRECTORY = Environment.getExternalStorageDirectory().getPath();

    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.scan_roms_activity);
        if (NativeExports.SettingsLoadBool(SettingsID.RomList_GameDirUseSelected.toString()))
        {
            File CurrentPath = new File(NativeExports.SettingsLoadString(SettingsID.RomList_GameDir.toString()));
            if (CurrentPath.exists() && CurrentPath.isDirectory())
            {
                mCurrentPath = CurrentPath;
            }
        }
        TextView selectDir = (TextView)findViewById(R.id.scanRomsDialog_selectDir);
        selectDir.setText(Strings.GetString(LanguageStringID.ANDROID_SELECTDIR));

        // Set check box state
        mScanRecursively = (CheckBox) findViewById( R.id.ScanRecursively );
        mScanRecursively.setChecked( NativeExports.SettingsLoadBool(SettingsID.RomList_GameDirRecursive.toString()) );
        mScanRecursively.setText(Strings.GetString(LanguageStringID.ANDROID_INCLUDE_SUBDIRECTORIES));

        mCancelButton = (Button) findViewById( R.id.buttonCancel );
        mCancelButton.setText(Strings.GetString(LanguageStringID.ANDROID_CANCEL));
        mCancelButton.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View v)
            {
                ScanRomsActivity.this.setResult(RESULT_CANCELED, null);
                ScanRomsActivity.this.finish();
            }
        });

        mOkButton = (Button) findViewById( R.id.buttonOk );
        mOkButton.setText(Strings.GetString(LanguageStringID.ANDROID_OK));
        mOkButton.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View v)
            {
                Intent data = new Intent();

                data.putExtra(GAME_DIR_PATH, mCurrentPath.getPath());
                data.putExtra(GAME_DIR_RECURSIVELY, mScanRecursively.isChecked());
                ScanRomsActivity.this.setResult(RESULT_OK, data);
                ScanRomsActivity.this.finish();
            }
        });

        PopulateFileList();
    }

    private void PopulateFileList()
    {
        // Get the filenames and absolute paths
        ArrayList<String> StorageDirectories = AndroidDevice.getStorageDirectories();
        mNames = new ArrayList<CharSequence>();
        mPaths = new ArrayList<String>();
        if (mCurrentPath != null)
        {
            String path = mCurrentPath.getPath();
            for( String directory : StorageDirectories )
            {
                if (path.startsWith(directory))
                {
                    String BaseDir = TextUtils.equals(AndroidDevice.EXTERNAL_PUBLIC_DIRECTORY, directory) ? Strings.GetString(LanguageStringID.ANDROID_INTERNAL_MEMORY) : new File(directory).getName();
                    path = BaseDir + path.substring(directory.length());
                    break;
                }
            }
            setTitle( path );
            FileUtil.populate( mCurrentPath, true, true, true, mNames, mPaths );
            mOkButton.setEnabled(true);
        }
        else
        {
            setTitle( Strings.GetString(LanguageStringID.ANDROID_DIRECTORIES) );
            mPaths = StorageDirectories;
            for( String directory : mPaths )
            {
                if (TextUtils.equals(AndroidDevice.EXTERNAL_PUBLIC_DIRECTORY, directory))
                {
                    mNames.add( Html.fromHtml( "<b>" + Strings.GetString(LanguageStringID.ANDROID_INTERNAL_MEMORY) + "</b>" ) );
                }
                else
                {
                    mNames.add( Html.fromHtml( "<b>" + new File(directory).getName() + "</b>" ) );
                }
            }
            mOkButton.setEnabled(false);
        }
        ListView listView1 = (ListView) findViewById( R.id.listView1 );
        ArrayAdapter<String> adapter = createScanRomsAdapter( this, mPaths, mNames );
        listView1.setAdapter( adapter );
        listView1.setOnItemClickListener( this );
    }

    @Override
    public void onItemClick( AdapterView<?> parent, View view, int position, long id )
    {
        mCurrentPath = mPaths.get( position ) != null ? new File(mPaths.get( position )) : null;
        PopulateFileList();
    }

    /**
     * An interface that simplifies the population of list items.
     *
     * @param <T> The type of the data to be wrapped.
     * @see ScanRomsActivity#createAdapter(Context, List, int, int, ListItemPopulator)
     */
    public interface ListItemPopulator<T>
    {
        public void onPopulateListItem( T item, int position, View view );
    }

    /**
     * An interface that simplifies the population of list items having two text fields and an icon.
     *
     * @param <T> The type of the data to be wrapped.
     * @see ScanRomsActivity#createAdapter(Context, List, ListItemTwoTextIconPopulator)
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

            @Override
            public boolean isEnabled(int position)
            {
                T item = getItem( position );
                if (item != null)
                {
                    File file = new File( (String)item );
                    return file.isDirectory();
                }
                return true;
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

    public static ArrayAdapter<String> createScanRomsAdapter( Context context, List<String> paths,
            final List<CharSequence> names )
    {
        return createAdapter( context, paths, new ListItemTwoTextIconPopulator<String>()
        {
            @Override
            public void onPopulateListItem( String path, int position, TextView text1,
                    TextView text2, ImageView icon )
            {
                String name = names.get( position ).toString();
                if( name.equals( ".." ) )
                {
                    text1.setText( Strings.GetString(LanguageStringID.ANDROID_PARENTFOLDER) );
                    icon.setVisibility( View.VISIBLE );
                    icon.setImageResource( R.drawable.ic_arrow_u );
                }
                else
                {
                    if( !TextUtils.isEmpty( path ) )
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
                }
                text2.setVisibility( View.GONE );
                text2.setText( null );
            }
        } );
    }
}
