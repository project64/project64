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
package emu.project64.game;

import emu.project64.input.TouchController;
import emu.project64.input.map.VisibleTouchMap;
import emu.project64.util.DeviceUtil;
import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.View;

public class GameOverlay extends View implements TouchController.OnStateChangedListener
{
    private VisibleTouchMap mTouchMap;
    private boolean mDrawingEnabled = true;
    private int mHatRefreshPeriod = 0;
    private int mHatRefreshCount = 0;
    
    public GameOverlay( Context context, AttributeSet attribs )
    {
        super( context, attribs );
        requestFocus();
    }
    
    public void initialize( VisibleTouchMap touchMap, boolean drawingEnabled, boolean fpsEnabled, boolean joystickAnimated )
    {
        mTouchMap = touchMap;
        mDrawingEnabled = drawingEnabled;
        mHatRefreshPeriod = joystickAnimated ? 3 : 0;
    }
    
    @Override
    public void onAnalogChanged( float axisFractionX, float axisFractionY )
    {
        if( mHatRefreshPeriod > 0 && mDrawingEnabled )
        {
            // Increment the count since last refresh
            mHatRefreshCount++;
            
            // If stick re-centered, always refresh
            if( axisFractionX == 0 && axisFractionY == 0 )
                mHatRefreshCount = 0;
            
            // Update the analog stick assets and redraw if required
            if( mHatRefreshCount % mHatRefreshPeriod == 0 && mTouchMap != null
                    && mTouchMap.updateAnalog( axisFractionX, axisFractionY ) )
            {
                postInvalidate();
            }
        }
    }

    @Override
    public void onAutoHold( boolean autoHold, int index )
    {
        // Update the AutoHold mask, and redraw if required
        if( mTouchMap != null && mTouchMap.updateAutoHold( autoHold , index) )
        {
            postInvalidate();
        }
    }
        
    @Override
    protected void onSizeChanged( int w, int h, int oldw, int oldh )
    {
        // Recompute skin layout geometry
        if( mTouchMap != null )
            mTouchMap.resize( w, h, DeviceUtil.getDisplayMetrics( this ) );
        super.onSizeChanged( w, h, oldw, oldh );
    }
    
    @Override
    protected void onDraw( Canvas canvas )
    {
        if(canvas == null )
            return;
        
        if( mTouchMap != null && mDrawingEnabled )
        {
            // Redraw the static buttons
            mTouchMap.drawButtons( canvas );
        
            // Redraw the dynamic analog stick
            mTouchMap.drawAnalog( canvas );
            
            // Redraw the autoHold mask
            mTouchMap.drawAutoHold( canvas );
        }
    }
}
