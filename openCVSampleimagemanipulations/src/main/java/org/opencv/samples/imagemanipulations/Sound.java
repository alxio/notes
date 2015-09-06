package org.opencv.samples.imagemanipulations;

import java.util.Collection;
import java.util.Random;

/**
 * Created by ALX on 2015-09-06.
 */
public class Sound {
    byte length = 0;
    byte height = 0;

    private static Random R = new Random();
    private static final int SAMPLE_RATE = 16000;
    private static float basicLen = 0.125f;
    private static float[] freqs = {32.70f, 36.71f, 41.20f, 43.65f, 49.00f, 55.00f, 61.74f, 65.41f, 73.42f, 82.41f, 87.31f, 98.00f, 110.00f};

    public static short[] generateSound(Collection<Sound> collection) {
        float len = 0;
        for (Sound s : collection) {
            len += s.length;
        }
        short[] data = new short[(int) (SAMPLE_RATE * len * basicLen)];
        int offset = 0;
        for (Sound s : collection) {
            offset += genTone(getFreq(s.height), s.length * basicLen, data, offset);
        }
        return data;
    }

    static float getFreq(byte i) {
        return freqs[i + 6];
    }

    private static int genTone(float freq, float duration, short[] buffer, int offset) {
        float angle = 0;
        float angular_frequency = (float) (2 * Math.PI) * freq / SAMPLE_RATE;
        for (int i = offset; i < offset + SAMPLE_RATE * duration; i++) {
            buffer[i] = (short) (Short.MAX_VALUE * ((float) Math.sin(angle)));
            angle += angular_frequency;
        }
        return (int) (SAMPLE_RATE * duration);
    }

    public static Sound random() {
        Sound s = new Sound();
        s.length = (byte) R.nextInt(9);
        s.height = (byte) (R.nextInt(13) - 6);
        return s;
    }
}
