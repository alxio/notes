package org.opencv.samples.imagemanipulations;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class Player {
    private final int SAMPLE_RATE = 16000;
    private AudioTrack mTrack;

    public void play(short[] sounds) {
        play(sounds, SAMPLE_RATE);
    }

    /**
     * Play sounds array with given sample rate
     *
     * @param sounds array of sound values
     * @param rate   sample rate in Hz
     */
    public void play(short[] sounds, int rate) {
        if (mTrack != null) {
            mTrack.stop();
            mTrack.release();
        }
        try {
            mTrack = new AudioTrack(AudioManager.STREAM_MUSIC, rate,
                    AudioFormat.CHANNEL_OUT_MONO,
                    AudioFormat.ENCODING_PCM_16BIT, sounds.length * 2,
                    AudioTrack.MODE_STATIC);
            mTrack.write(sounds, 0, sounds.length);
            mTrack.play();
        } catch (RuntimeException e) {
            e.printStackTrace();
        }
    }
}