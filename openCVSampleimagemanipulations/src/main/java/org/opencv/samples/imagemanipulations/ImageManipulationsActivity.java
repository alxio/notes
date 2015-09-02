package org.opencv.samples.imagemanipulations;

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
import android.view.WindowManager;

public class ImageManipulationsActivity extends Activity implements CvCameraViewListener2 {
    private static final String TAG = "OCVSample::Activity";
    private CameraBridgeViewBase mOpenCvCameraView;

    private Mat mIntermediateMat;
    private Mat mTransposed;
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
    double[] gray = {230};

    public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        Mat gray = inputFrame.gray();
        Size sizeGray = gray.size();
        Mat grayInner;
        int rows = (int) sizeGray.height;
        int cols = (int) sizeGray.width;
        int left = cols / 16;
        int top = rows / 16;
        int width = cols * 7 / 8;
        int height = rows * 7 / 8;
        grayInner = gray.submat(top, top + height, left, left + width);

        Imgproc.adaptiveThreshold(grayInner, mIntermediateMat, 255, Imgproc.ADAPTIVE_THRESH_MEAN_C, Imgproc.THRESH_BINARY, 25, 5.0);
        Core.transpose(mIntermediateMat, mTransposed);

        int white = 0;
        int best = 0;
        Arrays.fill(count, 0);
        for (int i = 0; i < mTransposed.rows(); ++i) {
            if (mRandom.nextInt(100) > 0) continue;
            white = 0;
            for (int j = 0; j < mTransposed.cols(); ++j) {
                if (mTransposed.get(i, j)[0] > 0) {
                    ++white;
                } else {
                    if (white > MIN && white < MAX) {
                        ++count[white];
                    }
                    white = 0;
                }
            }
        }
        for (int i = MIN; i < MAX; ++i) {
            if (count[i] > count[best]) best = i;
        }
        Log.e(TAG, "NDK: " + computeLineHeight(mTransposed.dataAddr(),
                mTransposed.cols(), mTransposed.rows()));
        Log.e(TAG, "Best: " + best);

        colorizeLine(mTransposed.dataAddr(), mTransposed.cols(), mTransposed.rows(), best);

        /*
        for (int i = 0; i < mTransposed.rows(); ++i) {
            white = 0;
            for (int j = 0; j < mTransposed.cols(); ++j) {
                if (mTransposed.get(i, j)[0] > 0) {
                    ++white;
                } else {
                    if (white == best) {
                        for (int k = white; k > 0; --k) {
                            mTransposed.put(i, j - k, 200.0);
                        }
                    }
                    white = 0;
                }
            }
        }
        */
        Core.transpose(mTransposed, mIntermediateMat);
        mIntermediateMat.copyTo(grayInner);
        return gray;
    }

    public native int computeLineHeight(long nativeObject, int w, int h);

    public native int colorizeLine(long nativeObject, int w, int h, int lineHeight);
}
