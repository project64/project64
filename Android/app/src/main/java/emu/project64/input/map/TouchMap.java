package emu.project64.input.map;

import java.util.ArrayList;
import java.util.HashMap;
import emu.project64.input.AbstractController;
import emu.project64.input.TouchController;
import emu.project64.persistent.ConfigFile;
import emu.project64.persistent.ConfigFile.ConfigSection;
import emu.project64.profile.Profile;
import emu.project64.util.Image;
import emu.project64.util.Utility;
import android.annotation.SuppressLint;
import android.content.res.Resources;
import android.graphics.Point;
import android.graphics.Rect;
import android.util.Log;
import android.util.SparseArray;

/**
 * A class for mapping digitizer coordinates to N64 buttons/axes.
 * 
 * @see VisibleTouchMap
 * @see TouchController
 */
@SuppressLint("FloatMath")
public class TouchMap
{
    /** Map flag: Touch location is not mapped. */
    public static final int UNMAPPED = -1;
    
    /** Map offset: N64 pseudo-buttons. */
    public static final int OFFSET_EXTRAS = AbstractController.NUM_N64_BUTTONS;
    
    /** N64 pseudo-button: dpad-right-up. */
    public static final int DPD_RU = OFFSET_EXTRAS;
    
    /** N64 pseudo-button: dpad-right-down. */
    public static final int DPD_RD = OFFSET_EXTRAS + 1;
    
    /** N64 pseudo-button: dpad-left-down. */
    public static final int DPD_LD = OFFSET_EXTRAS + 2;
    
    /** N64 pseudo-button: dpad-left-up. */
    public static final int DPD_LU = OFFSET_EXTRAS + 3;
    
    /** Total number of N64 (pseudo-)buttons. */
    public static final int NUM_N64_PSEUDOBUTTONS = OFFSET_EXTRAS + 4;
    
    /** Folder containing the images. */
    protected String skinFolder;
    
    /** Scaling factor to apply to images. */
    protected float scale = 1.0f;
    
    /** Button images. */
    protected ArrayList<Image> buttonImages;
    
    /** Button masks. */
    private final ArrayList<Image> buttonMasks;
    
    /** X-coordinates of the buttons, in percent. */
    private final ArrayList<Integer> buttonX;
    
    /** Y-coordinates of the buttons, in percent. */
    private final ArrayList<Integer> buttonY;
    
    /** names of the buttons. */
    private final ArrayList<String> buttonNames;
    
    /** Analog background image (fixed). */
    protected Image analogBackImage;
    
    /** Analog foreground image (movable). */
    protected Image analogForeImage;
    
    /** X-coordinate of the analog background, in percent. */
    private int analogBackX;
    
    /** Y-coordinate of the analog background, in percent. */
    private int analogBackY;
    
    /** Deadzone of the analog stick, in pixels. */
    private int analogDeadzone;
    
    /** Maximum displacement of the analog stick, in pixels. */
    protected int analogMaximum;
    
    /** Extra region beyond maximum in which the analog stick can be captured, in pixels. */
    private int analogPadding;
    
    /** The resources of the associated activity. */
    protected final Resources mResources;
    
    /** Map from N64 (pseudo-)button to mask color. */
    private final int[] mN64ToColor;
    
    /** The map from strings in the skin.ini file to N64 button indices. */
    public static final HashMap<String, Integer> MASK_KEYS = new HashMap<String, Integer>();
    
    /** The map from N64 button indices to asset name prefixes in the skin folder. */
    public static final SparseArray<String> ASSET_NAMES = new SparseArray<String>();
    
    /** The error in RGB (256x256x256) space that we tolerate when matching mask colors. */
    private static final int MATCH_TOLERANCE = 10;
    
    static
    {
        // Define the map from skin.ini keys to N64 button indices
        // @formatter:off
        MASK_KEYS.put( "Dr",  AbstractController.DPD_R );
        MASK_KEYS.put( "Dl",  AbstractController.DPD_L );
        MASK_KEYS.put( "Dd",  AbstractController.DPD_D );
        MASK_KEYS.put( "Du",  AbstractController.DPD_U );
        MASK_KEYS.put( "S",   AbstractController.START );
        MASK_KEYS.put( "Z",   AbstractController.BTN_Z );
        MASK_KEYS.put( "B",   AbstractController.BTN_B );
        MASK_KEYS.put( "A",   AbstractController.BTN_A );
        MASK_KEYS.put( "Cr",  AbstractController.CPD_R );
        MASK_KEYS.put( "Cl",  AbstractController.CPD_L );
        MASK_KEYS.put( "Cd",  AbstractController.CPD_D );
        MASK_KEYS.put( "Cu",  AbstractController.CPD_U );
        MASK_KEYS.put( "R",   AbstractController.BTN_R );
        MASK_KEYS.put( "L",   AbstractController.BTN_L );
        MASK_KEYS.put( "Dru", DPD_RU );
        MASK_KEYS.put( "Drd", DPD_RD );
        MASK_KEYS.put( "Dld", DPD_LD );
        MASK_KEYS.put( "Dlu", DPD_LU );
        // @formatter:on
        
        // Define the map from N64 button indices to profile key prefixes
        ASSET_NAMES.put( AbstractController.DPD_R, "dpad" );
        ASSET_NAMES.put( AbstractController.DPD_L, "dpad" );
        ASSET_NAMES.put( AbstractController.DPD_D, "dpad" );
        ASSET_NAMES.put( AbstractController.DPD_U, "dpad" );
        ASSET_NAMES.put( AbstractController.START, "buttonS" );
        ASSET_NAMES.put( AbstractController.BTN_Z, "buttonZ" );
        ASSET_NAMES.put( AbstractController.BTN_B, "groupAB" );
        ASSET_NAMES.put( AbstractController.BTN_A, "groupAB" );
        ASSET_NAMES.put( AbstractController.CPD_R, "groupC" );
        ASSET_NAMES.put( AbstractController.CPD_L, "groupC" );
        ASSET_NAMES.put( AbstractController.CPD_D, "groupC" );
        ASSET_NAMES.put( AbstractController.CPD_U, "groupC" );
        ASSET_NAMES.put( AbstractController.BTN_R, "buttonR" );
        ASSET_NAMES.put( AbstractController.BTN_L, "buttonL" );
        ASSET_NAMES.put( DPD_LU, "dpad" );
        ASSET_NAMES.put( DPD_LD, "dpad" );
        ASSET_NAMES.put( DPD_RD, "dpad" );
        ASSET_NAMES.put( DPD_RU, "dpad" );
    }
    
    /**
     * Instantiates a new touch map.
     * 
     * @param resources The resources of the activity associated with this touch map.
     */
    public TouchMap( Resources resources )
    {
        mResources = resources;
        mN64ToColor = new int[NUM_N64_PSEUDOBUTTONS];
        buttonImages = new ArrayList<Image>();
        buttonMasks = new ArrayList<Image>();
        buttonX = new ArrayList<Integer>();
        buttonY = new ArrayList<Integer>();
        buttonNames = new ArrayList<String>();
    }
    
    /**
     * Clears the map data.
     */
    public void clear()
    {
        buttonImages.clear();
        buttonMasks.clear();
        buttonX.clear();
        buttonY.clear();
        buttonNames.clear();
        analogBackImage = null;
        analogForeImage = null;
        analogBackX = analogBackY = 0;
        analogPadding = 32;
        analogDeadzone = 2;
        analogMaximum = 360;
        for( int i = 0; i < mN64ToColor.length; i++ )
            mN64ToColor[i] = -1;
    }
    
    /**
     * Recomputes the map data for a given digitizer size.
     * 
     * @param w The width of the digitizer, in pixels.
     * @param h The height of the digitizer, in pixels.
     */
    public void resize( int w, int h )
    {
        // Recompute button locations
        for( int i = 0; i < buttonImages.size(); i++ )
        {
            buttonImages.get( i ).setScale( scale );
            buttonImages.get( i ).fitPercent( buttonX.get( i ), buttonY.get( i ), w, h );
            buttonMasks.get( i ).setScale( scale );
            buttonMasks.get( i ).fitPercent( buttonX.get( i ), buttonY.get( i ), w, h );
        }
        
        // Recompute analog background location
        if( analogBackImage != null )
        {
            analogBackImage.setScale(  scale );
            analogBackImage.fitPercent( analogBackX, analogBackY, w, h );
        }
    }
    
    /**
     * Gets the N64 button mapped to a given touch location.
     * 
     * @param xLocation The x-coordinate of the touch, in pixels.
     * @param yLocation The y-coordinate of the touch, in pixels.
     * 
     * @return The N64 button the location is mapped to, or UNMAPPED.
     * 
     * @see TouchMap#UNMAPPED
     */
    public int getButtonPress( int xLocation, int yLocation )
    {
        // Search through every button mask to see if the corresponding button was touched
        for( Image mask : buttonMasks )
        {
            if( mask != null )
            {
                int left = mask.x;
                int right = left + (int) ( mask.width * mask.scale );
                int bottom = mask.y;
                int top = bottom + (int) ( mask.height * mask.scale );
                
                // See if the touch falls in the vicinity of the button (conservative test)
                if( xLocation >= left && xLocation < right && yLocation >= bottom
                        && yLocation < top )
                {
                    // Get the mask color at this location
                    int c = mask.image.getPixel( (int) ( ( xLocation - mask.x ) / scale ), (int) ( ( yLocation - mask.y ) / scale ) );
                    
                    // Ignore the alpha component if any
                    int rgb = c & 0x00ffffff;
                    
                    // Ignore black and get the N64 button associated with this color
                    if( rgb > 0 )
                        return getButtonFromColor( rgb );
                }
            }
        }
        return UNMAPPED;
    }
    
    /**
     * Gets the frame for the N64 button with a given asset name
     * 
     * @param assetName The asset name for the button
     * 
     * @return The frame for the N64 button with the given asset name
     * 
     */
    public Rect getButtonFrame( String assetName )
    {
        for( int i = 0; i < buttonNames.size(); i++ )
        {
            if ( buttonNames.get( i ).equals( assetName ) )
                return new Rect( buttonMasks.get( i ).drawRect );
        }
        return new Rect(0, 0, 0, 0);
    }
    
    /**
     * Gets the N64 button mapped to a given mask color.
     * 
     * @param color The mask color.
     * 
     * @return The N64 button the color is mapped to, or UNMAPPED.
     */
    private int getButtonFromColor( int color )
    {
        // Find the N64 button whose mask matches the given color. Because we scale the mask images,
        // the mask boundaries can get softened. Therefore we tolerate a bit of error in the match.
        int closestMatch = UNMAPPED;
        int matchDif = MATCH_TOLERANCE * MATCH_TOLERANCE;
        
        // Get the RGB values of the given color
        int r = ( color & 0xFF0000 ) >> 16;
        int g = ( color & 0x00FF00 ) >> 8;
        int b = ( color & 0x0000FF );
        
        // Find the mask color with the smallest squared error
        for( int i = 0; i < mN64ToColor.length; i++ )
        {
            int color2 = mN64ToColor[i];
            
            // Compute squared error in RGB space
            int difR = r - ( ( color2 & 0xFF0000 ) >> 16 );
            int difG = g - ( ( color2 & 0x00FF00 ) >> 8 );
            int difB = b - ( ( color2 & 0x0000FF ) );
            int dif = difR * difR + difG * difG + difB * difB;
            
            if( dif < matchDif )
            {
                closestMatch = i;
                matchDif = dif;
            }
        }
        return closestMatch;
    }
    
    /**
     * Gets the N64 analog stick displacement.
     * 
     * @param xLocation The x-coordinate of the touch, in pixels.
     * @param yLocation The y-coordinate of the touch, in pixels.
     * 
     * @return The analog displacement, in pixels.
     */
    public Point getAnalogDisplacement( int xLocation, int yLocation )
    {
        if( analogBackImage == null )
            return new Point( 0, 0 );
        
        // Distance from center along x-axis
        int dX = xLocation - ( analogBackImage.x + (int) ( analogBackImage.hWidth * scale ) );
        
        // Distance from center along y-axis
        int dY = yLocation - ( analogBackImage.y + (int) ( analogBackImage.hHeight * scale ) );
        
        return new Point( dX, dY );
    }
    
    /**
     * Gets the N64 analog stick's frame.
     * 
     * @return The analog stick's frame.
     */
    public Rect getAnalogFrame()
    {
        if( analogBackImage != null )
            return new Rect( analogBackImage.drawRect );
        return new Rect(0, 0, 0, 0);
    }
    
    /**
     * Gets the N64 analog stick displacement, constrained to an octagon.
     * 
     * @param dX The x-displacement of the stick, in pixels.
     * @param dY The y-displacement of the stick, in pixels.
     * 
     * @return The constrained analog displacement, in pixels.
     */
    public Point getConstrainedDisplacement( int dX, int dY )
    {
        final float dC = (int) ( analogMaximum * scale );
        final float dA = dC * (float)Math.sqrt( 0.5f );
        final float signX = (dX < 0) ? -1 : 1;
        final float signY = (dY < 0) ? -1 : 1;
        
        Point crossPt = new Point();
        crossPt.x = dX;
        crossPt.y = dY;
        
        if( ( signX * dX ) > ( signY * dY ) )
            segsCross( 0, 0, dX, dY, signX * dC, 0, signX * dA, signY * dA, crossPt );
        else
            segsCross( 0, 0, dX, dY, 0, signY * dC, signX * dA, signY * dA, crossPt );
        
        return crossPt;
    }
    
    /**
     * Gets the analog strength, accounting for deadzone and motion limits.
     * 
     * @param displacement The Pythagorean displacement of the analog stick, in pixels.
     * 
     * @return The analog strength, between 0 and 1, inclusive.
     */
    public float getAnalogStrength( float displacement )
    {
        /*displacement /= scale;
        float p = ( displacement - analogDeadzone ) / ( analogMaximum - analogDeadzone );
        return Utility.clamp( p, 0.0f, 1.0f );*/
        return 0.0f;
    }
    
    /**
     * Checks if a touch is within capture range of the analog stick.
     * 
     * @param displacement The displacement of the touch with respect to analog center, in pixels.
     * 
     * @return True, if the touch is in capture range of the stick.
     */
    public boolean isInCaptureRange( float displacement )
    {
        displacement /= scale;
        return ( displacement >= analogDeadzone ) && ( displacement < analogMaximum + analogPadding );
    }
    
    /**
     * Loads all touch map data from the filesystem.
     * 
     * @param skinDir The directory containing the skin.ini and image files.
     * @param profile  The name of the layout profile.
     * @param animated True to load the analog assets in two parts for animation.
     */
    public void load( String skinDir, Profile profile, boolean animated )
    {
        // Clear any old assets and map data
        clear();
        
        // Load the configuration files
        skinFolder = skinDir;
        ConfigFile skin_ini = new ConfigFile( skinFolder + "/skin.ini" );
        
        // Look up the mask colors
        loadMaskColors( skin_ini );
        
        // Loop through all the configuration sections
        loadAllAssets( profile, animated );
    }
    
    /**
     * Loads the mask colors from a configuration file.
     * 
     * @param skin_ini The configuration file containing mask info.
     */
    private void loadMaskColors( ConfigFile skin_ini )
    {
        ConfigSection section = skin_ini.get( "MASK_COLOR" );
        if( section != null )
        {
            // Loop through the key-value pairs
            for( String key : section.keySet() )
            {
                // Assign the map colors to the appropriate N64 button
                String val = section.get( key );
                Integer index = MASK_KEYS.get( key );
                if( index != null )
                {
                    try
                    {
                        mN64ToColor[index] = Integer.parseInt( val, 16 );
                    }
                    catch( NumberFormatException ex )
                    {
                        mN64ToColor[index] = -1;
                        Log.w( "TouchMap", "Invalid mask color '" + val + "' in " + skinFolder + "/skin.ini" );
                    }
                }
            }
        }
    }
    
    /**
     * Loads all assets and properties specified in a profile.
     * 
     * @param profile  The touchscreen/touchpad profile.
     * @param animated True to load the analog assets in two parts for animation.
     */
    protected void loadAllAssets( Profile profile, boolean animated )
    {
        if( profile != null )
        {
            loadAnalog( profile, animated );
            loadButton( profile, "dpad" );
            loadButton( profile, "groupAB" );
            loadButton( profile, "groupC" );
            loadButton( profile, "buttonL" );
            loadButton( profile, "buttonR" );
            loadButton( profile, "buttonZ" );
            loadButton( profile, "buttonS" );
        }
    }
    
    /**
     * Loads analog assets and properties from the filesystem.
     * 
     * @param profile  The touchscreen/touchpad profile containing the analog properties.
     * @param animated True to load the assets in two parts for animation.
     */
    protected void loadAnalog( Profile profile, boolean animated )
    {
        int x = profile.getInt( "analog-x", -1 );
        int y = profile.getInt( "analog-y", -1 );
        
        if( x >= 0 && y >= 0 )
        {
            // Position (percentages of the digitizer dimensions)
            analogBackX = x;
            analogBackY = y;
            
            // The images (used by touchscreens) are in PNG image format.
            if( animated )
            {
                 analogBackImage = new Image( mResources, skinFolder + "/analog-back.png" );
                 analogForeImage = new Image( mResources, skinFolder + "/analog-fore.png" );
            }
            else
            {
                analogBackImage = new Image( mResources, skinFolder + "/analog.png" );
            }
            
            // Sensitivity (percentages of the radius, i.e. half the image width)
            analogDeadzone = (int) ( analogBackImage.hWidth * ( profile.getFloat( "analog-min", 1 ) / 100.0f ) );
            analogMaximum = (int) ( analogBackImage.hWidth * ( profile.getFloat( "analog-max", 55 ) / 100.0f ) );
            analogPadding = (int) ( analogBackImage.hWidth * ( profile.getFloat( "analog-buff", 55 ) / 100.0f ) );
        }
    }
    
    /**
     * Loads button assets and properties from the filesystem.
     * 
     * @param profile The touchscreen/touchpad profile containing the button properties.
     * @param name    The name of the button/group to load.
     */
    protected void loadButton( Profile profile, String name )
    {
        int x = profile.getInt( name + "-x", -1 );
        int y = profile.getInt( name + "-y", -1 );
        
        if( x >= 0 && y >= 0 )
        {
            // Position (percentages of the digitizer dimensions)
            buttonX.add( x );
            buttonY.add( y );
            buttonNames.add( name );
            
            // Load the displayed and mask images
            buttonImages.add( new Image( mResources, skinFolder + "/" + name + ".png" ) );
            buttonMasks.add( new Image( mResources, skinFolder + "/" + name + "-mask.png" ) );
        }
    }

    /**
     * Determines if the two specified line segments intersect with each other, and calculates where
     * the intersection occurs if they do.
     * 
     * @param seg1pt1_x X-coordinate for the first end of the first line segment.
     * @param seg1pt1_y Y-coordinate for the first end of the first line segment.
     * @param seg1pt2_x X-coordinate for the second end of the first line segment.
     * @param seg1pt2_y Y-coordinate for the second end of the first line segment.
     * @param seg2pt1_x X-coordinate for the first end of the second line segment.
     * @param seg2pt1_y Y-coordinate for the first end of the second line segment.
     * @param seg2pt2_x X-coordinate for the second end of the second line segment.
     * @param seg2pt2_y Y-coordinate for the second end of the second line segment.
     * @param crossPt Changed to the point of intersection if there is one, otherwise unchanged.
     * 
     * @return True if the two line segments intersect.
     */
    private static boolean segsCross( float seg1pt1_x, float seg1pt1_y, float seg1pt2_x,
            float seg1pt2_y, float seg2pt1_x, float seg2pt1_y, float seg2pt2_x, float seg2pt2_y,
            Point crossPt )
    {
        float vec1_x = seg1pt2_x - seg1pt1_x;
        float vec1_y = seg1pt2_y - seg1pt1_y;
        
        float vec2_x = seg2pt2_x - seg2pt1_x;
        float vec2_y = seg2pt2_y - seg2pt1_y;
        
        float div = ( -vec2_x * vec1_y + vec1_x * vec2_y );
        
        // Segments don't cross
        if( div == 0 )
            return false;
        
        float s = ( -vec1_y * ( seg1pt1_x - seg2pt1_x ) + vec1_x * ( seg1pt1_y - seg2pt1_y ) ) / div;
        float t = ( vec2_x  * ( seg1pt1_y - seg2pt1_y ) - vec2_y * ( seg1pt1_x - seg2pt1_x ) ) / div;
        
        if( s >= 0 && s < 1 && t >= 0 && t <= 1 )
        {
            // Segments cross, point of intersection stored in 'crossPt'
            crossPt.x = (int) ( seg1pt1_x + ( t * vec1_x ) );
            crossPt.y = (int) ( seg1pt1_y + ( t * vec1_y ) );
            return true;
        }
        
        // Segments don't cross
        return false;
    }
}
