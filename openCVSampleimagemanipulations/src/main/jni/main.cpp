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

vector<vector<pair<int, int> > > data; //[x][(y)], y1, y2

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

vector<line> lines;

void init() {
    data.resize(originalW / GRID);
    for (int i = 0; i < data.size(); ++i) data[i].clear();
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
    int currY = 1000000000;
    int index = 0;
    for (int i = 0; i < data.size(); ++i) {
        int maxIndex = lines.size() - 1;
        if (data[i].size() < 4) continue;
        for (int j = 0; j < data[i].size(); ++i) {
            while (currY < data[i][j].first && index < maxIndex) {
                currY = lines[++index].h1;
            }
            if (lines[index].y1 > data[i][j].second) {
                lines.push_back(line(i, data[i][j]));
            } else {
                lines[index].set(i, data[i][j]);
            }
        }
        if (maxIndex + 1 != lines.size())
            sort(lines.begin(), lines.end());
    }
}

JNIEXPORT jint
JNICALL Java_org_opencv_samples_imagemanipulations_ImageManipulationsActivity_colorizeLine
        (JNIEnv *, jobject, jlong matptr, jint w, jint h, jint lineh) {
    transposedImg = (imageptr) matptr;
    originalH = w;
    originalW = h;
    lineH = lineh;
    init();
    findLines();
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
    return best;
}