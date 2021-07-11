package emu.project64.persistent;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.Set;

import android.text.TextUtils;
import android.util.Log;

public class ConfigFile
{
    /** Name of the config file. */
    private final String mFilename;

    /** Sections mapped by title for easy lookup, with insertion order retained. */
    private final LinkedHashMap<String, ConfigSection> mConfigMap;

    /**
     * Reads the entire config file, and saves the data to internal collections for manipulation.
     *
     * @param filename The config file to read from.
     */
    public ConfigFile( String filename )
    {
        mFilename = filename;
        mConfigMap = new LinkedHashMap<String, ConfigSection>();
        reload();
    }

    /**
     * Looks up a config section by its title.
     *
     * @param sectionTitle Title of the section containing the parameter.
     *
     * @return A ConfigSection containing parameters, or null if not found.
     */
    public ConfigSection get( String sectionTitle )
    {
        return mConfigMap.get( sectionTitle );
    }

    /**
     * Looks up the specified parameter under the specified section title.
     *
     * @param sectionTitle Title of the section containing the parameter.
     * @param parameter Name of the parameter.
     *
     * @return The value of the specified parameter, or null if not found.
     */
    public String get( String sectionTitle, String parameter )
    {
        ConfigSection section = mConfigMap.get( sectionTitle );

        // The specified section doesn't exist or is empty.. quit
        if( section == null || section.parameters == null )
            return null;

        ConfigParameter confParam = section.parameters.get( parameter );

        // The specified parameter doesn't exist.. quit
        if( confParam == null )
            return null;

        // Got it
        return confParam.value;
    }

    /**
     * Assigns the specified value to the specified parameter under the specified section.
     *
     * @param sectionTitle The title of the section to contain the parameter.
     * @param parameter The name of the parameter.
     * @param value The value to give the parameter.
     */
    public void put( String sectionTitle, String parameter, String value )
    {
        ConfigSection section = mConfigMap.get( sectionTitle );
        if( section == null )
        {
            // Add a new section
            section = new ConfigSection( sectionTitle );
            mConfigMap.put( sectionTitle, section );
        }
        section.put( parameter, value );
    }

    /**
     * Erases any previously loaded data.
     */
    public void clear()
    {
        mConfigMap.clear();
    }

    /**
     * Re-loads the entire config file, overwriting any unsaved changes, and saves the data in
     * 'configMap'.
     *
     * @return True if successful.
     * @see #save()
     */
    public boolean reload()
    {
        // Make sure a file was actually specified
        if( TextUtils.isEmpty( mFilename ) )
            return false;

        // Free any previously loaded data
        clear();

        FileInputStream fstream;
        try
        {
            fstream = new FileInputStream( mFilename );
        }
        catch( FileNotFoundException fnfe )
        {
            // File not found... we can't continue
            return false;
        }

        DataInputStream in = new DataInputStream( fstream );
        BufferedReader br = new BufferedReader( new InputStreamReader( in ) );

        ConfigSection section = new ConfigSection( "", br );                                                                       // section
        // Loop through reading the remaining sections
        while( !TextUtils.isEmpty( section.nextName ) )
        {
            // Get the next section name
            String sectionName = section.nextName;

            // Load the next section
            section = new ConfigSection( sectionName, br );
            mConfigMap.put( sectionName, section ); // Save the data to 'configMap'
        }

        try
        {
            // Finished. Close the file.
            in.close();
            br.close();
        }
        catch( IOException ioe )
        {
            // (Don't care)
        }

        // Success
        return true;
    }

    /**
     * Saves the data from 'configMap' back to the config file.
     *
     * @return True if successful. False otherwise.
     * @see #reload()
     */
    public boolean save()
    {
        // No filename was specified.
        if( TextUtils.isEmpty( mFilename ) )
        {
            Log.e( "ConfigFile", "Filename not specified in method save()" );
            return false; // Quit
        }

        // Ensure parent directories exist before writing file
        new File( mFilename ).getParentFile().mkdirs();

        // Write data to file
        FileWriter fw = null;
        try
        {
            fw = new FileWriter( mFilename );

            // Loop through the sections
            for( ConfigSection section : mConfigMap.values() )
            {
                if( section != null )
                    section.save( fw );
            }
        }
        catch( IOException ioe )
        {
            Log.e( "ConfigFile", "IOException creating file " + mFilename + ", error message: "
                    + ioe.getMessage() );
            return false; // Some problem creating the file.. quit
        }
        finally
        {
            if( fw != null )
            {
                try
                {
                    fw.close();
                }
                catch( IOException ignored )
                {
                }
            }
        }

        // Success
        return true;
    }

    /**
     * Returns a handle to the configMap keyset.
     *
     * @return keyset containing all the config section titles.
     */
    public Set<String> keySet()
    {
        return mConfigMap.keySet();
    }

    /**
     * The ConfigSection class reads all the parameters in the next section of the config file.
     * Saves the name of the next section (or null if end of file or error). Can also be used to add
     * a new section to an existing configuration.
     */
    public static class ConfigSection
    {
        public String name; // Section name
        private HashMap<String, ConfigParameter> parameters; // Parameters sorted by name for easy
                                                             // lookup
        private LinkedList<ConfigLine> lines; // All the lines in this section, including comments

        // Name of the next section, or null if there are no sections left to read in the file:
        private String nextName = null;

        /**
         * Constructor: Creates an empty config section
         *
         * @param sectionName The section title.
         */
        public ConfigSection( String sectionName )
        {
            parameters = new HashMap<String, ConfigParameter>();
            lines = new LinkedList<ConfigLine>();

            if( !TextUtils.isEmpty( sectionName ))
            {
                lines.add( new ConfigLine( ConfigLine.LINE_SECTION, "[" + sectionName + "]\n", null ) );
            }
            name = sectionName;
        }

        /**
         * Constructor: Reads the next section of the config file, and saves it in 'parameters'.
         *
         * @param sectionName The section title.
         * @param br The config file to read from.
         */
        public ConfigSection( String sectionName, BufferedReader br )
        {
            String fullLine, strLine, p, v;
            ConfigParameter confParam;
            int x, y;

            parameters = new HashMap<String, ConfigParameter>();
            lines = new LinkedList<ConfigLine>();

            if( !TextUtils.isEmpty( sectionName ))
            {
                lines.add( new ConfigLine( ConfigLine.LINE_SECTION, "[" + sectionName + "]\n", null ) );
            }
            name = sectionName;

            // No file to read from. Quit.
            if( br == null )
                return;

            try
            {
                while( ( fullLine = br.readLine() ) != null )
                {
                    strLine = fullLine.trim();
                    if( ( strLine.length() < 1 )
                            || ( strLine.substring( 0, 1 ).equals( "#" ) )
                            || ( strLine.substring( 0, 1 ).equals( ";" ) )
                            || ( ( strLine.length() > 1 ) && ( strLine.substring( 0, 2 )
                                    .equals( "//" ) ) ) )

                    { // A comment or blank line.
                        lines.add( new ConfigLine( ConfigLine.LINE_GARBAGE, fullLine + "\n", null ) );
                    }
                    else if( strLine.contains( "=" ) )
                    {
                        // This should be a "parameter=value" pair:
                        x = strLine.indexOf( '=' );

                        if( x < 1 )
                            return; // This shouldn't happen (bad syntax). Quit.

                        if( x < ( strLine.length() - 1 ) )
                        {
                            p = strLine.substring( 0, x ).trim();
                            if( p.length() < 1 )
                                return; // This shouldn't happen (bad syntax). Quit.

                            v = strLine.substring( x + 1, strLine.length() ).trim();
                            // v = v.replace( "\"", "" ); // I'm doing this later, so I can save
                            // back without losing them

                            if( v.length() > 0 )
                            {
                                // Save the parameter=value pair
                                confParam = parameters.get( p );
                                if( confParam != null )
                                {
                                    confParam.value = v;
                                }
                                else
                                {
                                    confParam = new ConfigParameter( p, v );
                                    lines.add( new ConfigLine( ConfigLine.LINE_PARAM, fullLine
                                            + "\n", confParam ) );
                                    parameters.put( p, confParam ); // Save the pair.
                                }
                            }
                        } // It's ok to have an empty assignment (such as "param=")
                    }
                    else if( strLine.contains( "[" ) )
                    {
                        // This should be the beginning of the next section
                        if( ( strLine.length() < 3 ) || ( !strLine.contains( "]" ) ) )
                            return; // This shouldn't happen (bad syntax). Quit.

                        x = strLine.indexOf( '[' );
                        y = strLine.indexOf( ']' );

                        if( ( y <= x + 1 ) || ( x == -1 ) || ( y == -1 ) )
                            return; // This shouldn't happen (bad syntax). Quit.

                        p = strLine.substring( x + 1, y ).trim();

                        // Save the name of the next section.
                        nextName = p;

                        // Done reading parameters. Return.
                        return;
                    }
                    else
                    {
                        // This shouldn't happen (bad syntax). Quit.
                        return;
                    }
                }
            }
            catch( IOException ioe )
            {
                // (Don't care)
            }

            // Reached end of file or error.. either way, just quit
            return;
        }

        /**
         * Returns a handle to the parameter keyset.
         *
         * @return keyset containing all the parameters.
         */
        public Set<String> keySet()
        {
            return parameters.keySet();
        }

        /**
         * Returns the value of the specified parameter.
         *
         * @param parameter Name of the parameter.
         *
         * @return Parameter's value, or null if not found.
         */
        public String get( String parameter )
        {
            // Error: no parameters, or parameter was null
            if( parameters == null || TextUtils.isEmpty( parameter ) )
                return null;

            ConfigParameter confParam = parameters.get( parameter );

            // Parameter not found
            if( confParam == null )
                return null;

            // Got it
            return confParam.value;
        }

        /**
         * Adds the specified parameter to this config section, updates the value if it already
         * exists, or removes the parameter.
         *
         * @param parameter The name of the parameter.
         * @param value The parameter's value, or null to remove.
         */
        public void put( String parameter, String value )
        {
            ConfigParameter confParam = parameters.get( parameter );
            if( confParam == null ) // New parameter
            {
                if( !TextUtils.isEmpty( value ) )
                {
                    confParam = new ConfigParameter( parameter, value );
                    lines.add( new ConfigLine( ConfigLine.LINE_PARAM, parameter + "=" + value
                            + "\n", confParam ) );
                    parameters.put( parameter, confParam );
                }
            }
            else
            {
                // Change the parameter's value
                confParam.value = value;
            }
        }

        /**
         * Writes the entire section to file.
         *
         * @param fw File to write to.
         *
         * @throws IOException if a writing error occurs.
         */
        public void save( FileWriter fw ) throws IOException
        {
            for( ConfigLine line : lines )
            {
                if( line != null )
                    line.save( fw );
            }
        }
    }

    /**
     * The ConfigLine class stores each line of the config file (including comments).
     */
    private static class ConfigLine
    {
        public static final int LINE_GARBAGE = 0; // Comment, whitespace, or blank line
        public static final int LINE_SECTION = 1; // Section title
        public static final int LINE_PARAM = 2; // Parameter=value pair

        public int lineType = 0; // LINE_GARBAGE, LINE_SECTION, or LINE_PARAM.
        public String strLine = ""; // Actual line from the config file.
        public ConfigParameter confParam = null; // Null unless this line has a parameter.

        /**
         * Constructor: Saves the relevant information about the line.
         *
         * @param type The type of line.
         * @param line The line itself.
         * @param param Config parameters pertaining to the line.
         */
        public ConfigLine( int type, String line, ConfigParameter param )
        {
            lineType = type;
            strLine = line;
            confParam = param;
        }

        /**
         * Saves the ConfigLine.
         *
         * @param fw The file to save the ConfigLine to.
         *
         * @throws IOException If a writing error occurs.
         */
        public void save( FileWriter fw ) throws IOException
        {
            int x;
            if( lineType == LINE_PARAM )
            {
                if( !strLine.contains( "=" ) || confParam == null )
                    return; // This shouldn't happen

                x = strLine.indexOf( '=' );

                if( x < 1 )
                    return; // This shouldn't happen either

                if( x < strLine.length() )
                    fw.write( strLine.substring( 0, x + 1 ) + confParam.value + "\n" );
            }
            else
            {
                fw.write( strLine );
            }
        }
    }

    /**
     * The ConfigParameter class associates a parameter with its value.
     */
    private static class ConfigParameter
    {
        @SuppressWarnings( "unused" )
        public String parameter;
        public String value;

        /**
         * Constructor: Associate the parameter and value
         *
         * @param parameter The name of the parameter.
         * @param value The value of the parameter.
         */
        public ConfigParameter( String parameter, String value )
        {
            this.parameter = parameter;
            this.value = value;
        }
    }
}
