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

    public synchronized Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        mGray = inputFrame.gray();
        Imgproc.adaptiveThreshold(mGray, mIntermediateMat, 255, Imgproc.ADAPTIVE_THRESH_MEAN_C, Imgproc.THRESH_BINARY, 25, 5.0);
        Imgproc.dilate(mIntermediateMat, mGray, Imgproc.getStructuringElement(Imgproc.MORPH_RECT, new Size(1.5, 1.5)));
        Core.transpose(mGray, mTransposed);

        colorizeLine(mGray.dataAddr(), mGray.cols(), mGray.rows(), 0);
        int rows = computeLineHeight(mTransposed.dataAddr(), mTransposed.cols(), mTransposed.rows());
        Log.e(TAG, "NDK: " + rows);
        readData();
        Core.transpose(mTransposed, mGray);

        return mGray;
    }

    private void readData() {
        ArrayList<Integer> arr = new ArrayList<>();
        byte[] len = new byte[1];
        byte[] tmp = new byte[2];
        mTransposed.get(0, 0, len);
        for (int i = 1; i / 2 < len[0]; i += 2) {
            mTransposed.get(0, i, tmp);
            arr.add((int) tmp[0]);
            arr.add((int) tmp[1]);
        }
        Log.e(TAG, "NOTES: " + arr.toString() + " " + arr.toString());
    }

    public native int computeLineHeight(long nativeObject, int w, int h);

    public native int colorizeLine(long nativeObject, int w, int h, int lineHeight);
}
