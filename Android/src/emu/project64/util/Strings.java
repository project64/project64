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
package emu.project64.util;

import java.nio.charset.Charset;
import java.util.List;

import emu.project64.jni.LanguageStringID;
import emu.project64.jni.NativeExports;
import android.view.Menu;
import android.view.MenuItem;

public class Strings 
{
    static public boolean startsWith(String[] array, String text) 
    {
        for (String item : array)
        {
            if (text.startsWith(item))
            {
                return true;
            }
        }
        return false;
    }

    static public int containsName(List<String> array, String text) 
    {
        for (int i = array.size()-1 ; i >= 0 ; --i)
        {
            if (array.get(i).endsWith(text))
            {
                return i;
            }
        }
        return -1;
    }
    
    static public String GetString(LanguageStringID StringId)
    {
    	byte[] bytes = NativeExports.GetString(StringId.getValue());
    	return (bytes.length > 0 ? new String(bytes, Charset.forName("UTF8")) : new String("#" + Integer.toString(StringId.getValue()) + "#"));
    }
    
    static public void SetMenuTitle(Menu menu, int MenuItemID, LanguageStringID StringId)
    {
    	MenuItem menuitem = menu.findItem(MenuItemID);
    	if (menuitem == null)
    	{
    		return;
    	}
    	menuitem.setTitle(GetString(StringId));
    }
}
