/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
package emu.project64.preference;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import emu.project64.R;

import emu.project64.AndroidDevice;
import emu.project64.dialog.Prompt;
import emu.project64.util.FileUtil;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.os.Environment;
import android.os.Parcelable;
import android.preference.DialogPreference;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.widget.ArrayAdapter;

/**
 * A {@link DialogPreference} that is specifically for choosing a directory path or file on a device.
 */
public class PathPreference extends DialogPreference
{
    /** The user must select a directory. No files will be shown in the list. */
    public static final int SELECTION_MODE_DIRECTORY = 0;
    
    /** The user must select a file. The dialog will only close when a file is selected. */
    public static final int SELECTION_MODE_FILE = 1;
    
    /** The user may select a file or a directory. The Ok button must be used. */
    public static final int SELECTION_MODE_ANY = 2;
    
    private static final String STORAGE_DIR = Environment.getExternalStorageDirectory().getAbsolutePath();
    
    private final boolean mUseDefaultSummary;
    private int mSelectionMode = SELECTION_MODE_ANY;
    private boolean mDoReclick = false;
    private final List<CharSequence> mNames = new ArrayList<CharSequence>();
    private final List<String> mPaths = new ArrayList<String>();
    private String mNewValue;
    private String mValue;

    /**
     * Constructor
     *
     * @param context The {@link Context} that this PathPreference is being used in.
     * @param attrs   A collection of attributes, as found associated with a tag in an XML document.
     */
    public PathPreference( Context context, AttributeSet attrs )
    {
        super( context, attrs );
        
        mUseDefaultSummary = TextUtils.isEmpty( getSummary() );
        
        // Get the selection mode from the XML file, if provided
        TypedArray a = context.obtainStyledAttributes( attrs, R.styleable.PathPreference );
        mSelectionMode = a.getInteger( R.styleable.PathPreference_selectionMode, SELECTION_MODE_ANY );
        a.recycle();
    }

    /**
     * Sets the path that PathPrefence will use.
     * 
     * @param value The path that this PathPreference instance will use.
     */
    public void setValue( String value )
    {
        mValue = validate( value );
        if( shouldPersist() )
            persistString( mValue );
        
        // Summary always reflects the true/persisted value, does not track the temporary/new value
        if( mUseDefaultSummary )
            setSummary( mSelectionMode == SELECTION_MODE_FILE ? new File( mValue ).getName() : mValue );
        
        // Reset the dialog info
        populate( mValue );
    }

    /**
     * Sets the specific selection mode to use.
     * 
     * @param value The selection mode to use.</p>
     *              <li>0 = Directories can only be used as a choice.
     *              <li>1 = Files can only be used as a choice.
     *              <li>2 = Directories and files can be used as a choice.</li>
     */
    public void setSelectionMode( int value )
    {
        mSelectionMode = value;
    }

    /**
     * Gets the path value being used.
     * 
     * @return The path value being used by this PathPreference.
     */
    public String getValue()
    {
        return mValue;
    }

    /**
     * Gets the current selection mode being used.
     * 
     * @return The current selection mode being used by this PathPreference.
     */
    public int getSelectionMode()
    {
        return mSelectionMode;
    }
    
    @Override
    protected Object onGetDefaultValue( TypedArray a, int index )
    {
        return a.getString( index );
    }
    
    @Override
    protected void onSetInitialValue( boolean restorePersistedValue, Object defaultValue )
    {
        setValue( restorePersistedValue ? getPersistedString( mValue ) : (String) defaultValue );
    }
    
    @Override
    protected void onPrepareDialogBuilder( Builder builder )
    {
        super.onPrepareDialogBuilder( builder );
        
        // Add the list entries
        if( AndroidDevice.IS_HONEYCOMB )
        {
            // Holo theme has folder icons and "Parent folder" text
            ArrayAdapter<String> adapter = Prompt.createFilenameAdapter( getContext(), mPaths, mNames );
            builder.setAdapter( adapter, this );
        }
        else
        {
            // Basic theme uses bold text for folders and ".." for the parent
            CharSequence[] items = mNames.toArray( new CharSequence[mNames.size()] );
            builder.setItems( items, this );
        }
        
        // Remove the Ok button when user must choose a file
        if( mSelectionMode == SELECTION_MODE_FILE )
            builder.setPositiveButton( null, null );
    }
    
    @Override
    public void onClick( DialogInterface dialog, int which )
    {
        // If the user clicked a list item...
        if( which >= 0 && which < mPaths.size() )
        {
            mNewValue = mPaths.get( which );
            File path = new File( mNewValue );
            if( path.isDirectory() )
            {
                // ...navigate into...
                populate( mNewValue );
                mDoReclick = true;
            }
            else
            {
                // ...or close dialog positively
                which = DialogInterface.BUTTON_POSITIVE;
            }
        }
        
        // Call super last, parameters may have changed above
        super.onClick( dialog, which );
    }
    
    @Override
    protected void onDialogClosed( boolean positiveResult )
    {
        super.onDialogClosed( positiveResult );
        
        if( positiveResult && callChangeListener( mNewValue ) )
        {
            // User clicked Ok: clean the state by persisting value
            setValue( mNewValue );
        }
        else if( mDoReclick )
        {
            // User clicked a list item: maintain dirty value and re-open
            mDoReclick = false;
            onClick();
        }
        else
        {
            // User clicked Cancel/Back: clean state by restoring persisted value
            populate( mValue );
        }
    }
    
    @Override
    protected Parcelable onSaveInstanceState()
    {
        final SavedStringState myState = new SavedStringState( super.onSaveInstanceState() );
        myState.mValue = mNewValue;
        return myState;
    }
    
    @Override
    protected void onRestoreInstanceState( Parcelable state )
    {
        if( state == null || !state.getClass().equals( SavedStringState.class ) )
        {
            // Didn't save state for us in onSaveInstanceState
            super.onRestoreInstanceState( state );
            return;
        }
        
        final SavedStringState myState = (SavedStringState) state;
        super.onRestoreInstanceState( myState.getSuperState() );
        populate( myState.mValue );
        
        // If the dialog is already showing, we must close and reopen to refresh the contents
        // TODO: Find a less hackish solution, if one exists
        if( getDialog() != null )
        {
            mDoReclick = true;
            getDialog().dismiss();
        }
    }

    // Populates the dialog view with files and folders on the device.
    private void populate( String path )
    {
        // Cache the path to persist on Ok
        mNewValue = path;
        
        // Quick exit if null
        if( path == null )
            return;
        
        // If start path is a file, list it and its siblings in the parent directory
        File startPath = new File( path );
        if( startPath.isFile() )
            startPath = startPath.getParentFile();
        
        // Set the dialog title based on the selection mode
        switch( mSelectionMode )
        {
            case SELECTION_MODE_FILE:
                // If selecting only files, set title to parent directory name
                setDialogTitle( startPath.getPath() );
                break;
            case SELECTION_MODE_DIRECTORY:
            case SELECTION_MODE_ANY:
                // Otherwise clarify the directory that will be selected if user clicks Ok
                setDialogTitle( getContext().getString( R.string.pathPreference_dialogTitle,
                        startPath.getPath() ) );
                break;
        }
        
        // Populate the key-value pairs for the list entries
        boolean isFilesIncluded = mSelectionMode != SELECTION_MODE_DIRECTORY;
        FileUtil.populate( startPath, true, true, isFilesIncluded, mNames, mPaths );
    }

    private static String validate( String value )
    {
        if( TextUtils.isEmpty( value ) )
        {
            // Use storage directory if value is empty
            value = STORAGE_DIR;
        }
        else
        {
            // Non-empty string provided
            // Prefixes encode additional information:
            // ! and ~ mean path is relative to storage dir
            // ! means parent dirs should be created if path does not exist
            // ~ means storage dir should be used if path does not exist
            boolean isRelativePath = value.startsWith( "!" ) || value.startsWith( "~" );
            boolean forceParentDirs = value.startsWith( "!" );
            
            // Build the absolute path if necessary
            if( isRelativePath )
                value = STORAGE_DIR + "/" + value.substring( 1 );
            
            // Ensure the parent directories exist if requested
            File file = new File( value );
            if( forceParentDirs )
                file.mkdirs();
            else if( !file.exists() )
                value = STORAGE_DIR;
        }
        return value;
    }
}
