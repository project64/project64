package emu.project64.task;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import android.content.res.AssetManager;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;

public class ExtractAssetZipTask extends AsyncTask<Void, String, List<ExtractAssetZipTask.Failure>>
{
    public interface ExtractAssetZipListener
    {
        public void onExtractAssetsProgress( String nextFileToExtract );
        public void onExtractAssetsFinished( List<Failure> failures );
    }
    
    public ExtractAssetZipTask( AssetManager assetManager, String dstPath, ExtractAssetZipListener listener)
    {
        if (assetManager == null )
            throw new IllegalArgumentException( "Asset manager cannot be null" );
        if( TextUtils.isEmpty( dstPath ) )
            throw new IllegalArgumentException( "Destination path cannot be null or empty" );

        mAssetManager = assetManager;
        mDstPath = dstPath;
        mListener = listener;
    }
    
    private final AssetManager mAssetManager;
    private final String mDstPath;
    private final ExtractAssetZipListener mListener;
    
    @Override
    protected List<Failure> doInBackground( Void... params )
    {
        return extractAssets( mDstPath );
    }
    
    @Override
    protected void onProgressUpdate( String... values )
    {
        mListener.onExtractAssetsProgress( values[0] );
    }
    
    @Override
    protected void onPostExecute( List<ExtractAssetZipTask.Failure> result )
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
    
    private List<Failure> extractAssets( String dstPath )
    {
        final List<Failure> failures = new ArrayList<Failure>();

        // Ensure the parent directories exist
        File root = new File(dstPath + "/");
        if (!root.exists()) {
            root.mkdirs();
        }

        String dstFile = dstPath + "/assets.zip";
        String srcFile = "assets.zip";
        // Call the progress listener before extracting
        publishProgress( dstPath );

        // IO objects, initialize null to eliminate lint error
        OutputStream out = null;
        InputStream in = null;

        Boolean ExtractZip = false;
        // Extract the file
        try
        {
            out = new FileOutputStream( dstFile );
            in = mAssetManager.open( srcFile);
            byte[] buffer = new byte[1024];
            int read;

            mListener.onExtractAssetsProgress( "copying asset zip file" );
            while( ( read = in.read( buffer ) ) != -1 )
            {
                out.write( buffer, 0, read );
            }
            mListener.onExtractAssetsProgress( "Finished copying assset zip" );
            out.flush();
            ExtractZip = true;
        }
        catch( FileNotFoundException e )
        {
            Failure failure = new Failure( srcFile, dstPath, Failure.Reason.FILE_UNWRITABLE );
            Log.e( "ExtractAssetZipTask", failure.toString() );
            failures.add( failure );
        }
        catch( IOException e )
        {
            Failure failure = new Failure( srcFile, dstPath, Failure.Reason.ASSET_IO_EXCEPTION );
            Log.e( "ExtractAssetZipTask", failure.toString() );
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
                    Failure failure = new Failure( srcFile, dstPath, Failure.Reason.FILE_UNCLOSABLE );
                    Log.e( "ExtractAssetZipTask", failure.toString() );
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
                    Failure failure = new Failure( srcFile, dstPath, Failure.Reason.ASSET_UNCLOSABLE );
                    Log.e( "ExtractAssetZipTask", failure.toString() );
                    failures.add( failure );
                }
            }
        }

        if (ExtractZip)
        {
            String zipFilePath = dstPath + "/assets.zip";
            String destDir = dstPath + "/";
            File dir = new File(destDir);
            // create output directory if it doesn't exist
            if(!dir.exists()) dir.mkdirs();
            FileInputStream fis;
            //buffer for read and write data to file
            byte[] buffer = new byte[1024];
            try {
                fis = new FileInputStream(zipFilePath);
                ZipInputStream zis = new ZipInputStream(fis);
                ZipEntry ze = zis.getNextEntry();
                while(ze != null){
                    String fileName = ze.getName();
                    File newFile = new File(destDir + File.separator + fileName.replace("\\", "/"));
                    mListener.onExtractAssetsProgress( "Unzipping "+fileName );

                    //create directories for sub directories in zip
                    new File(newFile.getParent()).mkdirs();
                    if (ze.getSize() != 0) {
                        FileOutputStream fos = new FileOutputStream(newFile);
                        int len;
                        while ((len = zis.read(buffer)) > 0) {
                            fos.write(buffer, 0, len);
                        }
                        fos.close();
                    }
                    //close this ZipEntry
                    zis.closeEntry();
                    ze = zis.getNextEntry();
                }
                //close last ZipEntry
                zis.closeEntry();
                zis.close();
                fis.close();
            } catch (IOException e) {
                Failure failure = new Failure( srcFile, dstPath, Failure.Reason.ASSET_IO_EXCEPTION );
                Log.e( "ExtractAssetZipTask", failure.toString() );
                failures.add( failure );
            }
        }
        return failures;
    }
}
