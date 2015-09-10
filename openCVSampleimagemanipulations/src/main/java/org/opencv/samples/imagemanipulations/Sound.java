package org.opencv.samples.imagemanipulations;

import java.util.Collection;
import java.util.Random;

/**
 * Created by ALX on 2015-09-06.
 */
public class Sound {
    public Sound() {
    }

    public Sound(byte h, byte l) {
        height = h;
        length = l;
    }

    byte length = 0;
    byte height = 0;

    private static Random R = new Random();
    private static final int SAMPLE_RATE = 16000;
    public static float basicLen = 0.125f;
    private static float[] freqs = {246.94f, 261.63f, 293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f, 523.25f, 587.33f, 659.25f, 698.46f, 783.99f, 880.00f, 932.33f};

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
        return freqs[i];
    }

    private static int genTone(float freq, float duration, short[] buffer, int offset) {
        float angle = 0;
        float delta = (float) (2 * Math.PI) * freq / SAMPLE_RATE;
        for (int i = 0; i < SAMPLE_RATE * duration; i++) {
            float progress = (float) (Math.sin((Math.PI * i) / (SAMPLE_RATE * duration)));
            buffer[offset + i] = (short) (Short.MAX_VALUE * Math.sin(angle) * progress);
            angle += delta;
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
