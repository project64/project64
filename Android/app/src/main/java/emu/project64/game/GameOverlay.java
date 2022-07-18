package emu.project64.game;

import java.util.Timer;
import java.util.TimerTask;

import emu.project64.input.TouchController;
import emu.project64.input.map.VisibleTouchMap;
import emu.project64.util.DeviceUtil;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.view.View;

public class GameOverlay extends View implements TouchController.OnStateChangedListener
{
    private VisibleTouchMap mTouchMap;
    private boolean mDrawingEnabled = true;
    private int mHatRefreshPeriod = 0;
    private int mHatRefreshCount = 0;
    private Paint mPaint = new Paint();
    private Rect mRectangle = new Rect();
    private String mDisplayMessage = "";
    private String mDisplayMessage2 = "";
    private int mDisplayMessageDuration = 0;
    private Timer mTimer = null;

    public GameOverlay(Context context, AttributeSet attribs)
    {
        super(context, attribs);
        requestFocus();
    }

    public void initialize( VisibleTouchMap touchMap, boolean drawingEnabled, boolean joystickAnimated )
    {
        mTouchMap = touchMap;
        mDrawingEnabled = drawingEnabled;
        mHatRefreshPeriod = joystickAnimated ? 3 : 0;
    }

    public void SetDisplayMessage(String Message, int Duratation) 
    {
        if (Duratation >= mDisplayMessageDuration)
        {
            mDisplayMessage = Message;
            mDisplayMessageDuration = Duratation;
            if (mTimer != null)
            {
                mTimer.cancel();
                mTimer.purge();
            }
            if (Duratation == 0)
            {
                Duratation = 10;
            }
            TimerTask task = new TimerTask() 
            {
                @Override
                public void run() 
                {
                    mDisplayMessage = "";
                    mDisplayMessageDuration = 0;
                    Timer CurrentTimer = mTimer;
                    mTimer = null;
                    CurrentTimer.cancel();
                    CurrentTimer.purge();
                }

            };
            mTimer = new Timer();
            mTimer.schedule(task, Duratation * 1000);            
        }
        postInvalidate();
    }

    public void SetDisplayMessage2(String Message) 
    {
        mDisplayMessage2 = Message;
        postInvalidate();
    }

    @Override
    public void onAnalogChanged(float axisFractionX, float axisFractionY) 
    {
        if (mHatRefreshPeriod > 0 && mDrawingEnabled) 
        {
            // Increment the count since last refresh
            mHatRefreshCount++;

            // If stick re-centered, always refresh
            if (axisFractionX == 0 && axisFractionY == 0)
            {
                mHatRefreshCount = 0;
            }

            // Update the analog stick assets and redraw if required
            if (mHatRefreshCount % mHatRefreshPeriod == 0 && mTouchMap != null
                    && mTouchMap.updateAnalog(axisFractionX, axisFractionY)) 
            {
                postInvalidate();
            }
        }
    }

    @Override
    public void onAutoHold(boolean autoHold, int index) 
    {
        // Update the AutoHold mask, and redraw if required
        if (mTouchMap != null && mTouchMap.updateAutoHold(autoHold, index))
        {
            postInvalidate();
        }
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh)
    {
        // Recompute skin layout geometry
        if (mTouchMap != null)
        {
            mTouchMap.resize(w, h, DeviceUtil.getDisplayMetrics(this));
        }
        super.onSizeChanged(w, h, oldw, oldh);
    }

    @Override
    protected void onDraw(Canvas canvas) 
    {
        if (canvas == null)
        {
            return;
        }

        if (mTouchMap != null && mDrawingEnabled) 
        {
            // Redraw the static buttons
            mTouchMap.drawButtons(canvas);

            // Redraw the dynamic analog stick
            mTouchMap.drawAnalog(canvas);

            // Redraw the autoHold mask
            mTouchMap.drawAutoHold(canvas);
        }

        String txt = mDisplayMessage;
        if (txt.length() > 0 && mDisplayMessage2.length() > 0) 
        {
            txt += " - ";
        }
        txt += mDisplayMessage2;
        if (txt.length() > 0) 
        {
            mPaint.setStyle(Paint.Style.FILL);
            mPaint.setColor(Color.parseColor("#DDDDDD"));
            mPaint.setAntiAlias(true);
            mPaint.setTextSize(25);
            mPaint.setTextAlign(Paint.Align.CENTER);
            Typeface typeface = Typeface.create(Typeface.SANS_SERIF,
                    Typeface.BOLD_ITALIC);
            mPaint.setTypeface(typeface);
            mPaint.getTextBounds(txt, 0, txt.length(), mRectangle);
            canvas.drawText(txt, Math.abs(getWidth() / 2), Math.abs((int) (getHeight() * 0.95)) - Math.abs(mRectangle.height()), mPaint);
        }
    }
}
