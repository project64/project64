package emu.project64.util;

import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import emu.project64.AndroidDevice;
import android.text.Html;
import android.text.TextUtils;

/**
 * Utility class that provides methods which simplify file I/O tasks.
 */
public final class FileUtil
{
    public static void populate( File startPath, boolean includeParent, boolean includeDirectories,
            boolean includeFiles, List<CharSequence> outNames, List<String> outPaths )
    {
        if( !startPath.exists() )
        {
            return;
        }

        if(startPath.isFile())
        {
            startPath = startPath.getParentFile();
        }

        if(startPath.getParentFile() == null)
        {
            includeParent = false;
        }
        
        outNames.clear();
        outPaths.clear();
        
        if( includeParent )
        {
            outNames.add( Html.fromHtml( "<b>..</b>" ) );
            boolean BaseDir = false;
            ArrayList<String> StorageDirectories = AndroidDevice.getStorageDirectories();
            for( String directory : StorageDirectories )
            {
                if (TextUtils.equals(startPath.getPath(), directory))
                {
                    BaseDir = true;
                    break;
                }
            }
            outPaths.add( BaseDir ? null : startPath.getParentFile().getPath() );
        }
        
        if(includeDirectories)
        {
            for( File directory : getContents( startPath, new VisibleDirectoryFilter() ) )
            {
                outNames.add( Html.fromHtml( "<b>" + directory.getName() + "</b>" ) );
                outPaths.add( directory.getPath() );
            }
        }
        
        if(includeFiles)
        {
            for( File file : getContents( startPath, new VisibleFileFilter() ) )
            {
                outNames.add( Html.fromHtml( file.getName() ) );
                outPaths.add( file.getPath() );
            }
        }
    }
    
    public static List<File> getContents( File startPath, FileFilter fileFilter )
    {
        // Get a filtered, sorted list of files
        List<File> results = new ArrayList<>();
        File[] files = startPath.listFiles( fileFilter );

        if( files != null )
        {
            Collections.addAll( results, files );
            Collections.sort( results, new FileUtil.FileComparer() );
        }
        
        return results;
    }
    
    private static class FileComparer implements Comparator<File>
    {
        // Compare files first by directory/file then alphabetically (case-insensitive)
        @Override
        public int compare( File lhs, File rhs )
        {
            if( lhs.isDirectory() && rhs.isFile() )
                return -1;
            else if( lhs.isFile() && rhs.isDirectory() )
                return 1;
            else
                return lhs.getName().compareToIgnoreCase( rhs.getName() );
        }
    }
    
    private static class VisibleFileFilter implements FileFilter
    {
        // Include only non-hidden files not starting with '.'
        @Override
        public boolean accept( File pathname )
        {
            return ( pathname != null ) && ( pathname.isFile() ) && ( !pathname.isHidden() )
                    && ( !pathname.getName().startsWith( "." ) );
        }
    }
    
    private static class VisibleDirectoryFilter implements FileFilter
    {
        // Include only non-hidden directories not starting with '.'
        @Override
        public boolean accept( File pathname )
        {
            return ( pathname != null ) && ( pathname.isDirectory() ) && ( !pathname.isHidden() )
                    && ( !pathname.getName().startsWith( "." ) );
        }
    }
    
    /**
     * Deletes a given folder directory in the form of a {@link File}
     * 
     * @param folder The folder to delete.
     * 
     * @return True if the folder was deleted, false otherwise.
     */
    public static boolean deleteFolder( File folder )
    {
        if( folder.isDirectory() )
        {
            String[] children = folder.list();
            if( children != null )
            {
                for( String child : children )
                {
                    boolean success = deleteFolder( new File( folder, child ) );
                    if( !success )
                        return false;
                }
            }
        }
        
        return folder.delete();
    }
    
    public static String getFileNameFromPath(String path)
    {
        if (path == null)
        {
            return "";
        }
        int index = path.lastIndexOf('/');
        return index > -1 ? path.substring(index+1) : path;
    }
}
