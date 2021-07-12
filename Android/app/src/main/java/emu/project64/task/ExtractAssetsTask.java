package emu.project64.task;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.content.res.AssetManager;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;

public class ExtractAssetsTask extends AsyncTask<Void, String, List<ExtractAssetsTask.Failure>>
{
    public interface ExtractAssetsListener
    {
        public void onExtractAssetsProgress( String nextFileToExtract );
        public void onExtractAssetsFinished( List<Failure> failures );
    }
    
    public ExtractAssetsTask( AssetManager assetManager, String srcPath, String dstPath, ExtractAssetsListener listener )
    {
        if (assetManager == null )
            throw new IllegalArgumentException( "Asset manager cannot be null" );
        if( TextUtils.isEmpty( srcPath ) )
            throw new IllegalArgumentException( "Source path cannot be null or empty" );
        if( TextUtils.isEmpty( dstPath ) )
            throw new IllegalArgumentException( "Destination path cannot be null or empty" );
        if( listener == null )
            throw new IllegalArgumentException( "Listener cannot be null" );
        
        mAssetManager = assetManager;
        mSrcPath = srcPath;
        mDstPath = dstPath;
        mListener = listener;
    }
    
    private final AssetManager mAssetManager;
    private final String mSrcPath;
    private final String mDstPath;
    private final ExtractAssetsListener mListener;
    
    @Override
    protected List<Failure> doInBackground( Void... params )
    {
        return extractAssets( mSrcPath, mDstPath );
    }
    
    @Override
    protected void onProgressUpdate( String... values )
    {
        mListener.onExtractAssetsProgress( values[0] );
    }
    
    @Override
    protected void onPostExecute( List<ExtractAssetsTask.Failure> result )
    {
        mListener.onExtractAssetsFinished( result );
    }
    
    public static final class Failure
    {
        public enum Reason
        {
            FILE_UNWRITABLE,
            FILE_UNCLOSABLE,
            ASSET_UNCLOSABLE,
            ASSET_IO_EXCEPTION,
            FILE_IO_EXCEPTION,
        }
        
        public final String srcPath;
        public final String dstPath;
        public final Reason reason;
        public Failure( String srcPath, String dstPath, Reason reason )
        {
            this.srcPath = srcPath;
            this.dstPath = dstPath;
            this.reason = reason;
        }
        
        @Override
        public String toString()
        {
            switch( reason )
            {
                case FILE_UNWRITABLE:
                    return "Failed to open file " + dstPath;
                case FILE_UNCLOSABLE:
                    return "Failed to close file " + dstPath;
                case ASSET_UNCLOSABLE:
                    return "Failed to close asset " + srcPath;
                case ASSET_IO_EXCEPTION:
                    return "Failed to extract asset " + srcPath + " to file " + dstPath;
                case FILE_IO_EXCEPTION:
                    return "Failed to add file " + srcPath + " to file " + dstPath;
                default:
                    return "Failed using source " + srcPath + " and destination " + dstPath;
            }
        }
    }
    
    private List<Failure> extractAssets( String srcPath, String dstPath )
    {
        final List<Failure> failures = new ArrayList<Failure>();
        
        if( srcPath.startsWith( "/" ) )
            srcPath = srcPath.substring( 1 );
        
        String[] srcSubPaths = getAssetList( mAssetManager, srcPath );
        
        if( srcSubPaths.length > 0 )
        {
            // srcPath is a directory
            
            // Ensure the parent directories exist
            new File( dstPath ).mkdirs();
            
            // Some files are too big for Android 2.2 and below, so we break them into parts.
            // We use a simple naming scheme where we just append .part0, .part1, etc.
            Pattern pattern = Pattern.compile( "(.+)\\.part(\\d+)$" );
            HashMap<String, Integer> fileParts = new HashMap<String, Integer>();
            
            // Recurse into each subdirectory
            for( String srcSubPath : srcSubPaths )
            {
                Matcher matcher = pattern.matcher( srcSubPath );
                if( matcher.matches() )
                {
                    String name = matcher.group(1);
                    if( fileParts.containsKey( name ) )
                        fileParts.put( name, fileParts.get( name ) + 1 );
                    else
                        fileParts.put( name, 1 );
                }
                String suffix = "/" + srcSubPath;
                failures.addAll( extractAssets( srcPath + suffix, dstPath + suffix ) );
            }
            
            // Combine the large broken files, if any
            combineFileParts( fileParts, dstPath );
        }
        else // srcPath is a file.
        {
            // Call the progress listener before extracting
            publishProgress( dstPath );
            
            // IO objects, initialize null to eliminate lint error
            OutputStream out = null;
            InputStream in = null;
            
            // Extract the file
            try
            {
                out = new FileOutputStream( dstPath );
                in = mAssetManager.open( srcPath );
                byte[] buffer = new byte[1024];
                int read;
                
                while( ( read = in.read( buffer ) ) != -1 )
                {
                    out.write( buffer, 0, read );
                }
                out.flush();
            }
            catch( FileNotFoundException e )
            {
                Failure failure = new Failure( srcPath, dstPath, Failure.Reason.FILE_UNWRITABLE ); 
                Log.e( "ExtractAssetsTask", failure.toString() );
                failures.add( failure );
            }
            catch( IOException e )
            {
                Failure failure = new Failure( srcPath, dstPath, Failure.Reason.ASSET_IO_EXCEPTION ); 
                Log.e( "ExtractAssetsTask", failure.toString() );
                failures.add( failure );
            }
            finally
            {
                if( out != null )
                {
                    try
                    {
                        out.close();
                    }
                    catch( IOException e )
                    {
                        Failure failure = new Failure( srcPath, dstPath, Failure.Reason.FILE_UNCLOSABLE ); 
                        Log.e( "ExtractAssetsTask", failure.toString() );
                        failures.add( failure );
                    }
                }
                if( in != null )
                {
                    try
                    {
                        in.close();
                    }
                    catch( IOException e )
                    {
                        Failure failure = new Failure( srcPath, dstPath, Failure.Reason.ASSET_UNCLOSABLE ); 
                        Log.e( "ExtractAssetsTask", failure.toString() );
                        failures.add( failure );
                    }
                }
            }
        }
        
        return failures;
    }
    
    private static String[] getAssetList( AssetManager assetManager, String srcPath )
    {
        String[] srcSubPaths = null;
        
        try
        {
            srcSubPaths = assetManager.list( srcPath );
        }
        catch( IOException e )
        {
            Log.w( "ExtractAssetsTask", "Failed to get asset file list." );
        }
        
        return srcSubPaths;
    }

    private static List<Failure> combineFileParts( Map<String, Integer> filePieces, String dstPath )
    {
        List<Failure> failures = new ArrayList<Failure>();
        for (String name : filePieces.keySet() )
        {
            String src = null;
            String dst = dstPath + "/" + name;
            OutputStream out = null;
            InputStream in = null;
            try
            {
                out = new FileOutputStream( dst );
                byte[] buffer = new byte[1024];
                int read;
                for( int i = 0; i < filePieces.get( name ); i++ )
                {
                    src = dst + ".part" + i;
                    try
                    {
                        in = new FileInputStream( src );
                        while( ( read = in.read( buffer ) ) != -1 )
                        {
                            out.write( buffer, 0, read );
                        }
                        out.flush();
                    }
                    catch( IOException e )
                    {
                        Failure failure = new Failure( src, dst, Failure.Reason.FILE_IO_EXCEPTION ); 
                        Log.e( "ExtractAssetsTask", failure.toString() );
                        failures.add( failure );
                    }
                    finally
                    {
                        if( in != null )
                        {
                            try
                            {
                                in.close();
                                new File( src ).delete();
                            }
                            catch( IOException e )
                            {
                                Failure failure = new Failure( src, dst, Failure.Reason.FILE_UNCLOSABLE ); 
                                Log.e( "ExtractAssetsTask", failure.toString() );
                                failures.add( failure );
                            }
                        }
                    }
                }
            }
            catch( FileNotFoundException e )
            {
                Failure failure = new Failure( src, dst, Failure.Reason.FILE_UNWRITABLE ); 
                Log.e( "ExtractAssetsTask", failure.toString() );
                failures.add( failure );
            }
            finally
            {
                if( out != null )
                {
                    try
                    {
                        out.close();
                    }
                    catch( IOException e )
                    {
                        Failure failure = new Failure( src, dst, Failure.Reason.FILE_UNCLOSABLE ); 
                        Log.e( "ExtractAssetsTask", failure.toString() );
                        failures.add( failure );
                    }
                }
            }
        }
        return failures;
    }
}
