package org.opencv.samples.imagemanipulations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Random;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.MatOfFloat;
import org.opencv.core.MatOfInt;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.imgproc.Imgproc;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

public class ImageManipulationsActivity extends Activity implements CvCameraViewListener2 {
    private static final String TAG = "OCVSample::Activity";
    private CameraBridgeViewBase mOpenCvCameraView;

    private Mat mIntermediateMat;
    private Mat mTransposed;
    private Mat mGray;
    private Random mRandom = new Random();

    static {
        System.loadLibrary("myNativeLib");
    }

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS: {
                    Log.i(TAG, "OpenCV loaded successfully");
                    mOpenCvCameraView.enableView();
                }
                break;
                default: {
                    super.onManagerConnected(status);
                }
                break;
            }
        }
    };

    public ImageManipulationsActivity() {
        Log.i(TAG, "Instantiated new " + this.getClass());
    }

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "called onCreate");
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.image_manipulations_surface_view);

        mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.image_manipulations_activity_surface_view);
        mOpenCvCameraView.setCvCameraViewListener(this);
        mOpenCvCameraView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (playing) {
                    mPlayer.stop();
                    playing = false;
                } else {
                    canPlay = true;
                }
            }
        });
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    @Override
    public void onResume() {
        super.onResume();
        if (!OpenCVLoader.initDebug()) {
            Log.d(TAG, "Internal OpenCV library not found. Using OpenCV Manager for initialization");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_3_0_0, this, mLoaderCallback);
        } else {
            Log.d(TAG, "OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        }
    }

    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    public void onCameraViewStarted(int width, int height) {
        mIntermediateMat = new Mat();
        mTransposed = new Mat();
    }

    public void onCameraViewStopped() {
        if (mIntermediateMat != null)
            mIntermediateMat.release();

        mIntermediateMat = null;

        if (mTransposed != null)
            mTransposed.release();

        mTransposed = null;
    }

    final int MAX = 32;
    final int MIN = 2;
    int[] count = new int[MAX];

    private boolean isProcessing = false;

    public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        boolean cantProcess = false;
        synchronized (this) {
            if (isProcessing) {
                cantProcess = true;
            } else {
                isProcessing = true;
            }
        }
        if (cantProcess) return null;
        if (!playing) {
            mGray = inputFrame.gray();
            Imgproc.adaptiveThreshold(mGray, mIntermediateMat, 255, Imgproc.ADAPTIVE_THRESH_MEAN_C, Imgproc.THRESH_BINARY, 25, 5.0);
            Imgproc.dilate(mIntermediateMat, mGray, Imgproc.getStructuringElement(Imgproc.MORPH_RECT, new Size(2, 2)));
            Imgproc.erode(mGray, mIntermediateMat, Imgproc.getStructuringElement(Imgproc.MORPH_RECT, new Size(2, 2)));
            Core.transpose(mIntermediateMat, mTransposed);
            int rows = computeLineHeight(mTransposed.dataAddr(), mIntermediateMat.dataAddr(), mTransposed.cols(), mTransposed.rows());
            Log.e(TAG, "NDK: " + rows);
            readData();
        } else {
            long now = System.currentTimeMillis();
            float time = 0.001f * (now - mPlaybackStarted);
            int i = -1;
            while (time > 0) {
                if (++i >= mSounds.size()) break;
                time -= mSounds.get(i).length * Sound.basicLen;
            }
            colorizeLine(i);
            if (i == mSounds.size() - 1) playing = false;
        }
        Core.transpose(mTransposed, mGray);
        isProcessing = false;
        return mGray;
    }

    boolean playing = false;
    boolean canPlay = false;

    Player mPlayer = new Player();
    ArrayList<Sound> mSounds = new ArrayList<>();
    long mPlaybackStarted = 0;

    private void readData() {
        ArrayList<Byte> arr = new ArrayList<>();
        byte[] len = new byte[2];
        byte[] tmp = new byte[2];
        mTransposed.get(0, 0, len);
        int l = len[0] * 128 + len[1];
        Log.e(TAG, "NOTES COUNT: " + l);
        for (int i = 2; i / 2 < l; i += 2) {
            mTransposed.get(0, i, tmp);
            arr.add(tmp[0]);
            arr.add(tmp[1]);
        }
        if (canPlay && !playing) {
            playing = true;
            canPlay = false;
            mSounds.clear();
            for (int i = 0; i < arr.size(); i += 2) {
                mSounds.add(new Sound(arr.get(i), arr.get(i + 1)));
            }
            short[] data = Sound.generateSound(mSounds);
            mPlayer.play(data);
            mPlaybackStarted = System.currentTimeMillis();
        }
        Log.e(TAG, "NOTES: " + arr.toString());
    }

    public native int computeLineHeight(long nativeObject, long nativeObject2, int w, int h);

    public native int colorizeLine(int currentSound);
}
