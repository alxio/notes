package org.opencv.samples.musicrecognition;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class Player {
    private final int SAMPLE_RATE = 16000;
    private AudioTrack mTrack = null;

    public boolean play(short[] sounds) {
        return play(sounds, SAMPLE_RATE);
    }

    /**
     * Play sounds array with given sample rate
     *
     * @param sounds array of sound values
     * @param rate   sample rate in Hz
     */
    public boolean play(short[] sounds, int rate) {
        synchronized (this) {
            if (mTrack != null) return false;
            try {
                mTrack = new AudioTrack(AudioManager.STREAM_MUSIC, rate,
                        AudioFormat.CHANNEL_OUT_MONO,
                        AudioFormat.ENCODING_PCM_16BIT, sounds.length * 2,
                        AudioTrack.MODE_STATIC);
                mTrack.write(sounds, 0, sounds.length);
                mTrack.play();
            } catch (RuntimeException e) {
                e.printStackTrace();
                return false;
            }
        }
        return true;
    }

    public boolean stop() {
        synchronized (this) {
            if (mTrack != null) {
                mTrack.flush();
                mTrack.stop();
                mTrack.release();
                mTrack = null;
                return true;
            }
            return false;
        }
    }

    public boolean isPlaying() {
        return mTrack != null;
    }
}