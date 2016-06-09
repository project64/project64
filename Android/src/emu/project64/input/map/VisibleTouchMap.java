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
package emu.project64.input.map;

import java.util.concurrent.CopyOnWriteArrayList;

import emu.project64.game.GameOverlay;
import emu.project64.persistent.ConfigFile;
import emu.project64.profile.Profile;
import emu.project64.util.Image;
import emu.project64.util.SafeMethods;
import emu.project64.util.Utility;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.util.DisplayMetrics;
import android.util.Log;

/**
 * A kind of touch map that can be drawn on a canvas.
 * 
 * @see TouchMap
 * @see GameOverlay
 */
public class VisibleTouchMap extends TouchMap
{
    /** FPS frame image. */
    private Image mFpsFrame;
    
    /** X-coordinate of the FPS frame, in percent. */
    private int mFpsFrameX;
    
    /** Y-coordinate of the FPS frame, in percent. */
    private int mFpsFrameY;
    
    /** X-coordinate of the FPS text centroid, in percent. */
    private int mFpsTextX;
    
    /** Y-coordinate of the FPS text centroid, in percent. */
    private int mFpsTextY;
    
    /** The current FPS value. */
    private int mFpsValue;
    
    /** The minimum size of the FPS indicator in pixels. */
    private float mFpsMinPixels;
    
    /** The minimum size to scale the FPS indicator. */
    private float mFpsMinScale;
    
    /** True if the FPS indicator should be drawn. */
    private boolean mFpsEnabled;
    
    /** The factor to scale images by. */
    private float mScalingFactor = 1.0f;
    
    /** Touchscreen opacity. */
    private int mTouchscreenTransparency;
    
    /** Reference screen width in pixels (if provided in skin.ini). */
    private int mReferenceWidth = 0;
    
    /** Reference screen height in pixels (if provided in skin.ini). */
    private int mReferenceHeight = 0;
    
    /** The last width passed to {@link #resize(int, int, DisplayMetrics)}. */
    private int cacheWidth = 0;
    
    /** The last height passed to {@link #resize(int, int, DisplayMetrics)}. */
    private int cacheHeight = 0;
    
    /** The last height passed to {@link #resize(int, int, DisplayMetrics)}. */
    private DisplayMetrics cacheMetrics;
    
    /** The set of images representing the FPS string. */
    private final CopyOnWriteArrayList<Image> mFpsDigits;
    
    /** The set of images representing the numerals 0, 1, 2, ..., 9. */
    private final Image[] mNumerals;
    
    /** Auto-hold overlay images. */
    public final Image[] autoHoldImages;
    
    /** X-coordinates of the AutoHold mask, in percent. */
    private final int[] autoHoldX;
    
    /** Y-coordinates of the AutoHold mask, in percent. */
    private final int[] autoHoldY;
    
    /**
     * Instantiates a new visible touch map.
     * 
     * @param resources  The resources of the activity associated with this touch map.
     */
    public VisibleTouchMap( Resources resources )
    {
        super( resources );
        mFpsDigits = new CopyOnWriteArrayList<Image>();
        mNumerals = new Image[10];
        autoHoldImages = new Image[NUM_N64_PSEUDOBUTTONS];
        autoHoldX = new int[NUM_N64_PSEUDOBUTTONS];
        autoHoldY = new int[NUM_N64_PSEUDOBUTTONS];
    }
    
    /*
     * (non-Javadoc)
     * 
     * @see emu.project64.input.map.TouchMap#clear()
     */
    @Override
    public void clear()
    {
        super.clear();
        mFpsFrame = null;
        mFpsFrameX = mFpsFrameY = 0;
        mFpsTextX = mFpsTextY = 50;
        mFpsValue = 0;
        mFpsDigits.clear();
        for( int i = 0; i < mNumerals.length; i++ )
            mNumerals[i] = null;
        for( int i = 0; i < autoHoldImages.length; i++ )
            autoHoldImages[i] = null;
        for( int i = 0; i < autoHoldX.length; i++ )
            autoHoldX[i] = 0;
        for( int i = 0; i < autoHoldY.length; i++ )
            autoHoldY[i] = 0;
    }
    
    /**
     * Recomputes the map data for a given digitizer size, and
     * recalculates the scaling factor.
     * 
     * @param w The width of the digitizer, in pixels.
     * @param h The height of the digitizer, in pixels.
     * @param metrics Metrics about the display (for use in scaling).
     */
    public void resize( int w, int h, DisplayMetrics metrics )
    {
        // Cache the width and height in case we need to reload assets
        cacheWidth = w;
        cacheHeight = h;
        cacheMetrics = metrics;
        scale = 1.0f;
        
        if( metrics != null )
        {
            // Scale buttons to match the skin designer's proportions
            float scaleW = 1f;
            float scaleH = 1f;
            if( mReferenceWidth > 0 )
                scaleW = Math.max( metrics.widthPixels, metrics.heightPixels ) / (float) mReferenceWidth;
            if( mReferenceHeight > 0 )
                scaleH = Math.min( metrics.widthPixels, metrics.heightPixels ) / (float) mReferenceHeight;
            scale = Math.min( scaleW, scaleH );
        }
        // Apply the global scaling factor (derived from user prefs)
        scale *= mScalingFactor;
        
        resize( w, h );
    }
    
    /*
     * (non-Javadoc)
     * 
     * @see emu.project64.input.map.TouchMap#resize(int, int)
     */
    @Override
    public void resize( int w, int h )
    {
        super.resize( w, h );
        
        // Compute analog foreground location (centered)
        if( analogBackImage != null && analogForeImage != null )
        {
            int cX = analogBackImage.x + (int) ( analogBackImage.hWidth * scale );
            int cY = analogBackImage.y + (int) ( analogBackImage.hHeight * scale );
            analogForeImage.setScale( scale );
            analogForeImage.fitCenter( cX, cY, analogBackImage.x, analogBackImage.y,
                    (int) ( analogBackImage.width * scale ), (int) ( analogBackImage.height * scale ) );
        }
        
        // Compute auto-hold overlay locations
        for( int i = 0; i < autoHoldImages.length; i++ )
        {
            if( autoHoldImages[i] != null )
            {
                autoHoldImages[i].setScale( scale );
                autoHoldImages[i].fitPercent( autoHoldX[i], autoHoldY[i], w, h );
            }
        }
        
        // Compute FPS frame location
        float fpsScale = scale;
        if( mFpsMinScale > scale )
            fpsScale = mFpsMinScale;
        if( mFpsFrame != null )
        {
            mFpsFrame.setScale( fpsScale );
            mFpsFrame.fitPercent( mFpsFrameX, mFpsFrameY, w, h );
        }
        for( int i = 0; i < mNumerals.length; i++ )
        {
            if( mNumerals[i] != null )
                mNumerals[i].setScale( fpsScale );
        }
        
        // Compute the FPS digit locations
        refreshFpsImages();
        refreshFpsPositions();
    }
    
    /**
     * Draws the buttons.
     * 
     * @param canvas The canvas on which to draw.
     */
    public void drawButtons( Canvas canvas )
    {
        // Draw the buttons onto the canvas
        for( Image button : buttonImages )
        {
            button.draw( canvas );
        }
    }
    
    /**
     * Draws the AutoHold mask.
     * 
     * @param canvas The canvas on which to draw.
     */
    public void drawAutoHold( Canvas canvas )
    {
        // Draw the AutoHold mask onto the canvas
        for( Image autoHoldImage : autoHoldImages )
        {
            if( autoHoldImage != null )
            {
                autoHoldImage.draw( canvas );
            }
        }
    }
    
    /**
     * Draws the analog stick.
     * 
     * @param canvas The canvas on which to draw.
     */
    public void drawAnalog( Canvas canvas )
    {
        // Draw the background image
        if( analogBackImage != null )
        {
            analogBackImage.draw( canvas );
        }
        
        // Draw the movable foreground (the stick)
        if( analogForeImage != null )
        {
            analogForeImage.draw( canvas );
        }
    }
    
    /**
     * Draws the FPS indicator.
     * 
     * @param canvas The canvas on which to draw.
     */
    public void drawFps( Canvas canvas )
    {
        if( canvas == null )
            return;
        
        // Redraw the FPS indicator
        if( mFpsFrame != null )
            mFpsFrame.draw( canvas );
        
        // Draw each digit of the FPS number
        for( Image digit : mFpsDigits )
            digit.draw( canvas );
    }
    
    /**
     * Updates the analog stick assets to reflect a new position.
     * 
     * @param axisFractionX The x-axis fraction, between -1 and 1, inclusive.
     * @param axisFractionY The y-axis fraction, between -1 and 1, inclusive.
     * 
     * @return True if the analog assets changed.
     */
    public boolean updateAnalog( float axisFractionX, float axisFractionY )
    {
        if( analogForeImage != null && analogBackImage != null )
        {
            // Get the location of stick center
            int hX = (int) ( ( analogBackImage.hWidth + ( axisFractionX * analogMaximum ) ) * scale );
            int hY = (int) ( ( analogBackImage.hHeight - ( axisFractionY * analogMaximum ) ) * scale );
            
            // Use other values if invalid
            if( hX < 0 )
                hX = (int) ( analogBackImage.hWidth * scale );
            if( hY < 0 )
                hY = (int) ( analogBackImage.hHeight * scale );
            
            // Update the position of the stick
            int cX = analogBackImage.x + hX;
            int cY = analogBackImage.y + hY;
            analogForeImage.fitCenter( cX, cY, analogBackImage.x, analogBackImage.y,
                    (int) ( analogBackImage.width * scale ), (int) ( analogBackImage.height * scale ) );
            return true;
        }
        return false;
    }
    
    /**
     * Updates the FPS indicator assets to reflect a new value.
     * 
     * @param fps The new FPS value.
     * 
     * @return True if the FPS assets changed.
     */
    public boolean updateFps( int fps )
    {
        // Clamp to positive, four digits max [0 - 9999]
        fps = Utility.clamp( fps, 0, 9999 );
        
        // Quick return if user has disabled FPS or it hasn't changed
        if( !mFpsEnabled || mFpsValue == fps )
            return false;
        
        // Store the new value
        mFpsValue = fps;
        
        // Refresh the FPS digits
        refreshFpsImages();
        refreshFpsPositions();
        
        return true;
    }
    
    /**
     * Updates the auto-hold assets to reflect a new value.
     * 
     * @param pressed The new autohold state value.
     * @param index   The index of the auto-hold mask.
     * 
     * @return True if the autohold assets changed.
     */
    public boolean updateAutoHold( boolean pressed, int index )
    {
        if( autoHoldImages[index] != null )
        {
            if( pressed )
                autoHoldImages[index].setAlpha( mTouchscreenTransparency );
            else
                autoHoldImages[index].setAlpha( 0 );
            return true;
        }
        return false;
    }
    
    /**
     * Refreshes the images used to draw the FPS string.
     */
    private void refreshFpsImages()
    {
        // Refresh the list of FPS digits
        String fpsString = Integer.toString( mFpsValue );
        mFpsDigits.clear();
        for( int i = 0; i < 4; i++ )
        {
            // Create a new sequence of numeral images
            if( i < fpsString.length() )
            {
                int numeral = SafeMethods.toInt( fpsString.substring( i, i + 1 ), -1 );
                if( numeral > -1 && numeral < 10 )
                {
                    // Clone the numeral from the font images and move to next digit
                    mFpsDigits.add( new Image( mResources, mNumerals[numeral] ) );
                }
            }
        }
    }
    
    /**
     * Refreshes the positions of the FPS images.
     */
    private void refreshFpsPositions()
    {
        // Compute the centroid of the FPS text
        int x = 0;
        int y = 0;
        if( mFpsFrame != null )
        {
            x = mFpsFrame.x + (int) ( ( mFpsFrame.width * mFpsFrame.scale ) * ( mFpsTextX / 100f ) );
            y = mFpsFrame.y + (int) ( ( mFpsFrame.height * mFpsFrame.scale ) * ( mFpsTextY / 100f ) );
        }
        
        // Compute the width of the FPS text
        int totalWidth = 0;
        for( Image digit : mFpsDigits )
            totalWidth += (int) ( digit.width * digit.scale );
        
        // Compute the starting position of the FPS text
        x -= (int) ( totalWidth / 2f );
        
        // Compute the position of each digit
        for( Image digit : mFpsDigits )
        {
            digit.setPos( x, y - (int) ( digit.hHeight * digit.scale ) );
            x += (int) ( digit.width * digit.scale );
        }
    }
    
    /**
     * Loads all touch map data from the filesystem.
     * 
     * @param skinDir    The directory containing the skin.ini and image files.
     * @param profile    The name of the touchscreen profile.
     * @param animated   True to load the analog assets in two parts for animation.
     * @param fpsEnabled True to display the FPS indicator.
     * @param scale      The factor to scale images by.
     * @param alpha      The opacity of the visible elements.
     */
    public void load( String skinDir, Profile profile, boolean animated, boolean fpsEnabled, float scale, int alpha )
    {
        mFpsEnabled = fpsEnabled;
        mScalingFactor = scale;
        mTouchscreenTransparency = alpha;
        
        super.load( skinDir, profile, animated );
        ConfigFile skin_ini = new ConfigFile( skinFolder + "/skin.ini" );
        mReferenceWidth = SafeMethods.toInt( skin_ini.get( "INFO", "referenceScreenWidth" ), 0 );
        mReferenceHeight = SafeMethods.toInt( skin_ini.get( "INFO", "referenceScreenHeight" ), 0 );
        mFpsTextX = SafeMethods.toInt( skin_ini.get( "INFO", "fps-numx" ), 50 );
        mFpsTextY = SafeMethods.toInt( skin_ini.get( "INFO", "fps-numy" ), 50 );
        mFpsMinPixels = SafeMethods.toInt( skin_ini.get( "INFO", "fps-minPixels" ), 0 );
        
        // Scale the assets to the last screensize used
        resize( cacheWidth, cacheHeight, cacheMetrics );
    }
    
    /*
     * (non-Javadoc)
     * 
     * @see
     * emu.project64.input.map.TouchMap#loadAllAssets(emu.project64
     * .profile.Profile, boolean)
     */
    @Override
    protected void loadAllAssets( Profile profile, boolean animated )
    {
        super.loadAllAssets( profile, animated );
        
        // Set the transparency of the images
        for( Image buttonImage : buttonImages )
        {
            buttonImage.setAlpha( mTouchscreenTransparency );
        }
        if( analogBackImage != null )
        {
            analogBackImage.setAlpha( mTouchscreenTransparency );
        }
        if( analogForeImage != null )
        {
            analogForeImage.setAlpha( mTouchscreenTransparency );
        }
        
        // Load the FPS and autohold images
        if( profile != null )
        {
            loadFpsIndicator( profile );
            loadAutoHoldImages( profile, "groupAB-holdA" );
            loadAutoHoldImages( profile, "groupAB-holdB" );
            loadAutoHoldImages( profile, "groupC-holdCu" );
            loadAutoHoldImages( profile, "groupC-holdCd" );
            loadAutoHoldImages( profile, "groupC-holdCl" );
            loadAutoHoldImages( profile, "groupC-holdCr" );
            loadAutoHoldImages( profile, "buttonL-holdL" );
            loadAutoHoldImages( profile, "buttonR-holdR" );
            loadAutoHoldImages( profile, "buttonZ-holdZ" );
            loadAutoHoldImages( profile, "buttonS-holdS" );
        }
    }
    
    /**
     * Loads FPS indicator assets and properties from the filesystem.
     * 
     * @param profile The touchscreen profile containing the FPS properties.
     */
    private void loadFpsIndicator( Profile profile )
    {
        int x = profile.getInt( "fps-x", -1 );
        int y = profile.getInt( "fps-y", -1 );
        
        if( x >= 0 && y >= 0 )
        {
            // Position (percentages of the screen dimensions)
            mFpsFrameX = x;
            mFpsFrameY = y;
            
            // Load frame image
            mFpsFrame = new Image( mResources, skinFolder + "/fps.png" );
            
            // Minimum factor the FPS indicator can be scaled by
            mFpsMinScale = mFpsMinPixels / (float) mFpsFrame.width;
            
            // Load numeral images
            String filename = "";
            try
            {
                // Make sure we can load them (they might not even exist)
                for( int i = 0; i < mNumerals.length; i++ )
                {
                    filename = skinFolder + "/fps-" + i + ".png";
                    mNumerals[i] = new Image( mResources, filename );
                }
            }
            catch( Exception e )
            {
                // Problem, let the user know
                Log.e( "VisibleTouchMap", "Problem loading fps numeral '" + filename
                        + "', error message: " + e.getMessage() );
            }
        }
    }
    
    /**
     * Loads auto-hold assets and properties from the filesystem.
     * 
     * @param profile The touchscreen profile containing the auto-hold properties.
     * @param name The name of the image to load.
     */
    private void loadAutoHoldImages( Profile profile, String name )
    {
        if ( !name.contains("-hold") )
            return;
        
        String[] fields = name.split( "-hold" );
        String group = fields[0];
        String hold = fields[1];
        
        int x = profile.getInt( group + "-x", -1 );
        int y = profile.getInt( group + "-y", -1 );
        Integer index = MASK_KEYS.get( hold );
        
        if( x >= 0 && y >= 0 && index != null )
        {
            // Position (percentages of the digitizer dimensions)
            autoHoldX[index] = x;
            autoHoldY[index] = y;
            
            // The drawable image is in PNG image format.
            autoHoldImages[index] = new Image( mResources, skinFolder + "/" + name + ".png" );
            autoHoldImages[index].setAlpha( 0 );
        }
    }
}
