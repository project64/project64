package emu.project64.profile;

import java.util.ArrayList;
import java.util.List;

import emu.project64.AndroidDevice;
import emu.project64.R;
import emu.project64.hack.MogaHack;
import emu.project64.input.AbstractController;
import emu.project64.input.map.InputMap;
import emu.project64.input.provider.AbstractProvider;
import emu.project64.input.provider.AbstractProvider.OnInputListener;
import emu.project64.input.provider.AxisProvider;
import emu.project64.input.provider.KeyProvider;
import emu.project64.input.provider.KeyProvider.ImeFormula;
import emu.project64.input.provider.MogaProvider;
import emu.project64.jni.NativeExports;
import emu.project64.jni.UISettingID;
import emu.project64.persistent.ConfigFile;
import emu.project64.persistent.ConfigFile.ConfigSection;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.PorterDuff;
import android.os.Bundle;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.bda.controller.Controller;

public class ControllerProfileActivity extends AppCompatActivity implements OnInputListener,
        OnClickListener
{
    // Visual settings
    private static final float UNMAPPED_BUTTON_ALPHA = 0.2f;
    private static final int UNMAPPED_BUTTON_FILTER = 0x66FFFFFF;
    private static final int MIN_LAYOUT_WIDTH_DP = 480;

    // Controller profile objects
    private ConfigFile mConfigFile;
    private Profile mProfile;

    // Input listening
    private KeyProvider mKeyProvider;
    private MogaProvider mMogaProvider;
    private AxisProvider mAxisProvider;
    private Controller mMogaController = Controller.getInstance( this );

    // Widgets
    private final Button[] mN64Buttons = new Button[InputMap.NUM_MAPPABLES];
    private TextView mFeedbackText;

    @Override
    public void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );

        // Initialize MOGA controller API
        // TODO: Remove hack after MOGA SDK is fixed
        // mMogaController.init();
        MogaHack.init( mMogaController, this );

        // Load the profile; fail fast if there are any programmer usage errors
        String name = NativeExports.UISettingsLoadString(UISettingID.Controller_CurrentProfile.getValue());
        if( TextUtils.isEmpty( name ) )
            throw new Error( "Invalid usage: profile name cannot be null or empty" );
        mConfigFile = new ConfigFile(NativeExports.UISettingsLoadString(UISettingID.Controller_ConfigFile.getValue()));
        ConfigSection section = mConfigFile.get( name );
        if( section == null )
        {
            //profile not found create it
            mConfigFile.put(name, "map", "-");
            section = mConfigFile.get( name );
            if( section == null )
            {
                throw new Error( "Invalid usage: profile name not found in config file" );
            }
        }
        mProfile = new Profile( false, section );

        // Set up input listeners
        mKeyProvider = new KeyProvider( ImeFormula.DEFAULT, AndroidDevice.getUnmappableKeyCodes() );
        mKeyProvider.registerListener( this );
        mMogaProvider = new MogaProvider( mMogaController );
        mMogaProvider.registerListener( this );
        if( AndroidDevice.IS_HONEYCOMB_MR1 )
        {
            mAxisProvider = new AxisProvider();
            mAxisProvider.registerListener( this );
        }

        // Initialize the layout
        initLayoutDefault();

        // Refresh everything
        refreshAllButtons();
    }

    @Override
    public void onResume()
    {
        super.onResume();
        mMogaController.onResume();
    }

    @Override
    public void onPause()
    {
        super.onPause();
        mMogaController.onPause();

        // Lazily persist the profile data; only need to do it on pause
        mProfile.writeTo( mConfigFile );
        mConfigFile.save();
    }

    @Override
    public void onDestroy()
    {
        super.onDestroy();
        mMogaController.exit();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        switch (item.getItemId())
        {
        case android.R.id.home:
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed()
    {
        finish();
    }

    private void initLayoutDefault()
    {
        WindowManager manager = (WindowManager) getSystemService( Context.WINDOW_SERVICE );
        DisplayMetrics metrics = new DisplayMetrics();
        manager.getDefaultDisplay().getMetrics( metrics );
        float scalefactor = (float) DisplayMetrics.DENSITY_DEFAULT / (float) metrics.densityDpi;
        int widthDp = Math.round( metrics.widthPixels * scalefactor );

        // For narrow screens, use an alternate layout
        if( widthDp < MIN_LAYOUT_WIDTH_DP )
        {
            setContentView( R.layout.controller_profile_activity_port );
        }
        else
        {
            setContentView( R.layout.controller_profile_activity );
        }

        // Add the tool bar to the activity (which supports the fancy menu/arrow animation)
        Toolbar toolbar = (Toolbar) findViewById( R.id.toolbar );
        toolbar.setTitle( getString(R.string.gamepad_title) );
        setSupportActionBar( toolbar );
        ActionBar actionbar = getSupportActionBar();

        if (AndroidDevice.IS_ICE_CREAM_SANDWICH)
        {
            actionbar.setHomeButtonEnabled(true);
            actionbar.setDisplayHomeAsUpEnabled(true);
        }

        // Initialize and refresh the widgets
        initWidgets();
    }

    private void initWidgets()
    {
        // Get the text view object
        mFeedbackText = (TextView) findViewById( R.id.textFeedback );
        mFeedbackText.setText( "" );

        // Create a button list to simplify highlighting and mapping
        setupButton( R.id.buttonDR, AbstractController.DPD_R );
        setupButton( R.id.buttonDL, AbstractController.DPD_L );
        setupButton( R.id.buttonDD, AbstractController.DPD_D );
        setupButton( R.id.buttonDU, AbstractController.DPD_U );
        setupButton( R.id.buttonS, AbstractController.START );
        setupButton( R.id.buttonZ, AbstractController.BTN_Z );
        setupButton( R.id.buttonB, AbstractController.BTN_B );
        setupButton( R.id.buttonA, AbstractController.BTN_A );
        setupButton( R.id.buttonCR, AbstractController.CPD_R );
        setupButton( R.id.buttonCL, AbstractController.CPD_L );
        setupButton( R.id.buttonCD, AbstractController.CPD_D );
        setupButton( R.id.buttonCU, AbstractController.CPD_U );
        setupButton( R.id.buttonR, AbstractController.BTN_R );
        setupButton( R.id.buttonL, AbstractController.BTN_L );
        setupButton( R.id.buttonAR, InputMap.AXIS_R );
        setupButton( R.id.buttonAL, InputMap.AXIS_L );
        setupButton( R.id.buttonAD, InputMap.AXIS_D );
        setupButton( R.id.buttonAU, InputMap.AXIS_U );
    }

    private void setupButton( int resId, int index )
    {
        mN64Buttons[index] = (Button) findViewById( resId );
        if( mN64Buttons[index] != null )
        {
            mN64Buttons[index].setOnClickListener( this );
        }
    }

    @Override
    public void onClick(View view)
    {
        // Handle button clicks in the mapping screen
        for( int i = 0; i < mN64Buttons.length; i++ )
        {
            // Find the button that was pressed
            if( view.equals( mN64Buttons[i] ) )
            {
                // Popup a dialog to listen to input codes from user
                Button button = (Button) view;
                popupListener( button.getText(), i );
            }
        }
    }

    private interface PromptInputCodeListener
    {
        public void onDialogClosed( int inputCode, int hardwareId, int which );
    }

    /**
     * Open a dialog to prompt the user for an input code.
     *
     * @param context            The activity context.
     * @param moga               The MOGA controller interface.
     * @param title              The title of the dialog.
     * @param message            The message to be shown inside the dialog.
     * @param neutralButtonText  The text to be shown on the neutral button, or null.
     * @param ignoredKeyCodes    The key codes to ignore.
     * @param listener           The listener to process the input code, when provided.
     */
    public static void promptInputCode( Context context, Controller moga, CharSequence title, CharSequence message,
            CharSequence neutralButtonText, final PromptInputCodeListener listener )
    {
        final ArrayList<AbstractProvider> providers = new ArrayList<AbstractProvider>();

        // Create a widget to dispatch key/motion event data
        FrameLayout view = new FrameLayout( context );
        EditText dummyImeListener = new EditText( context );
        dummyImeListener.setVisibility( View.INVISIBLE );
        dummyImeListener.setHeight( 0 );
        view.addView( dummyImeListener );

        // Set the focus parameters of the view so that it will dispatch events
        view.setFocusable( true );
        view.setFocusableInTouchMode( true );
        view.requestFocus();

        // Create the input event providers
        providers.add( new KeyProvider( view, ImeFormula.DEFAULT, AndroidDevice.getUnmappableKeyCodes() ) );
        providers.add( new MogaProvider( moga ) );
        if( AndroidDevice.IS_HONEYCOMB_MR1 )
        {
            providers.add( new AxisProvider( view ) );
        }

        // Notify the client when the user clicks the dialog's positive button
        DialogInterface.OnClickListener clickListener = new DialogInterface.OnClickListener()
        {
            @Override
            public void onClick( DialogInterface dialog, int which )
            {
                for( AbstractProvider provider : providers )
                    provider.unregisterAllListeners();
                listener.onDialogClosed( 0, 0, which );
            }
        };

        final AlertDialog dialog = new Builder( context ).setTitle( title ).setMessage( message ).setCancelable( false )
                .setNegativeButton( context.getString( android.R.string.cancel ), clickListener )
                .setPositiveButton( context.getString( android.R.string.ok ), clickListener )
                .setNeutralButton( neutralButtonText, clickListener ).setPositiveButton( null, null )
                .setView( view ).create();

        OnInputListener inputListener = new OnInputListener()
        {
            @Override
            public void onInput( int[] inputCodes, float[] strengths, int hardwareId )
            {
                if( inputCodes == null || strengths == null )
                    return;

                // Find the strongest input
                float maxStrength = 0;
                int strongestInputCode = 0;
                for( int i = 0; i < inputCodes.length; i++ )
                {
                    // Identify the strongest input
                    float strength = strengths[i];
                    if( strength > maxStrength )
                    {
                        maxStrength = strength;
                        strongestInputCode = inputCodes[i];
                    }
                }

                // Call the overloaded method with the strongest found
                onInput( strongestInputCode, maxStrength, hardwareId );
            }

            @Override
            public void onInput( int inputCode, float strength, int hardwareId )
            {
                if( inputCode != 0 && strength > AbstractProvider.STRENGTH_THRESHOLD )
                {
                    for( AbstractProvider provider : providers )
                        provider.unregisterAllListeners();
                    listener.onDialogClosed( inputCode, hardwareId, DialogInterface.BUTTON_POSITIVE );
                    dialog.dismiss();
                }
            }
        };

        // Connect the upstream event listeners
        for( AbstractProvider provider : providers )
        {
            provider.registerListener( inputListener );
        }

        // Launch the dialog
        dialog.show();
    }

    private void popupListener( CharSequence title, final int index )
    {
        final InputMap map = new InputMap( mProfile.get( "map" ) );
        String message = getString( R.string.inputMapActivity_popupMessage,map.getMappedCodeInfo( index ) );
        String btnText = getString( R.string.inputMapActivity_popupUnmap );

        PromptInputCodeListener listener = new PromptInputCodeListener()
        {
            @Override
            public void onDialogClosed( int inputCode, int hardwareId, int which )
            {
                if( which != DialogInterface.BUTTON_NEGATIVE )
                {
                    if( which == DialogInterface.BUTTON_POSITIVE )
                    {
                        map.map( inputCode, index );
                    }
                    else
                    {
                        map.unmapCommand( index );
                    }
                    mProfile.put( "map", map.serialize() );
                    refreshAllButtons();
                }

                // Refresh our MOGA provider since the prompt disconnected it
                mMogaProvider = new MogaProvider( mMogaController );
                mMogaProvider.registerListener( ControllerProfileActivity.this );
            }
        };
        promptInputCode( this, mMogaController, title, message, btnText, listener);
    }

    @Override
    public boolean onKeyDown( int keyCode, KeyEvent event )
    {
        return mKeyProvider.onKey( keyCode, event ) || super.onKeyDown( keyCode, event );
    }

    @Override
    public boolean onKeyUp( int keyCode, KeyEvent event )
    {
        return mKeyProvider.onKey( keyCode, event ) || super.onKeyUp( keyCode, event );
    }

    @TargetApi( 12 )
    @Override
    public boolean onGenericMotionEvent( MotionEvent event )
    {
        if( !AndroidDevice.IS_HONEYCOMB_MR1 )
        {
            return false;
        }
        return mAxisProvider.onGenericMotion( event ) || super.onGenericMotionEvent( event );
    }

    @Override
    public void onInput(int inputCode, float strength, int hardwareId)
    {
        refreshButton( inputCode, strength );
        refreshFeedbackText( inputCode, strength );
    }

    @Override
    public void onInput(int[] inputCodes, float[] strengths, int hardwareId)
    {
        float maxStrength = AbstractProvider.STRENGTH_THRESHOLD;
        int strongestInputCode = 0;
        for( int i = 0; i < inputCodes.length; i++ )
        {
            int inputCode = inputCodes[i];
            float strength = strengths[i];

            // Cache the strongest input
            if( strength > maxStrength )
            {
                maxStrength = strength;
                strongestInputCode = inputCode;
            }

            refreshButton( inputCode, strength );
        }
        refreshFeedbackText( strongestInputCode, maxStrength );
    }

    private void refreshFeedbackText( int inputCode, float strength )
    {
        // Update the feedback text (not all layouts include this, so check null)
        if( mFeedbackText != null )
        {
            mFeedbackText.setText( strength > AbstractProvider.STRENGTH_THRESHOLD ? AbstractProvider.getInputName( inputCode ) : "" );
        }
    }

    private void refreshButton( int inputCode, float strength )
    {
        InputMap map = new InputMap( mProfile.get( "map" ) );
        int command = map.get( inputCode );
        if( command != InputMap.UNMAPPED )
        {
            Button button = mN64Buttons[command];
            refreshButton( button, strength, true );
        }
    }

    @TargetApi( 11 )
    private void refreshButton( Button button, float strength, boolean isMapped )
    {
        if( button != null )
        {
            button.setPressed( strength > AbstractProvider.STRENGTH_THRESHOLD );

            // Fade any buttons that aren't mapped
            if( AndroidDevice.IS_HONEYCOMB )
            {
                if( isMapped )
                {
                    button.setAlpha( 1 );
                }
                else
                {
                    button.setAlpha( UNMAPPED_BUTTON_ALPHA );
                }
            }
            else
            {
                // For older APIs try something similar (not quite the same)
                if( isMapped )
                {
                    button.getBackground().clearColorFilter();
                }
                else
                {
                    button.getBackground().setColorFilter( UNMAPPED_BUTTON_FILTER, PorterDuff.Mode.MULTIPLY );
                }
                button.invalidate();
            }
        }
    }

    private void refreshAllButtons()
    {
        final InputMap map = new InputMap( mProfile.get( "map" ) );
        for( int i = 0; i < mN64Buttons.length; i++ )
        {
            refreshButton( mN64Buttons[i], 0, map.isMapped( i ) );
        }
    }
}
