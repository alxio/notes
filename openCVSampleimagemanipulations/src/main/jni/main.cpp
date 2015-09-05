#include "org_opencv_samples_imagemanipulations_ImageManipulationsActivity.h"

#include <vector>
#include <set>
#include <utility>
#include <algorithm>

using namespace std;
typedef unsigned char *imageptr;

const int TOLERANCE = 2;
const int GRID = 5;
const int MAX_HOLE = 40;

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

    line(line up, line down) {
        x0 = up.x0;
        x1 = up.x1;
        int m0 = (up.y0 + down.h0) / 2;
        y0 = 2 * up.y0 - m0;
        h0 = 2 * down.h0 - m0;
        int m1 = (up.y1 + down.h1) / 2;
        y1 = 2 * up.y1 - m1;
        h1 = 2 * down.h1 - m1;
    }

    void set(int x, pair<int, int> y) {
        x1 = x;
        y1 = y.first;
        h1 = y.second;
    }

    bool operator<(line other) const {
        return y0 < other.y0;
    }
};

vector<vector<pair<int, int> > > data; //[x][(y)], y1, y2
vector<line> lines;
vector<line> pentalines;
vector<line> oldLines;

void init() {
    int size = originalW / GRID - 1;
    data.clear();
    data.resize(size + 1);
    imageptr ptr = transposedImg + GRID * originalH;
    for (int i = 1; i < size; ++i) {
        int white = 0;
        for (int j = 0; j < originalH; ++j) {
            if (*ptr++ > 0) {
                ++white;
            } else {
                if (abs(white - lineH) <= TOLERANCE && white > 2 && white <= j) {
                    data[i].push_back(make_pair(j - white, j));
                }
                white = 0;
            }
        }
        ptr += (GRID - 1) * originalH;
    }
}

void findLines() {
    swap(lines, oldLines);
    lines.clear();
    for (int i = 1; i < data.size(); ++i) {
        int currY = -1;
        int index = -1;
        int initSize = lines.size();
        if (data[i].size() < 4) continue;
        for (int j = 0; j < data[i].size(); ++j) {
            while (currY < data[i][j].first) {
                if (++index < initSize)
                    currY = (lines[index].h1 + lines[index].y1) / 2;
                else {
                    break;
                }
            }
            if (index >= initSize || currY > data[i][j].second) {
                imageptr p = transposedImg + GRID * originalH * i;
                for (int k = data[i][j].first; k < data[i][j].second; ++k) {
                    if (k % 2) *(p + k) = 0;
                }
                lines.push_back(line(i * GRID, data[i][j]));
            } else {
                imageptr p = transposedImg + GRID * originalH * i;
                for (int k = data[i][j].first; k < data[i][j].second; ++k) {
                    if (k % 3 == 0) *(p + k) = 255;
                }
                lines[index].set(i * GRID, data[i][j]);
            }
        }
        for (int j = 0; j < lines.size(); ++j) {
            if (lines[j].x1 < (i - MAX_HOLE) * GRID) {
                swap(lines[j], lines[lines.size() - 1]);
                lines.pop_back();
            }
        }
        sort(lines.begin(), lines.end());
    }
}

int correctLines() {
    for (int j = 0; j < lines.size(); ++j) {
        if (lines[j].x1 - lines[j].x0 < originalW / 2) {
            swap(lines[j], lines[lines.size() - 1]);
            lines.pop_back();
        }
    }
    sort(lines.begin(), lines.end());
    if (lines.size() % 4 > 0) {
        lines.clear();
        swap(lines, oldLines);
        return 1;
    }
    pentalines.clear();
    int x0, x1;
    int count = 0;
    for (int a = 0; a < lines.size() / 4; ++a) {
        x0 = 1000000000;
        x1 = 0;
        for (int i = 0; i < 4; ++i) {
            line &l = lines[4 * a + i];
            if (l.x0 < x0) x0 = l.x0;
            if (l.x1 > x1) x1 = l.x1;
        }
        for (int i = 0; i < 4; ++i) {
            line &l = lines[4 * a + i];
            if (l.x1 == l.x0) continue;
            if (l.x0 > x0) {
                l.y0 += (l.y0 - l.y1) * (l.x0 - x0) / (l.x1 - l.x0);
                l.h0 += (l.h0 - l.h1) * (l.x0 - x0) / (l.x1 - l.x0);
                l.x0 = x0;
            }
            if (l.x1 < x1) {
                l.y1 += (l.y1 - l.y0) * (x1 - l.x1) / (l.x1 - l.x0);
                l.h1 += (l.h1 - l.h0) * (x1 - l.x1) / (l.x1 - l.x0);
                l.x1 = x1;
            }
        }
        line penta = line(lines[4 * a], lines[4 * a + 3]);
        pentalines.push_back(penta);
    }
    return 0;
}

void drawLines() {
    for (int L = 0; L < pentalines.size(); ++L) {
        line l = pentalines[L];
        if (l.x0 >= l.x1) continue;
        for (int x = l.x0; x <= l.x1; ++x) {
            int y0 = l.y0 + (l.y1 - l.y0) * (x - l.x0) / (l.x1 - l.x0);
            int y1 = l.h0 + (l.h1 - l.h0) * (x - l.x0) / (l.x1 - l.x0);
            imageptr ptr = transposedImg + x * originalH + y0;
            for (int y = y0; y <= y1; ++y) {
                *(ptr) |= 32;
                *(ptr++) &= 224;
            }
        }
    }
    for (int L = 0; L < lines.size(); ++L) {
        line l = lines[L];
        if (l.x0 >= l.x1) continue;
        for (int x = l.x0; x <= l.x1; ++x) {
            int y0 = l.y0 + (l.y1 - l.y0) * (x - l.x0) / (l.x1 - l.x0);
            int y1 = l.h0 + (l.h1 - l.h0) * (x - l.x0) / (l.x1 - l.x0);
            imageptr ptr = transposedImg + x * originalH + y0;
            for (int y = y0; y <= y1; ++y) {
                *(ptr++) &= 160;
            }
        }
    }
}

void postProcess() {
    for (imageptr ptr = transposedImg; ptr < transposedImg + originalH * originalW; ++ptr) {
        if (*ptr == 0) *ptr = 200;
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
        ptr += (SKIP - 1) * w;
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
    correctLines();
    drawLines();
    postProcess();
    return best;
}