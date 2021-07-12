package emu.project64.game;

import java.io.Writer;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.opengles.GL10;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class GameSurface extends SurfaceView implements SurfaceHolder.Callback
{
    public class EGL14 
    {
        public static final int EGL_OPENGL_ES2_BIT = 0x0004;
    }
    
    public class EGLExt 
    {
        public static final int EGL_OPENGL_ES3_BIT_KHR = 0x0040;
    }

    private final static boolean LOG_THREADS = false;
    private final static boolean LOG_SURFACE = false;
    private final static boolean LOG_EGL = false;

    /**
     * Check glError() after every GL call and throw an exception if glError indicates
     * that an error has occurred. This can be used to help track down which OpenGL ES call
     * is causing an error.
     *
     * @see #getDebugFlags
     * @see #setDebugFlags
     */
    public final static int DEBUG_CHECK_GL_ERROR = 1;
    
    /**
     * Log GL calls to the system log at "verbose" level with tag "GLSurfaceView".
     *
     * @see #getDebugFlags
     * @see #setDebugFlags
     */
    public final static int DEBUG_LOG_GL_CALLS = 2;

    // LogCat strings for debugging, defined here to simplify maintenance/lookup
    private static final String TAG = "GameSurface";

    public interface SurfaceInfo 
    {
        /**
         * Called when the surface is created or recreated.
         * <p>
         * Called when the rendering thread
         * starts and whenever the EGL context is lost. The EGL context will typically
         * be lost when the Android device awakes after going to sleep.
         * <p>
         * Since this method is called at the beginning of rendering, as well as
         * every time the EGL context is lost, this method is a convenient place to put
         * code to create resources that need to be created when the rendering
         * starts, and that need to be recreated when the EGL context is lost.
         * Textures are an example of a resource that you might want to create
         * here.
         * <p>
         * Note that when the EGL context is lost, all OpenGL resources associated
         * with that context will be automatically deleted. You do not need to call
         * the corresponding "glDelete" methods such as glDeleteTextures to
         * manually delete these lost resources.
         * <p>
         * @param mGl the GL interface. Use <code>instanceof</code> to
         * test if the interface supports GL11 or higher interfaces.
         * @param config the EGLConfig of the created surface. Can be used
         * to create matching pbuffers.
         */
        void onSurfaceCreated(GL10 mGl, EGLConfig config);

        /**
         * Called when the surface changed size.
         * <p>
         * Called after the surface is created and whenever
         * the OpenGL ES surface size changes.
         * <p>
         * Typically you will set your viewport here. If your camera
         * is fixed then you could also set your projection matrix here:
         * <pre class="prettyprint">
         * void onSurfaceChanged(GL10 mGl, int width, int height) {
         *     mGl.glViewport(0, 0, width, height);
         *     // for a fixed camera, set the projection too
         *     float ratio = (float) width / height;
         *     mGl.glMatrixMode(GL10.GL_PROJECTION);
         *     mGl.glLoadIdentity();
         *     mGl.glFrustumf(-ratio, ratio, -1, 1, 1, 10);
         * }
         * </pre>
         * @param mGl the GL interface. Use <code>instanceof</code> to
         * test if the interface supports GL11 or higher interfaces.
         * @param width
         * @param height
         */
        void onSurfaceChanged(GL10 mGl, int width, int height);
    }

    /**
     * Constructor that is called when inflating a view from XML. This is called when a view is
     * being constructed from an XML file, supplying attributes that were specified in the XML file.
     * This version uses a default style of 0, so the only attribute values applied are those in the
     * Context's Theme and the given AttributeSet. The method onFinishInflate() will be called after
     * all children have been added.
     * 
     * @param context The Context the view is running in, through which it can access the current
     *            theme, resources, etc.
     * @param attrs The attributes of the XML tag that is inflating the view.
     */
    public GameSurface( Context context, AttributeSet attribs )
    {
        super( context, attribs );
        init();
    }

    @Override
    protected void finalize() throws Throwable 
    {
        super.finalize();
    }

    private void init()
    {
        // Install a SurfaceHolder.Callback so we get notified when the
        // underlying surface is created and destroyed
        SurfaceHolder holder = getHolder();
        holder.addCallback(this);
        // setFormat is done by SurfaceView in SDK 2.3 and newer. Uncomment
        // this statement if back-porting to 2.2 or older:
        // holder.setFormat(PixelFormat.RGB_565);
        //
        // setType is not needed for SDK 2.0 or newer. Uncomment this
        // statement if back-porting this code to older SDKs.
        // holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
        if (mEGLConfigChooser == null) 
        {
            mEGLConfigChooser = new SimpleEGLConfigChooser(true);
        }
        if (mEGLContextFactory == null) 
        {
            mEGLContextFactory = new DefaultContextFactory();
        }
        if (mEGLWindowSurfaceFactory == null)
        {
            mEGLWindowSurfaceFactory = new DefaultWindowSurfaceFactory();
        }
        mEglHelper = new EglHelper(new WeakReference<GameSurface>(this));
    }
    
    public boolean createGLContext( ActivityManager activityManager )
    {
        ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();

        final boolean supportsEs2 = configurationInfo.reqGlEsVersion >= 0x20000 || isProbablyEmulator();
        if (supportsEs2) 
        {
            if (isProbablyEmulator()) 
            {
                // Avoids crashes on startup with some emulator images.
                this.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
            }

            this.setEGLContextClientVersion(2);
        }
        else 
        {
            // Should never be seen in production, since the manifest filters
            // unsupported devices.
            Log.e(TAG, "This device does not support OpenGL ES 2.0.");
            return false;
        }
        return true;
    }
    
    private boolean isProbablyEmulator() 
    {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1
                && (Build.FINGERPRINT.startsWith("generic")
                || Build.FINGERPRINT.startsWith("unknown")
                || Build.MODEL.contains("google_sdk")
                || Build.MODEL.contains("Emulator")
                || Build.MODEL.contains("Android SDK built for x86"));
    }
    
    public boolean ableToDraw()
    {
        return mHaveEglContext && mHaveEglSurface && readyToDraw();
    }

    boolean readyToDraw() 
    {
        return (!mPaused) && mHasSurface && (!mSurfaceIsBad)
            && (mWidth > 0) && (mHeight > 0);
    }

    /**
     * Install a custom EGLWindowSurfaceFactory.
     * <p>If this method is
     * called, it must be called before {@link #setRenderer(Renderer)}
     * is called.
     * <p>
     * If this method is not called, then by default
     * a window surface will be created with a null attribute list.
     */
    public void setEGLWindowSurfaceFactory(EGLWindowSurfaceFactory factory)
    {
        mEGLWindowSurfaceFactory = factory;
    }

    /**
     * Install a custom EGLConfigChooser.
     * <p>If this method is
     * called, it must be called before {@link #setRenderer(Renderer)}
     * is called.
     * <p>
     * If no setEGLConfigChooser method is called, then by default the
     * view will choose an EGLConfig that is compatible with the current
     * android.view.Surface, with a depth buffer depth of
     * at least 16 bits.
     * @param configChooser
     */
    public void setEGLConfigChooser(EGLConfigChooser configChooser) 
    {
        mEGLConfigChooser = configChooser;
    }

    /**
     * Install a config chooser which will choose a config
     * with at least the specified depthSize and stencilSize,
     * and exactly the specified redSize, greenSize, blueSize and alphaSize.
     * <p>If this method is
     * called, it must be called before {@link #setRenderer(Renderer)}
     * is called.
     * <p>
     * If no setEGLConfigChooser method is called, then by default the
     * view will choose an RGB_888 surface with a depth buffer depth of
     * at least 16 bits.
     *
     */
    public void setEGLConfigChooser(int redSize, int greenSize, int blueSize, int alphaSize, int depthSize, int stencilSize) 
    {
        setEGLConfigChooser(new ComponentSizeChooser(redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize));
    }

    /**
     * Inform the default EGLContextFactory and default EGLConfigChooser
     * which EGLContext client version to pick.
     * <p>Use this method to create an OpenGL ES 2.0-compatible context.
     * Example:
     * <pre class="prettyprint">
     *     public MyView(Context context) {
     *         super(context);
     *         setEGLContextClientVersion(2); // Pick an OpenGL ES 2.0 context.
     *         setRenderer(new MyRenderer());
     *     }
     * </pre>
     * <p>Note: Activities which require OpenGL ES 2.0 should indicate this by
     * setting @lt;uses-feature android:glEsVersion="0x00020000" /> in the activity's
     * AndroidManifest.xml file.
     * <p>If this method is called, it must be called before {@link #setRenderer(Renderer)}
     * is called.
     * <p>This method only affects the behavior of the default EGLContexFactory and the
     * default EGLConfigChooser. If
     * {@link #setEGLContextFactory(EGLContextFactory)} has been called, then the supplied
     * EGLContextFactory is responsible for creating an OpenGL ES 2.0-compatible context.
     * If
     * {@link #setEGLConfigChooser(EGLConfigChooser)} has been called, then the supplied
     * EGLConfigChooser is responsible for choosing an OpenGL ES 2.0-compatible config.
     * @param version The EGLContext client version to choose. Use 2 for OpenGL ES 2.0
     */
    public void setEGLContextClientVersion(int version)
    {
        mEGLContextClientVersion = version;
    }
    /**
     * This method is part of the SurfaceHolder.Callback interface, and is
     * not normally called or subclassed by clients of GLSurfaceView.
     */
    public void surfaceCreated(SurfaceHolder holder) 
    {
        synchronized(sGLThreadManager)
        {
            if (LOG_THREADS) 
            {
                Log.i("GLThread", "surfaceCreated tid=" + getId());
            }
            mHasSurface = true;
            mFinishedCreatingEglSurface = false;
            sGLThreadManager.notifyAll();
            while (mWaitingForSurface && !mFinishedCreatingEglSurface && !mExited) 
            {
                try 
                {
                    sGLThreadManager.wait();
                }
                catch (InterruptedException e) 
                {
                    Thread.currentThread().interrupt();
                }
            }
        }
    }

    /**
     * This method is part of the SurfaceHolder.Callback interface, and is
     * not normally called or subclassed by clients of GLSurfaceView.
     */
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        synchronized(sGLThreadManager) 
        {
            if (LOG_THREADS)
            {
                Log.i("GLThread", "surfaceDestroyed tid=" + getId());
            }
            mHasSurface = false;
            sGLThreadManager.notifyAll();
            while((!mWaitingForSurface) && (!mExited))
            {
                try 
                {
                    sGLThreadManager.wait();
                }
                catch (InterruptedException e)
                {
                    Thread.currentThread().interrupt();
                }
            }
        }
    }

    /**
     * This method is part of the SurfaceHolder.Callback interface, and is
     * not normally called or subclassed by clients of GLSurfaceView.
     */
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) 
    {
        synchronized (sGLThreadManager)
        {
            mWidth = w;
            mHeight = h;
            mSizeChanged = true;
            mRequestRender = true;
            mRenderComplete = false;
            sGLThreadManager.notifyAll();

            // Wait for thread to react to resize and render a frame
            while (! mExited && !mPaused && !mRenderComplete && ableToDraw())
            {
                if (LOG_SURFACE) {
                    Log.i("Main thread", "onWindowResize waiting for render complete from tid=" + getId());
                }
                try
                {
                    sGLThreadManager.wait();
                } 
                catch (InterruptedException ex)
                {
                    Thread.currentThread().interrupt();
                }
            }
        }
    }

    /**
     * An interface used to wrap a GL interface.
     * <p>Typically
     * used for implementing debugging and tracing on top of the default
     * GL interface. You would typically use this by creating your own class
     * that implemented all the GL methods by delegating to another GL instance.
     * Then you could add your own behavior before or after calling the
     * delegate. All the GLWrapper would do was instantiate and return the
     * wrapper GL instance:
     * <pre class="prettyprint">
     * class MyGLWrapper implements GLWrapper {
     *     GL wrap(GL gl) {
     *         return new MyGLImplementation(gl);
     *     }
     *     static class MyGLImplementation implements GL,GL10,GL11,... {
     *         ...
     *     }
     * }
     * </pre>
     * @see #setGLWrapper(GLWrapper)
     */
    public interface GLWrapper
    {
        /**
         * Wraps a gl interface in another gl interface.
         * @param gl a GL interface that is to be wrapped.
         * @return either the input argument or another GL object that wraps the input argument.
         */
        GL wrap(GL gl);
    }

    /**
     * An EGL helper class.
     */
    static class EglHelper
    {
        public EglHelper(WeakReference<GameSurface> glSurfaceViewWeakRef)
        {
            mGLSurfaceViewWeakRef = glSurfaceViewWeakRef;
        }

        /**
         * Initialize EGL for a given configuration spec.
         * @param configSpec
         */
        public void start() 
        {
            if (LOG_EGL)
            {
                Log.w("EglHelper", "start() tid=" + Thread.currentThread().getId());
            }
            /*
             * Get an EGL instance
             */
            mEgl = (EGL10) EGLContext.getEGL();

            /*
             * Get to the default display.
             */
            mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

            if (mEglDisplay == EGL10.EGL_NO_DISPLAY)
            {
                throw new RuntimeException("eglGetDisplay failed");
            }

            /*
             * We can now initialize EGL for that display
             */
            int[] version = new int[2];
            if(!mEgl.eglInitialize(mEglDisplay, version)) 
            {
                throw new RuntimeException("eglInitialize failed");
            }
            GameSurface view = mGLSurfaceViewWeakRef.get();
            if (view == null) 
            {
                mEglConfig = null;
                mEglContext = null;
            } 
            else 
            {
                mEglConfig = view.mEGLConfigChooser.chooseConfig(mEgl, mEglDisplay);

                /*
                * Create an EGL context. We want to do this as rarely as we can, because an
                * EGL context is a somewhat heavy object.
                */
                mEglContext = view.mEGLContextFactory.createContext(mEgl, mEglDisplay, mEglConfig);
            }
            if (mEglContext == null || mEglContext == EGL10.EGL_NO_CONTEXT)
            {
                mEglContext = null;
                throwEglException("createContext");
            }
            if (LOG_EGL)
            {
                Log.w("EglHelper", "createContext " + mEglContext + " tid=" + Thread.currentThread().getId());
            }

            mEglSurface = null;
        }

        /**
         * Create an egl surface for the current SurfaceHolder surface. If a surface
         * already exists, destroy it before creating the new surface.
         *
         * @return true if the surface was created successfully.
         */
        public boolean createSurface() 
        {
            if (LOG_EGL) 
            {
                Log.w("EglHelper", "createSurface()  tid=" + Thread.currentThread().getId());
            }
            /*
             * Check preconditions.
             */
            if (mEgl == null) 
            {
                throw new RuntimeException("egl not initialized");
            }
            if (mEglDisplay == null) 
            {
                throw new RuntimeException("eglDisplay not initialized");
            }
            if (mEglConfig == null) 
            {
                throw new RuntimeException("mEglConfig not initialized");
            }

            /*
             *  The window size has changed, so we need to create a new
             *  surface.
             */
            destroySurfaceImp();

            /*
             * Create an EGL surface we can render into.
             */
            GameSurface view = mGLSurfaceViewWeakRef.get();
            if (view != null)
            {
                mEglSurface = view.mEGLWindowSurfaceFactory.createWindowSurface(mEgl, mEglDisplay, mEglConfig, view.getHolder());
            }
            else 
            {
                mEglSurface = null;
            }

            if (mEglSurface == null || mEglSurface == EGL10.EGL_NO_SURFACE)
            {
                int error = mEgl.eglGetError();
                if (error == EGL10.EGL_BAD_NATIVE_WINDOW)
                {
                    Log.e("EglHelper", "createWindowSurface returned EGL_BAD_NATIVE_WINDOW.");
                }
                return false;
            }

            /*
             * Before we can issue GL commands, we need to make sure
             * the context is current and bound to a surface.
             */
            if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext))
            {
                /*
                 * Could not make the context current, probably because the underlying
                 * SurfaceView surface has been destroyed.
                 */
                logEglErrorAsWarning("EGLHelper", "eglMakeCurrent", mEgl.eglGetError());
                return false;
            }

            return true;
        }

        /**
         * Create a GL object for the current EGL context.
         * @return
         */
        GL createGL() 
        {
            GL gl = mEglContext.getGL();
            GameSurface view = mGLSurfaceViewWeakRef.get();
            if (view != null) 
            {
                if (view.mGLWrapper != null)
                {
                    gl = view.mGLWrapper.wrap(gl);
                }
            }
            return gl;
        }

        /**
         * Display the current render surface.
         * @return the EGL error code from eglSwapBuffers.
         */
        public int swap()
        {
            if (! mEgl.eglSwapBuffers(mEglDisplay, mEglSurface))
            {
                return mEgl.eglGetError();
            }
            return EGL10.EGL_SUCCESS;
        }

        public void destroySurface()
        {
            if (LOG_EGL) 
            {
                Log.w("EglHelper", "destroySurface()  tid=" + Thread.currentThread().getId());
            }
            destroySurfaceImp();
        }

        private void destroySurfaceImp() 
        {
            if (mEglSurface != null && mEglSurface != EGL10.EGL_NO_SURFACE)
            {
                mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
                GameSurface view = mGLSurfaceViewWeakRef.get();
                if (view != null) 
                {
                    view.mEGLWindowSurfaceFactory.destroySurface(mEgl, mEglDisplay, mEglSurface);
                }
                mEglSurface = null;
            }
        }

        public void finish() 
        {
            if (LOG_EGL) 
            {
                Log.w("EglHelper", "finish() tid=" + Thread.currentThread().getId());
            }
            if (mEglContext != null) 
            {
                GameSurface view = mGLSurfaceViewWeakRef.get();
                if (view != null) 
                {
                    view.mEGLContextFactory.destroyContext(mEgl, mEglDisplay, mEglContext);
                }
                mEglContext = null;
            }
            if (mEglDisplay != null) 
            {
                mEgl.eglTerminate(mEglDisplay);
                mEglDisplay = null;
            }
        }

        private void throwEglException(String function) 
        {
            throwEglException(function, mEgl.eglGetError());
        }

        public static void throwEglException(String function, int error) 
        {
            String message = formatEglError(function, error);
            if (LOG_THREADS) 
            {
                Log.e("EglHelper", "throwEglException tid=" + Thread.currentThread().getId() + " " + message);
            }
            throw new RuntimeException(message);
        }

        public static void logEglErrorAsWarning(String tag, String function, int error) 
        {
            Log.w(tag, formatEglError(function, error));
        }

        public static String formatEglError(String function, int error) 
        {
            return function + " failed: " + error;
        }

        private WeakReference<GameSurface> mGLSurfaceViewWeakRef;
        EGL10 mEgl;
        EGLDisplay mEglDisplay;
        EGLSurface mEglSurface;
        EGLConfig mEglConfig;
        EGLContext mEglContext;
    }

    /**
     * An interface for customizing the eglCreateContext and eglDestroyContext calls.
     * <p>
     * This interface must be implemented by clients wishing to call
     * {@link GLSurfaceView#setEGLContextFactory(EGLContextFactory)}
     */
    public interface EGLContextFactory 
    {
        EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig);
        void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context);
    }

    private class DefaultContextFactory implements EGLContextFactory 
    {
        private int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig config)
        {
            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, mEGLContextClientVersion, EGL10.EGL_NONE };

            return egl.eglCreateContext(display, config, EGL10.EGL_NO_CONTEXT, mEGLContextClientVersion != 0 ? attrib_list : null);
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) 
        {
            if (!egl.eglDestroyContext(display, context)) 
            {
                Log.e("DefaultContextFactory", "display:" + display + " context: " + context);
                if (LOG_THREADS) 
                {
                    Log.i("DefaultContextFactory", "tid=" + Thread.currentThread().getId());
                }
                String message = "eglDestroyContex failed: " + egl.eglGetError();
                if (LOG_THREADS) 
                {
                    Log.e("EglHelper", "throwEglException tid=" + Thread.currentThread().getId() + " " + message);
                }
                throw new RuntimeException(message);
            }
        }
    }
    
    /**
     * An interface for customizing the eglCreateWindowSurface and eglDestroySurface calls.
     * <p>
     * This interface must be implemented by clients wishing to call
     * {@link GLSurfaceView#setEGLWindowSurfaceFactory(EGLWindowSurfaceFactory)}
     */
    public interface EGLWindowSurfaceFactory 
    {
        /**
         *  @return null if the surface cannot be constructed.
         */
        EGLSurface createWindowSurface(EGL10 egl, EGLDisplay display, EGLConfig config, Object nativeWindow);
        void destroySurface(EGL10 egl, EGLDisplay display, EGLSurface surface);
    }

    private static class DefaultWindowSurfaceFactory implements EGLWindowSurfaceFactory 
    {

        public EGLSurface createWindowSurface(EGL10 egl, EGLDisplay display, EGLConfig config, Object nativeWindow) 
        {
            EGLSurface result = null;
            try 
            {
                result = egl.eglCreateWindowSurface(display, config, nativeWindow, null);
            }
            catch (IllegalArgumentException e) 
            {
                // This exception indicates that the surface flinger surface
                // is not valid. This can happen if the surface flinger surface has
                // been torn down, but the application has not yet been
                // notified via SurfaceHolder.Callback.surfaceDestroyed.
                // In theory the application should be notified first,
                // but in practice sometimes it is not. See b/4588890
                Log.e(TAG, "eglCreateWindowSurface", e);
            }
            return result;
        }

        public void destroySurface(EGL10 egl, EGLDisplay display, EGLSurface surface) 
        {
            egl.eglDestroySurface(display, surface);
        }
    }

    /**
     * An interface for choosing an EGLConfig configuration from a list of
     * potential configurations.
     * <p>
     * This interface must be implemented by clients wishing to call
     * {@link GLSurfaceView#setEGLConfigChooser(EGLConfigChooser)}
     */
    public interface EGLConfigChooser 
    {
        /**
         * Choose a configuration from the list. Implementors typically
         * implement this method by calling
         * {@link EGL10#eglChooseConfig} and iterating through the results. Please consult the
         * EGL specification available from The Khronos Group to learn how to call eglChooseConfig.
         * @param egl the EGL10 for the current display.
         * @param display the current display.
         * @return the chosen configuration.
         */
        EGLConfig chooseConfig(EGL10 egl, EGLDisplay display);
    }

    private abstract class BaseConfigChooser implements EGLConfigChooser 
    {
        public BaseConfigChooser(int[] configSpec) 
        {
            mConfigSpec = filterConfigSpec(configSpec);
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) 
        {
            int[] num_config = new int[1];
            if (!egl.eglChooseConfig(display, mConfigSpec, null, 0, num_config)) 
            {
                throw new IllegalArgumentException("eglChooseConfig failed");
            }

            int numConfigs = num_config[0];

            if (numConfigs <= 0) 
            {
                throw new IllegalArgumentException( "No configs match configSpec");
            }

            EGLConfig[] configs = new EGLConfig[numConfigs];
            if (!egl.eglChooseConfig(display, mConfigSpec, configs, numConfigs, num_config)) 
            {
                throw new IllegalArgumentException("eglChooseConfig#2 failed");
            }
            EGLConfig config = chooseConfig(egl, display, configs);
            if (config == null) 
            {
                throw new IllegalArgumentException("No config chosen");
            }
            return config;
        }

        abstract EGLConfig chooseConfig(EGL10 egl, EGLDisplay display, EGLConfig[] configs);

        protected int[] mConfigSpec;

        private int[] filterConfigSpec(int[] configSpec) 
        {
            if (mEGLContextClientVersion != 2 && mEGLContextClientVersion != 3) 
            {
                return configSpec;
            }
            /* We know none of the subclasses define EGL_RENDERABLE_TYPE.
             * And we know the configSpec is well formed.
             */
            int len = configSpec.length;
            int[] newConfigSpec = new int[len + 2];
            System.arraycopy(configSpec, 0, newConfigSpec, 0, len-1);
            newConfigSpec[len-1] = EGL10.EGL_RENDERABLE_TYPE;
            if (mEGLContextClientVersion == 2)
            {
                newConfigSpec[len] = EGL14.EGL_OPENGL_ES2_BIT;  /* EGL_OPENGL_ES2_BIT */
            }
            else 
            {
                newConfigSpec[len] = EGLExt.EGL_OPENGL_ES3_BIT_KHR; /* EGL_OPENGL_ES3_BIT_KHR */
            }
            newConfigSpec[len+1] = EGL10.EGL_NONE;
            return newConfigSpec;
        }
    }

    /**
     * Choose a configuration with exactly the specified r,g,b,a sizes,
     * and at least the specified depth and stencil sizes.
     */
    private class ComponentSizeChooser extends BaseConfigChooser 
    {
        public ComponentSizeChooser(int redSize, int greenSize, int blueSize, int alphaSize, int depthSize, int stencilSize) 
        {
            super(new int[] 
            {
                EGL10.EGL_RED_SIZE, redSize,
                EGL10.EGL_GREEN_SIZE, greenSize,
                EGL10.EGL_BLUE_SIZE, blueSize,
                EGL10.EGL_ALPHA_SIZE, alphaSize,
                EGL10.EGL_DEPTH_SIZE, depthSize,
                EGL10.EGL_STENCIL_SIZE, stencilSize,
                EGL10.EGL_NONE
            });
            mValue = new int[1];
            mRedSize = redSize;
            mGreenSize = greenSize;
            mBlueSize = blueSize;
            mAlphaSize = alphaSize;
            mDepthSize = depthSize;
            mStencilSize = stencilSize;
       }

        @Override
        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display, EGLConfig[] configs) 
        {
            for (EGLConfig config : configs) 
            {
                int d = findConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0);
                int s = findConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);
                if ((d >= mDepthSize) && (s >= mStencilSize)) 
                {
                    int r = findConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0);
                    int g = findConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
                    int b = findConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
                    int a = findConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);
                    if ((r == mRedSize) && (g == mGreenSize) && (b == mBlueSize) && (a == mAlphaSize)) 
                    {
                        return config;
                    }
                }
            }
            return null;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display, EGLConfig config, int attribute, int defaultValue) 
        {
            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) 
            {
                return mValue[0];
            }
            return defaultValue;
        }

        private int[] mValue;
        // Subclasses can adjust these values:
        protected int mRedSize;
        protected int mGreenSize;
        protected int mBlueSize;
        protected int mAlphaSize;
        protected int mDepthSize;
        protected int mStencilSize;
    }

    /**
     * This class will choose a RGB_888 surface with
     * or without a depth buffer.
     *
     */
    private class SimpleEGLConfigChooser extends ComponentSizeChooser 
    {
        public SimpleEGLConfigChooser(boolean withDepthBuffer) 
        {
            super(8, 8, 8, 0, withDepthBuffer ? 16 : 0, 0);
        }
    }

    static class LogWriter extends Writer 
    {
        @Override
        public void close() 
        {
            flushBuilder();
        }

        @Override
        public void flush() 
        {
            flushBuilder();
        }

        @Override
        public void write(char[] buf, int offset, int count) 
        {
            for(int i = 0; i < count; i++) 
            {
                char c = buf[offset + i];
                if ( c == '\n') 
                {
                    flushBuilder();
                }
                else 
                {
                    mBuilder.append(c);
                }
            }
        }

        private void flushBuilder() 
        {
            if (mBuilder.length() > 0) 
            {
                Log.v("GLSurfaceView", mBuilder.toString());
                mBuilder.delete(0, mBuilder.length());
            }
        }

        private StringBuilder mBuilder = new StringBuilder();
    }

    static class GLThreadManager 
    {
        private static String TAG = "GLThreadManager";

        public synchronized void threadExiting(GLThread thread) 
        {
            if (LOG_THREADS) 
            {
                Log.i("GLThread", "exiting tid=" + Thread.currentThread().getId());
            }
            if (mEglOwner == thread) 
            {
                mEglOwner = null;
            }
            notifyAll();
        }

        /*
         * Tries once to acquire the right to use an EGL
         * context. Does not block. Requires that we are already
         * in the sGLThreadManager monitor when this is called.
         *
         * @return true if the right to use an EGL context was acquired.
         */
        public boolean tryAcquireEglContextLocked(GLThread thread) 
        {
            if (mEglOwner == thread || mEglOwner == null) 
            {
                mEglOwner = thread;
                notifyAll();
                return true;
            }
            checkGLESVersion();
            if (mMultipleGLESContextsAllowed) 
            {
                return true;
            }
            // Notify the owning thread that it should release the context.
            // TODO: implement a fairness policy. Currently
            // if the owning thread is drawing continuously it will just
            // reacquire the EGL context.
            if (mEglOwner != null) 
            {
                mEglOwner.requestReleaseEglContextLocked();
            }
            return false;
        }

        /*
         * Releases the EGL context. Requires that we are already in the
         * sGLThreadManager monitor when this is called.
         */
        public void releaseEglContextLocked(GLThread thread) 
        {
            if (mEglOwner == thread) {
                mEglOwner = null;
            }
            notifyAll();
        }

        public synchronized boolean shouldReleaseEGLContextWhenPausing()
        {
            // Release the EGL context when pausing even if
            // the hardware supports multiple EGL contexts.
            // Otherwise the device could run out of EGL contexts.
            return mLimitedGLESContexts;
        }

        public synchronized boolean shouldTerminateEGLWhenPausing() 
        {
            checkGLESVersion();
            return !mMultipleGLESContextsAllowed;
        }

        public synchronized void checkGLDriver(GL10 gl)
        {
            if (! mGLESDriverCheckComplete)
            {
                checkGLESVersion();
                String renderer = gl.glGetString(GL10.GL_RENDERER);
                if (mGLESVersion < kGLES_20) 
                {
                    mMultipleGLESContextsAllowed = ! renderer.startsWith(kMSM7K_RENDERER_PREFIX);
                    notifyAll();
                }
                mLimitedGLESContexts = !mMultipleGLESContextsAllowed;
                if (LOG_SURFACE)
                {
                    Log.w(TAG, "checkGLDriver renderer = \"" + renderer + "\" multipleContextsAllowed = " + mMultipleGLESContextsAllowed + " mLimitedGLESContexts = " + mLimitedGLESContexts);
                }
                mGLESDriverCheckComplete = true;
            }
        }

        private void checkGLESVersion() 
        {
            if (! mGLESVersionCheckComplete) 
            {
                try 
                {
                    Class<?> SP = Class.forName("android.os.SystemProperties");
                    mGLESVersion = (Integer)SP.getMethod("getInt", String.class, int.class).invoke(null, "ro.opengles.version", ConfigurationInfo.GL_ES_VERSION_UNDEFINED);
                    if (mGLESVersion >= kGLES_20) 
                    {
                        mMultipleGLESContextsAllowed = true;
                    }
                } 
                catch (Exception e)
                {
                }
                if (LOG_SURFACE)
                {
                    Log.w(TAG, "checkGLESVersion mGLESVersion =" + " " + mGLESVersion + " mMultipleGLESContextsAllowed = " + mMultipleGLESContextsAllowed);
                }
                mGLESVersionCheckComplete = true;
            }
        }

        /**
         * This check was required for some pre-Android-3.0 hardware. Android 3.0 provides
         * support for hardware-accelerated views, therefore multiple EGL contexts are
         * supported on all Android 3.0+ EGL drivers.
         */
        private boolean mGLESVersionCheckComplete;
        private int mGLESVersion;
        private boolean mGLESDriverCheckComplete;
        private boolean mMultipleGLESContextsAllowed;
        private boolean mLimitedGLESContexts;
        private static final int kGLES_20 = 0x20000;
        private static final String kMSM7K_RENDERER_PREFIX = "Q3Dimension MSM7500 ";
        private GLThread mEglOwner;
    }

    /**
     * A generic GL Thread. Takes care of initializing EGL and GL. Delegates
     * to a Renderer instance to do the actual drawing. Can be configured to
     * render continuously or on request.
     *
     * All potentially blocking synchronization is done through the
     * sGLThreadManager object. This avoids multiple-lock ordering issues.
     *
     */
    public static class GLThread
    {
        private final static boolean LOG_THREADS = false;
        private final static boolean LOG_SURFACE = false;
        private final static boolean LOG_PAUSE_RESUME = false;
        private final static boolean LOG_RENDERER = false;

        /**
         * Check glError() after every GL call and throw an exception if glError indicates
         * that an error has occurred. This can be used to help track down which OpenGL ES call
         * is causing an error.
         *
         * @see #getDebugFlags
         * @see #setDebugFlags
         */
        public final static int DEBUG_CHECK_GL_ERROR = 1;
        
        /**
         * Log GL calls to the system log at "verbose" level with tag "GLSurfaceView".
         *
         * @see #getDebugFlags
         * @see #setDebugFlags
         */
        public final static int DEBUG_LOG_GL_CALLS = 2;

        public GLThread(WeakReference<GameSurface> glSurfaceViewWeakRef, GameSurface.SurfaceInfo surfaceInfo)
        {
            super();
            glSurfaceViewWeakRef.get().mRequestRender = true;
            mGLSurfaceViewWeakRef = glSurfaceViewWeakRef;
            mSurfaceInfo = surfaceInfo;
        }

        public void ThreadStarting() 
        {
            Log.i("GLThread", "ThreadStarting: start tid=" + Thread.currentThread().getId());
            if (LOG_THREADS)
            {
                Log.i("GLThread", "starting tid=" + Thread.currentThread().getId());
            }
            GameSurface view = mGLSurfaceViewWeakRef.get();
            view.mHaveEglContext = false;
            view.mHaveEglSurface = false;
            ReadyToDraw();
            if (LOG_THREADS)
            {
                Log.i("GLThread", "starting tid=" + Thread.currentThread().getId());
            }
            Log.i("GLThread", "ThreadStarting: done tid=" + Thread.currentThread().getId());
        }

        public void ThreadExiting()
        {
            Log.i("GLThread", "ThreadExiting: start tid=" + Thread.currentThread().getId());
            /*
             * clean-up everything...
             */
            GameSurface.sGLThreadManager.threadExiting(this);
            GameSurface view = mGLSurfaceViewWeakRef.get();
            view.mExited = true;
            synchronized (GameSurface.sGLThreadManager) 
            {
                stopEglSurfaceLocked();
                stopEglContextLocked();
            }
            Log.i("GLThread", "ThreadExiting: done tid=" + Thread.currentThread().getId());
        }

        /*
         * This private method should only be called inside a
         * synchronized(sGLThreadManager) block.
         */
        private void stopEglSurfaceLocked() 
        {
            GameSurface view = mGLSurfaceViewWeakRef.get();
            if (view.mHaveEglSurface) 
            {
                view.mHaveEglSurface = false;
                view.mEglHelper.destroySurface();
            }
        }

        /*
         * This private method should only be called inside a
         * synchronized(sGLThreadManager) block.
         */
        private void stopEglContextLocked() 
        {
            GameSurface view = mGLSurfaceViewWeakRef.get();
            if (view.mHaveEglContext) 
            {
                view.mEglHelper.finish();
                view.mHaveEglContext = false;
                GameSurface.sGLThreadManager.releaseEglContextLocked(this);
            }
        }

        public void ReadyToDraw() 
        {
            GameSurface view = mGLSurfaceViewWeakRef.get();
            try
            {
                synchronized (GameSurface.sGLThreadManager) 
                {
                    while (true) 
                    {
                        if (mShouldExit) 
                        {
                            return;
                        }

                        if (! mEventQueue.isEmpty()) 
                        {
                            mEvent = mEventQueue.remove(0);
                            break;
                        }

                        // Update the pause state.
                        boolean pausing = false;
                        if (view.mPaused != mRequestPaused) 
                        {
                            pausing = mRequestPaused;
                            view.mPaused = mRequestPaused;
                            GameSurface.sGLThreadManager.notifyAll();
                            if (LOG_PAUSE_RESUME) 
                            {
                                Log.i("GLThread", "mPaused is now " + view.mPaused + " tid=" + Thread.currentThread().getId());
                            }
                        }

                        // Do we need to give up the EGL context?
                        if (mShouldReleaseEglContext) 
                        {
                            if (LOG_SURFACE) 
                            {
                                Log.i("GLThread", "releasing EGL context because asked to tid=" + Thread.currentThread().getId());
                            }
                            stopEglSurfaceLocked();
                            stopEglContextLocked();
                            mShouldReleaseEglContext = false;
                            mAskedToReleaseEglContext = true;
                        }

                        // Have we lost the EGL context?
                        if (mLostEglContext) 
                        {
                            stopEglSurfaceLocked();
                            stopEglContextLocked();
                            mLostEglContext = false;
                        }

                        // When pausing, release the EGL surface:
                        if (pausing && view.mHaveEglSurface) 
                        {
                            if (LOG_SURFACE) 
                            {
                                Log.i("GLThread", "releasing EGL surface because paused tid=" + Thread.currentThread().getId());
                            }
                            stopEglSurfaceLocked();
                        }

                        // When pausing, optionally release the EGL Context:
                        if (pausing && view.mHaveEglContext) 
                        {
                            boolean preserveEglContextOnPause = view == null ? false : view.mPreserveEGLContextOnPause;
                            if (!preserveEglContextOnPause || GameSurface.sGLThreadManager.shouldReleaseEGLContextWhenPausing()) 
                            {
                                stopEglContextLocked();
                                if (LOG_SURFACE) 
                                {
                                    Log.i("GLThread", "releasing EGL context because paused tid=" + Thread.currentThread().getId());
                                }
                            }
                        }

                        // When pausing, optionally terminate EGL:
                        if (pausing) 
                        {
                            if (GameSurface.sGLThreadManager.shouldTerminateEGLWhenPausing()) 
                            {
                                view.mEglHelper.finish();
                                if (LOG_SURFACE) 
                                {
                                    Log.i("GLThread", "terminating EGL because paused tid=" + Thread.currentThread().getId());
                                }
                            }
                        }

                        // Have we lost the SurfaceView surface?
                        if ((!view.mHasSurface) && (!view.mWaitingForSurface)) 
                        {
                            if (LOG_SURFACE) 
                            {
                                Log.i("GLThread", "noticed surfaceView surface lost tid=" + Thread.currentThread().getId());
                            }
                            if (view.mHaveEglSurface) 
                            {
                                stopEglSurfaceLocked();
                            }
                            view.mWaitingForSurface = true;
                            view.mSurfaceIsBad = false;
                            GameSurface.sGLThreadManager.notifyAll();
                        }

                        // Have we acquired the surface view surface?
                        if (view.mHasSurface && view.mWaitingForSurface) 
                        {
                            if (LOG_SURFACE) 
                            {
                                Log.i("GLThread", "noticed surfaceView surface acquired tid=" + Thread.currentThread().getId());
                            }
                            view.mWaitingForSurface = false;
                            GameSurface.sGLThreadManager.notifyAll();
                        }

                        if (mDoRenderNotification) 
                        {
                            if (LOG_SURFACE) 
                            {
                                Log.i("GLThread", "sending render notification tid=" + Thread.currentThread().getId());
                            }
                            mWantRenderNotification = false;
                            mDoRenderNotification = false;
                            view.mRenderComplete = true;
                            GameSurface.sGLThreadManager.notifyAll();
                        }

                        // Ready to draw?
                        if (view.readyToDraw()) 
                        {
                            // If we don't have an EGL context, try to acquire one.
                            if (!view.mHaveEglContext) 
                            {
                                if (mAskedToReleaseEglContext) 
                                {
                                    mAskedToReleaseEglContext = false;
                                }
                                else if (GameSurface.sGLThreadManager.tryAcquireEglContextLocked(this)) 
                                {
                                    try 
                                    {
                                        view.mEglHelper.start();
                                    }
                                    catch (RuntimeException t) 
                                    {
                                        GameSurface.sGLThreadManager.releaseEglContextLocked(this);
                                        throw t;
                                    }
                                    view.mHaveEglContext = true;
                                    mCreateEglContext = true;

                                    GameSurface.sGLThreadManager.notifyAll();
                                }
                            }

                            if (view.mHaveEglContext && !view.mHaveEglSurface) 
                            {
                                view.mHaveEglSurface = true;
                                mCreateEglSurface = true;
                                mCreateGlInterface = true;
                                mSizeChanged = true;
                            }

                            if (view.mHaveEglSurface) 
                            {
                                if (view.mSizeChanged) 
                                {
                                    mSizeChanged = true;
                                    mW = view.mWidth;
                                    mH = view.mHeight;
                                    mWantRenderNotification = true;
                                    if (LOG_SURFACE) 
                                    {
                                        Log.i("GLThread", "noticing that we want render notification tid=" + Thread.currentThread().getId());
                                    }

                                    // Destroy and recreate the EGL surface.
                                    mCreateEglSurface = true;

                                    view.mSizeChanged = false;
                                }
                                view.mRequestRender = false;
                                GameSurface.sGLThreadManager.notifyAll();
                                break;
                            }
                        }

                        // By design, this is the only place in a GLThread thread where we wait().
                        if (LOG_THREADS) 
                        {
                            Log.i("GLThread", "waiting tid=" + Thread.currentThread().getId()
                                + " mHaveEglContext: " + view.mHaveEglContext
                                + " mHaveEglSurface: " + view.mHaveEglSurface
                                + " mFinishedCreatingEglSurface: " + view.mFinishedCreatingEglSurface
                                + " mPaused: " + view.mPaused
                                + " mHasSurface: " + view.mHasSurface
                                + " mSurfaceIsBad: " + view.mSurfaceIsBad
                                + " mWaitingForSurface: " + view.mWaitingForSurface
                                + " mWidth: " + view.mWidth
                                + " mHeight: " + view.mHeight
                                + " mRequestRender: " + view.mRequestRender);
                        }

                        try 
                        {
                            GameSurface.sGLThreadManager.wait();
                        }
                        catch (InterruptedException e) 
                        {
                            // fall thru and exit normally
                        }
                    }
                } // end of synchronized(sGLThreadManager)

                if (mEvent != null) 
                {
                    mEvent.run();
                    mEvent = null;
                    ReadyToDraw();
                    return;
                }

                if (mCreateEglSurface) 
                {
                    if (LOG_SURFACE) 
                    {
                        Log.w("GLThread", "egl createSurface");
                    }
                    if (view.mEglHelper.createSurface()) 
                    {
                        synchronized(GameSurface.sGLThreadManager) 
                        {
                            view.mFinishedCreatingEglSurface = true;
                            GameSurface.sGLThreadManager.notifyAll();
                        }
                    } 
                    else 
                    {
                        synchronized(GameSurface.sGLThreadManager) 
                        {
                            view.mFinishedCreatingEglSurface = true;
                            view.mSurfaceIsBad = true;
                            GameSurface.sGLThreadManager.notifyAll();
                        }
                        ReadyToDraw();
                        return;
                    }
                    mCreateEglSurface = false;
                }

                if (mCreateGlInterface) 
                {
                    mGl = (GL10) view.mEglHelper.createGL();

                    GameSurface.sGLThreadManager.checkGLDriver(mGl);
                    mCreateGlInterface = false;
                }

                if (mCreateEglContext) 
                {
                    if (LOG_RENDERER) 
                    {
                        Log.w("GLThread", "onSurfaceCreated");
                    }
                    if (view != null) 
                    {
                        try 
                        {
                            //Trace.traceBegin(Trace.TRACE_TAG_VIEW, "onSurfaceCreated");
                            mSurfaceInfo.onSurfaceCreated(mGl, view.mEglHelper.mEglConfig);
                        }
                        finally 
                        {
                            //Trace.traceEnd(Trace.TRACE_TAG_VIEW);
                        }
                    }
                    mCreateEglContext = false;
                }

                if (mSizeChanged) 
                {
                    if (LOG_RENDERER) 
                    {
                        Log.w("GLThread", "onSurfaceChanged(" + mW + ", " + mH + ")");
                    }
                    if (view != null) 
                    {
                        try 
                        {
                            //Trace.traceBegin(Trace.TRACE_TAG_VIEW, "onSurfaceChanged");
                            mSurfaceInfo.onSurfaceChanged(mGl, mW, mH);
                        }
                        finally 
                        {
                            //Trace.traceEnd(Trace.TRACE_TAG_VIEW);
                        }
                    }
                    mSizeChanged = false;
                }
            }
            finally 
            {
                /*
                 * clean-up everything...
                 */
            }
        }

        public void SwapBuffers()
        {
            GameSurface view = mGLSurfaceViewWeakRef.get();
            int swapError = view.mEglHelper.swap();
            switch (swapError) 
            {
                case EGL10.EGL_SUCCESS:
                    break;
                case EGL11.EGL_CONTEXT_LOST:
                    if (LOG_SURFACE) 
                    {
                        Log.i("GLThread", "egl context lost tid=" + Thread.currentThread().getId());
                    }
                    mLostEglContext = true;
                    break;
                default:
                    // Other errors typically mean that the current surface is bad,
                    // probably because the SurfaceView surface has been destroyed,
                    // but we haven't been notified yet.
                    // Log the error to help developers understand why rendering stopped.
                    EglHelper.logEglErrorAsWarning("GLThread", "eglSwapBuffers", swapError);

                    synchronized(GameSurface.sGLThreadManager) 
                    {
                        view.mSurfaceIsBad = true;
                        GameSurface.sGLThreadManager.notifyAll();
                    }
                    break;
            }

            if (mWantRenderNotification) 
            {
                mDoRenderNotification = true;
            }
            ReadyToDraw();
        }
        
        public void requestRender() 
        {
            GameSurface view = mGLSurfaceViewWeakRef.get();
            synchronized(GameSurface.sGLThreadManager) 
            {
                view.mRequestRender = true;
                GameSurface.sGLThreadManager.notifyAll();
            }
        }

        public void onPause() 
        {
            Log.i("GLThread", "onPause start");
           GameSurface view = mGLSurfaceViewWeakRef.get();
            synchronized (GameSurface.sGLThreadManager) 
            {
                if (LOG_PAUSE_RESUME) 
                {
                    Log.i("GLThread", "onPause tid=" + Thread.currentThread().getId());
                }
                mRequestPaused = true;
                GameSurface.sGLThreadManager.notifyAll();
                while ((! view.mExited) && (! view.mPaused)) 
                {
                    if (LOG_PAUSE_RESUME) 
                    {
                        Log.i("Main thread", "onPause waiting for mPaused.");
                    }
                    try 
                    {
                        GameSurface.sGLThreadManager.wait();
                    }
                    catch (InterruptedException ex) 
                    {
                        Thread.currentThread().interrupt();
                    }
                }
            }
            Log.i("GLThread", "onPause done");
        }

        public void onResume() 
        {
            Log.i("GLThread", "onResume start");
            GameSurface view = mGLSurfaceViewWeakRef.get();
            synchronized (GameSurface.sGLThreadManager) 
            {
                if (LOG_PAUSE_RESUME) 
                {
                    Log.i("GLThread", "onResume tid=" + Thread.currentThread().getId());
                }
                mRequestPaused = false;
                view.mRequestRender = true;
                view.mRenderComplete = false;
                GameSurface.sGLThreadManager.notifyAll();
                while ((!view.mExited) && view.mPaused && (!view.mRenderComplete)) 
                {
                    if (LOG_PAUSE_RESUME) 
                    {
                        Log.i("Main thread", "onResume waiting for !mPaused.");
                    }
                    try 
                    {
                        GameSurface.sGLThreadManager.wait();
                    }
                    catch (InterruptedException ex) 
                    {
                        Thread.currentThread().interrupt();
                    }
                }
            }
            Log.i("GLThread", "onResume Done");
        }

        public void requestExitAndWait() 
        {
            Log.i("GLThread", "requestExitAndWait start");
           // don't call this from GLThread thread or it is a guaranteed
            // deadlock!
            GameSurface view = mGLSurfaceViewWeakRef.get();
            synchronized(GameSurface.sGLThreadManager) 
            {
                mShouldExit = true;
                GameSurface.sGLThreadManager.notifyAll();
                while (!view.mExited) 
                {
                    try 
                    {
                        GameSurface.sGLThreadManager.wait();
                    } 
                    catch (InterruptedException ex) 
                    {
                        Thread.currentThread().interrupt();
                    }
                }
            }
            Log.i("GLThread", "requestExitAndWait exit");
        }

        public void requestReleaseEglContextLocked() 
        {
            mShouldReleaseEglContext = true;
            GameSurface.sGLThreadManager.notifyAll();
        }

        /**
         * Queue an "mEvent" to be run on the GL rendering thread.
         * @param r the runnable to be run on the GL rendering thread.
         */
        public void queueEvent(Runnable r) 
        {
            Log.i("GLThread", "queueEvent start");
           if (r == null) 
            {
                throw new IllegalArgumentException("r must not be null");
            }
            synchronized(GameSurface.sGLThreadManager) 
            {
                mEventQueue.add(r);
                GameSurface.sGLThreadManager.notifyAll();
            }
            Log.i("GLThread", "queueEvent Done");
       }

        // Once the thread is started, all accesses to the following member
        // variables are protected by the sGLThreadManager monitor
        private boolean mShouldExit;
        private boolean mRequestPaused;
        private boolean mShouldReleaseEglContext;
        private GameSurface.SurfaceInfo mSurfaceInfo;
        private ArrayList<Runnable> mEventQueue = new ArrayList<Runnable>();

        // End of member variables protected by the sGLThreadManager monitor.

        /**
         * Set once at thread construction time, nulled out when the parent view is garbage
         * called. This weak reference allows the GLSurfaceView to be garbage collected while
         * the GLThread is still alive.
         */
        private WeakReference<GameSurface> mGLSurfaceViewWeakRef;

        GL10 mGl = null;
        boolean mCreateEglContext = false;
        boolean mCreateEglSurface = false;
        boolean mCreateGlInterface = false;
        boolean mLostEglContext = false;
        boolean mSizeChanged = false;
        boolean mWantRenderNotification = false;
        boolean mDoRenderNotification = false;
        boolean mAskedToReleaseEglContext = false;
        int mW = 0;
        int mH = 0;
        Runnable mEvent = null;
    }

    static final GLThreadManager sGLThreadManager = new GLThreadManager();

    EGLConfigChooser mEGLConfigChooser;
    EGLContextFactory mEGLContextFactory;
    EGLWindowSurfaceFactory mEGLWindowSurfaceFactory;
    GLWrapper mGLWrapper;
    int mDebugFlags;
    int mEGLContextClientVersion;
    boolean mPreserveEGLContextOnPause;
    int mWidth;
    int mHeight;
    boolean mSizeChanged = true;
    boolean mRequestRender;
    boolean mRenderComplete;
    boolean mExited;
    boolean mPaused;
    boolean mHasSurface;
    boolean mSurfaceIsBad;
    boolean mHaveEglSurface;
    boolean mWaitingForSurface;
    boolean mFinishedCreatingEglSurface;
    boolean mHaveEglContext;
    EglHelper mEglHelper;
}