package emu.project64.util;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import emu.project64.Project64Application;

/**
 * Utility class which collects a bunch of commonly used methods into one class.
 */
public final class Utility
{
    
    /**
     * Clamps a value to the limit defined by min and max.
     * 
     * @param val The value to clamp to min and max.
     * @param min The lowest number val can be equal to.
     * @param max The largest number val can be equal to.
     * 
     * @return If the value is lower than min, min is returned. <br/>
     *         If the value is higher than max, max is returned.
     */
    public static<T extends Comparable<? super T>> T clamp( T val, T min, T max )
    {
        final T temp;

        //  val < max
        if ( val.compareTo(max) < 0 )
            temp = val;
        else
            temp = max;

        // temp > min
        if ( temp.compareTo(min) > 0 )
            return temp;
        else
            return min;
    }
    
    public static String executeShellCommand(String... args)
    {
        try
        {
            Process process = Runtime.getRuntime().exec( args );
            BufferedReader reader = new BufferedReader( new InputStreamReader( process.getInputStream() ) );
            StringBuilder result = new StringBuilder();
            String line;
            while( ( line = reader.readLine() ) != null )
            {
                result.append( line + "\n" );
            }
            return result.toString();
        }
        catch( IOException ignored )
        {
        }
        return "";
    }
    
    public static boolean close(Closeable closeable) 
    {
        if (closeable != null)
        {
            try 
            {
                closeable.close();
                return true;
            } 
            catch (IOException e) 
            {
            }
        }
        return false;
    }
    
    public static String readAsset(String assetName, String defaultS)
    {
        InputStream is = null;
        BufferedReader r = null;
        try 
        {
            is = Project64Application.getAppResources().getAssets().open(assetName);
            r = new BufferedReader(new InputStreamReader(is, "UTF8"));
            StringBuilder sb = new StringBuilder();
            String line = r.readLine();
            if(line != null)
            {
                sb.append(line);
                line = r.readLine();
                while(line != null) 
                {
                    sb.append('\n');
                    sb.append(line);
                    line = r.readLine();
                }
            }
            return sb.toString();
        }
        catch (IOException e)
        {
            return defaultS;
        }
        finally 
        {
            close(is);
            close(r);
        }
    }
}
