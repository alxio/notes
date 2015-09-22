package org.opencv.samples.musicrecognition;

import java.util.ArrayList;
import java.util.Random;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.Mat;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.imgproc.Imgproc;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;

public class ImageManipulationsActivity extends Activity implements CvCameraViewListener2 {
    private static final String TAG = "OCVSample::Activity";
    private CameraBridgeViewBase mOpenCvCameraView;

    private Mat mIntermediateMat;
    private Mat mTransposed;
    private Mat mGray;
    private Mat mOriginal;
    private Mat mImage;
    private Mat mColor;
    private Mat mMask;

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
        mFreezeButton = (Button) findViewById(R.id.freeze);
        mFreezeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setFrozen(!mFrozen);
            }
        });
        mPlayButton = (Button) findViewById(R.id.play);
        mPlayButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setPlaying(!mPlaying);
            }
        });
        mPlayButton.setEnabled(false);
        mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.image_manipulations_activity_surface_view);
        mOpenCvCameraView.setCvCameraViewListener(this);
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
        mImage = new Mat();
        mColor = new Mat();
        mMask = new Mat();
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
        if (!mFrozen) {
            mOriginal = inputFrame.gray().clone();
            mGray = mOriginal.clone();
            Imgproc.adaptiveThreshold(mGray, mIntermediateMat, 255, Imgproc.ADAPTIVE_THRESH_MEAN_C, Imgproc.THRESH_BINARY, 25, 5.0);
            Imgproc.dilate(mIntermediateMat, mGray, Imgproc.getStructuringElement(Imgproc.MORPH_RECT, new Size(2, 2)));
            Imgproc.erode(mGray, mIntermediateMat, Imgproc.getStructuringElement(Imgproc.MORPH_RECT, new Size(2, 2)));
            Core.transpose(mIntermediateMat, mTransposed);
            int status = computeLineHeight(mTransposed.dataAddr(), mIntermediateMat.dataAddr(), mTransposed.cols(), mTransposed.rows());
            Log.e(TAG, "NDK: " + status);
            readData();
        } else {
            long now = System.currentTimeMillis();
            int i = -1;
            if (mPlaying) synchronized (mSounds) {
                float time = 0.001f * (now - mPlaybackStarted);
                while (time > 0) {
                    if (++i >= mSounds.size()) break;
                    time -= mSounds.get(i).length * Sound.basicLen;
                }
                if (i == mSounds.size() - 1) setPlaying(false);
            }
            colorizeLine(i);
        }
        Core.transpose(mTransposed, mGray);
        Core.inRange(mGray, Scalar.all(31), Scalar.all(33), mMask);
        Core.addWeighted(mOriginal, 0.75, mGray, 0.25, 1.0, mImage);
        Imgproc.cvtColor(mImage, mColor, Imgproc.COLOR_GRAY2RGBA);
        mColor.setTo(new Scalar(255, 0, 0, 255), mMask);
        isProcessing = false;
        return mColor;
    }

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
        //Log.e(TAG, "NOTES: " + arr.toString());
        synchronized (mSounds) {
            mSounds.clear();
            for (int i = 0; i < arr.size(); i += 2) {
                mSounds.add(new Sound(arr.get(i), arr.get(i + 1)));
            }
        }
    }

    public native int computeLineHeight(long nativeObject, long nativeObject2, int w, int h);

    public native int colorizeLine(int currentSound);


    private Button mFreezeButton;
    private Button mPlayButton;
    private boolean mFrozen = false;
    private boolean mPlaying = false;


    private void setText(final CharSequence text, final Button button) {
        button.post(new Runnable() {
            public void run() {
                button.setText(text);
            }
        });
    }

    private void setPlaying(boolean value) {
        if (value == false) {
            mPlayer.stop();
            mPlaying = false;
            setText("Play", mPlayButton);
        } else {
            if (mFrozen == true) {
                synchronized (mSounds) {
                    short[] data = Sound.generateSound(mSounds);
                    if (mPlayer.play(data)) {
                        mPlaybackStarted = System.currentTimeMillis();
                        mPlaying = true;
                        setText("Stop", mPlayButton);
                    }
                }
            }
        }
    }

    private void setFrozen(final boolean value) {
        if (value == false) {
            setPlaying(false);
            setText("Freeze", mFreezeButton);
        } else {
            setText("Unfreeze", mFreezeButton);
        }
        mPlayButton.post(new Runnable() {
            public void run() {
                mPlayButton.setEnabled(value);
            }
        });
        mFrozen = value;
    }
}
