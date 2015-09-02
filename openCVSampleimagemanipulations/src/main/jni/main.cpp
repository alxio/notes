#include "org_opencv_samples_imagemanipulations_ImageManipulationsActivity.h"

#include <vector>
#include <set>
#include <utility>
#include <algorithm>

using namespace std;
typedef unsigned char *imageptr;

const int TOLERANCE = 1;
const int GRID = 10;

int originalH, originalW, lineH;

imageptr transposedImg;
imageptr originalImg;

struct line {
    int x0, y0, h0, x1, y1, h1;

    line(int x, pair<int, int> y) {
        x0 = x1 = x;
        y0 = y1 = y.first;
        h0 = h1 = y.second;
    }

    void set(int x, pair<int, int> y) {
        x1 = x;
        y1 = y.first;
        h1 = y.second;
    }

    bool operator<(line other) const {
        return y0 != other.y0 ? y0 < other.y0
                              : y1 < other.y1;
    }
};

vector<vector<pair<int, int> > > data; //[x][(y)], y1, y2
vector<line> lines;

void init() {
    data.clear();
    data.resize(originalW / GRID);
    int white = 0;
    imageptr ptr = transposedImg;
    for (int i = GRID; i < originalW; i += GRID) {
        ptr += GRID * originalH;
        for (int j = 0; j < originalH; ++j) {
            if (*ptr++ > 0) {
                ++white;
            } else {
                if (abs(white - lineH) < TOLERANCE) {
                    data[i / GRID - 1].push_back(make_pair(j - white, j));
                }
                white = 0;
            }
        }
    }
}

void findLines() {
    lines.clear();
    for (int i = 0; i < data.size(); ++i) {
        int currY = 1000000000;
        int index = 0;
        int initSize = lines.size();
        if (data[i].size() < 4) continue;
        for (int j = 0; j < data[i].size(); ++j) {
            while (currY < data[i][j].first) {
                if (++index < initSize)
                    currY = lines[index].h1;
                else break;
            }
            if (index >= initSize) {// || lines[index].y1 > data[i][j].second) {
                lines.push_back(line(i, data[i][j]));
            } else {
                lines[index].set(i, data[i][j]);
            }
        }
        if (initSize != lines.size())
            sort(lines.begin(), lines.end());
    }
}

void drawLines() {
    for (int L = 0; L < lines.size(); ++L) {
        line l = lines[L];
        if (l.x0 >= l.x1) continue;
        for (int x = l.x0; x <= l.x1; ++x) {
            int y0 = l.y0 + (l.y1 - l.y0) * (x - l.x0) / (l.x1 - l.x0);
            int y1 = l.h0 + (l.h1 - l.h0) * (x - l.x0) / (l.x1 - l.x0);
            imageptr ptr = transposedImg + x * originalH + y0;
            for (int y = y0; y < y1; ++y) {
                *(ptr++) = 200;
            }
        }
    }
}

JNIEXPORT jint
JNICALL Java_org_opencv_samples_imagemanipulations_ImageManipulationsActivity_colorizeLine
        (JNIEnv *, jobject, jlong matptr, jint w, jint h, jint lineh) {
    originalImg = (imageptr) matptr;
    //lineH = lineh;
    //init();
    //findLines();
    //drawLines();
    return 0;
}

const int MAX = 32;
const int MIN = 2;
const int SKIP = 100;
int counters[MAX];

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

    lineH = best;
    transposedImg = (imageptr) matptr;
    originalH = w;
    originalW = h;
    init();
    findLines();
    //drawLines();

    return best;
}