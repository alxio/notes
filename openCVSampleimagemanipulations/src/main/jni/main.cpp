#include "org_opencv_samples_imagemanipulations_ImageManipulationsActivity.h"

#include <vector>
#include <set>
#include <utility>

using namespace std;
typedef unsigned char *imageptr;

const int MAX = 32;
const int MIN = 2;
const int SKIP = 100;
const int TOLERANCE = 1;

int counters[MAX];
int lineHeight;
int avgLines;

#define NEXTX first.first
#define NEXTY first.second
#define FIRSTX second.first
#define FIRSTY second.second
#define X first
#define Y second
typedef pair<pair<int, int>, pair<int, int> > line;
#define LINE(a, b) make_pair(make_pair(a,b), make_pair(a,b))

set<line> lines;

void findLines(imageptr bmp, int w, int h) {
    double direction = 1.0;
    int i = 0;
    imageptr ptr = bmp;
    int white = 0;
    int count = 0;

    //choose initial position (original x, transposed y)
    for (i = 0; i < h; i += lineHeight) {
        ptr += lineHeight * w;
        for (int j = 0; j < w; ++j) {
            if (*ptr++ > 0) {
                ++white;
            } else {
                if (abs(white - lineHeight) < TOLERANCE) {
                    ++count;
                }
                white = 0;
            }
        }
        if (count > avgLines)break;
        count = 0;
    }

    lines.clear();
    line dummy;
    dummy.first = dummy.second = make_pair(0, 0);
    white = 0;

    float asum;
    float acount;

    for (; i < h; i += lineHeight) {//real x, transposed y
        asum = acount = 0;
        for (int j = 0; j < w; ++j) {//real y, transposed x
            if (*ptr++ > 0) {
                ++white;
            } else {
                if (abs(white - lineHeight) < TOLERANCE) {
                    dummy.FIRSTX = j - lineHeight + 1;
                    set<line>::iterator it = lines.find(dummy);
                    if (it == lines.end() || it->FIRSTX > j + lineHeight - 1) {
                        lines.insert(LINE(j, i));
                    } else {
                        pair<int, int> old = it->first;
                        float a = (old.X - j) / (old.Y - i);
                        asum += a;
                        ++acount;
                        it->FIRSTX = j;
                        it->FIRSTY = i;
                    }
                }
                white = 0;
            }
        }
        float a = asum / acount;
        for (set<line>::iterator it = lines.begin(); it != lines.end(); ++it) {
            pair<int, int> old = it->first;
            it->FIRSTX = old.X + a * (old.Y - i);
            it->FIRSTY = i;
        }
        ptr += lineHeight * w;
    }
}

JNIEXPORT jint
JNICALL Java_org_opencv_samples_imagemanipulations_ImageManipulationsActivity_computeLineHeight
        (JNIEnv *, jobject, jlong matptr, jint w, jint h) {
    int white = 0;
    int best = 0;
    imageptr ptr = (imageptr) matptr;
    for (int i = 0; i < MAX; ++i) counters[i] = 0;
    white = 0;
    for (int i = SKIP; i < h; i += SKIP) {
        ptr += SKIP * w;
        for (int j = 0; j < w; ++j) {
            if (*ptr++ > 0) {
                ++white;
            } else {
                if (white > MIN && white < MAX) {
                    ++counters[white];
                }
                white = 0;
            }
        }
    }
    for (int i = MIN; i < MAX; ++i) {
        if (counters[i] > counters[best]) best = i;
    }
    lineHeight = best;
    avgLines = counters[best] * SKIP / h;
    findLines(ptr, w, h);
    return best;
}

JNIEXPORT jint
JNICALL Java_org_opencv_samples_imagemanipulations_ImageManipulationsActivity_colorizeLine
        (JNIEnv *, jobject, jlong matptr, jint w, jint h, jint lineh) {
    imageptr ptr = (imageptr) matptr;
    imageptr p;
    int white = 0;
    for (int i = 0; i < h * w; ++i) {
        if (*ptr++ > 0) {
            ++white;
        } else {
            if (abs(white - lineh) <= TOLERANCE) {
                ++white;
                p = ptr - white;
                while (--white) {
                    *(++p) = 127;
                }
            }
            white = 0;
        }
    }
    return 0;
}

JNIEXPORT jint
JNICALL Java_org_opencv_samples_imagemanipulations_ImageManipulationsActivity_colorizeLineOld
        (JNIEnv *, jobject, jlong matptr, jint w, jint h, jint lineh) {
    imageptr ptr = (imageptr) matptr;
    imageptr p;
    int white = 0;
    for (int i = 0; i < h * w; ++i) {
        if (*ptr++ > 0) {
            ++white;
        } else {
            if (abs(white - lineh) <= TOLERANCE) {
                ++white;
                p = ptr - white;
                while (--white) {
                    *(++p) = 127;
                }
            }
            white = 0;
        }
    }
    return 0;
}