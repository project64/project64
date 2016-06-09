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

import android.os.Parcel;
import android.os.Parcelable;
import android.preference.Preference;
import android.preference.Preference.BaseSavedState;

public class SavedStringState extends BaseSavedState
{
    String mValue;
    
    public SavedStringState( Parcel source )
    {
        super( source );
        mValue = source.readString();
    }
    
    @Override
    public void writeToParcel( Parcel dest, int flags )
    {
        super.writeToParcel( dest, flags );
        dest.writeString( mValue );
    }
    
    public SavedStringState( Parcelable superState )
    {
        super( superState );
    }
    
    public static final Parcelable.Creator<SavedStringState> CREATOR = new Parcelable.Creator<SavedStringState>()
    {
        @Override
        public SavedStringState createFromParcel( Parcel in )
        {
            return new SavedStringState( in );
        }

        @Override
        public SavedStringState[] newArray( int size )
        {
            return new SavedStringState[size];
        }
    };
    
    public static Parcelable onSaveInstanceState( final Parcelable superState,
            Preference preference, String value )
    {
        if( preference.isPersistent() )
        {
            // No need to save instance state since it's persistent
            return superState;
        }
        
        final SavedStringState myState = new SavedStringState( superState );
        myState.mValue = value;
        return myState;
    }
}
