#include "org_opencv_samples_imagemanipulations_ImageManipulationsActivity.h"

#include <vector>
#include <set>
#include <utility>
#include <algorithm>

using namespace std;
typedef unsigned char *imageptr;

const int TOLERANCE = 2;
const int GRID = 20;
const int MAX_HOLE = 5;

int originalH, originalW, lineH;

imageptr transposedImg;
imageptr originalImg;

inline unsigned char &IMG(int x, int y) {
    return *(transposedImg + originalH * x + y);
}

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
        y0 = up.y0 + 1.25 * (up.y0 - m0);
        h0 = down.h0 + 1.25 * (down.h0 - m0);
        int m1 = (up.y1 + down.h1) / 2;
        y1 = up.y1 + 1.25 * (up.y1 - m1);
        h1 = down.h1 + 1.25 * (down.h1 - m1);
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

struct penta {
    int x0, x1, m0, m1, th0, th1;

    penta(line up, line down) {
        x0 = up.x0;
        x1 = up.x1;
        m0 = (up.y0 + down.h0) / 2;
        th0 = -up.y0 + down.y0;
        m1 = (up.y1 + down.h1) / 2;
        th1 = -up.y1 + down.y1;
    }
};

struct blob {
    blob(int x0, int x1, int y0, int y1) {
        this->x0 = x0;
        this->x1 = x1;
        this->y0 = y0;
        this->y1 = y1;
    }

    int x0, x1, y0, y1;
};

vector<vector<pair<int, int> > > data; //[x][(y)], y1, y2
vector<line> lines;
vector<penta> pentalines;
vector<line> oldLines;
vector<vector<blob> > blobs;


void removeBlackLines() {
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        if (l.x0 >= l.x1) continue;
        for (int base = -2; base <= 2; ++base) {
            float y0 = l.m0 + base * l.th0 / 3.0f;
            float y1 = l.m1 + base * l.th1 / 3.0f;
            for (int x = l.x0; x <= l.x1; ++x) {
                float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
                float f0 = 1.0f - f1;
                int yy = (int) (y0 * f0 + y1 * f1);
                imageptr baseptr = transposedImg + x * originalH + yy;
                int count = 0;
                for (int y = -3; y <= 3; ++y) {
                    if (*(baseptr + y) == 0) count += (1 + abs(y / 2));
                }
                if (count < 6) {
                    for (int y = -3; y <= 3; ++y) {
                        *(baseptr + y) = 255;
                    }
                }
//                if (*(baseptr - 3) == 0 && *(baseptr - 4) == 0) continue;
//                if (*(baseptr + 3) == 0 && *(baseptr + 4) == 0) continue;
//                for (int y = -2; y <= 2; ++y) {
//                    *(baseptr + y) = 255;
////                    if (*(baseptr + y) == 0) *(baseptr + y) = 159;
////                    else *(baseptr + y) = 95;
//                }
            }
        }
    }
}

float START_TRESH = 0.9;
float STOP_TRESH = 0.75;

void findBlobs2() {
    blobs.clear();
    blobs.resize(pentalines.size());
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        if (l.x0 >= l.x1) continue;
        int state = 0;
        int first = 0;
        for (int x = l.x0; x <= l.x1; ++x) {
            float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
            float f0 = 1.0f - f1;
            int y0 = (l.m0 - 3 * l.th0 / 2) * f0 + (l.m1 - 3 * l.th1 / 2) * f1;
            int y1 = (l.m0 + 3 * l.th0 / 2) * f0 + (l.m1 + 3 * l.th1 / 2) * f1;
            imageptr ptr = transposedImg + x * originalH + y0;
            int max = 0;
            int count = 0;
            for (int y = y0; y <= y1; ++y) {
                if (*ptr++ == 0) {
                    ++count;
                } else {
                    if (count > max) max = count;
                    count = 0;
                }
            }
            if (max > START_TRESH * lineH) {
                if (state == 0) {
                    first = x;
                    state = 1;
                }
            }
            if (max < STOP_TRESH * lineH || x - first > 2 * lineH) {
                if (state == 1) {
                    blobs[L].push_back(blob(first, x, y0, y1));
                    state = 0;
                }
            }
//            if (state) {
//                ptr = transposedImg + x * originalH + y0;
//                for (int y = y0; y <= y1; ++y) {
//                    *ptr++ &= 127;
//                }
//            }
        }
    }
}

void correctBlobs2() {
    for (int L = 0; L < blobs.size(); ++L) {
        penta l = pentalines[L];
        for (int b = 0; b < blobs[L].size(); ++b) {
            blob &bl = blobs[L][b];
            int best = 0;
            int besty = 0;
            int c = 0;
            for (int y = bl.y0; y <= bl.y1; ++y) {
                int count = 0;
                for (int x = bl.x0; x <= bl.x1; ++x) {
                    if (IMG(x, y) == 0) {
                        ++count;
                    }
                }
                if (count > lineH) {
                    ++c;
                } else {
                    if (c > best) {
                        best = c;
                        besty = y - c;
                    }
                    c = 0;
                }
            }
            bl.y0 = besty;
            bl.y1 = besty + best;
        }
    }
}

void drawBlobs() {
    for (int L = 0; L < blobs.size(); ++L) {
        penta l = pentalines[L];
        for (int b = 0; b < blobs[L].size(); ++b) {
            blob bl = blobs[L][b];
            int y0 = bl.y0;
            int y1 = bl.y1;
            for (int x = bl.x0; x <= bl.x1; ++x) {
                imageptr ptr = transposedImg + x * originalH + y0;
                for (int y = y0; y <= y1; ++y) {
                    *ptr++ ^= 255;
                }
            }
        }
    }
}

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

const float MIN_H = 0.8f;

void findBlobs(){
    blobs.clear();
    blobs.resize(pentalines.size());
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        vector<blob>& lines = blobs[L];
        if (l.x0 >= l.x1) continue;
        for (int x = l.x0; x <= l.x1; ++x) {
            float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
            float f0 = 1.0f - f1;
            int y0 = (l.m0 - 3 * l.th0 / 2) * f0 + (l.m1 - 3 * l.th1 / 2) * f1;
            int y1 = (l.m0 + 3 * l.th0 / 2) * f0 + (l.m1 + 3 * l.th1 / 2) * f1;
            
            int currY = -1;
            int index = -1;
            int initSize = lines.size();
            int count = 0;
            imageptr ptr = transposedImg + x * originalH + y0;
            
            for (int y = y0; y <= y1; ++y) {
                if (*ptr++ == 0) {
                    ++count;
                } else {
                    if(count > MIN_H * lineH){
                        while (currY < y - count) {
                            if (++index < initSize)
                                currY = (lines[index].y0 + lines[index].y1) / 2;
                            else {
                                break;
                            }
                        }
                        if (index >= initSize || currY >= y) {
                            lines.push_back(blob(x,x,y - count,y-1));
                        } else {
                            lines[index].x1 = x;
                            lines[index].y0 = y - count;
                            lines[index].y1 = y - 1;
                        }
                    }
                    count = 0;
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
        if (lines[j].x1 - lines[j].x0 < originalW / 4 || lines[j].x1 == lines[j].x0) {
            swap(lines[j], lines[lines.size() - 1]);
            lines.pop_back();
        }
    }
    sort(lines.begin(), lines.end());
    //return 0;
    int x0, x1;
    int count = 0;
    int skipped = -1;
    int y = 0;
    vector<int> badLines;
    pentalines.clear();
    for (int i = 0; i < lines.size(); ++i) {
        line li = lines[i];
        if (li.x1 == li.x0) continue;
        int h = li.h0 + (li.h1 - li.h0) * (originalW / 2 - li.x0) / (li.x1 - li.x0);
        //int h = (li.h0 + li.h1) / 2;
        if (count > 0 && h - y < lineH * 2) {
            ++count;
            y = h;
        } else if (count > 0 && count < 3 && skipped == -1 && h - y < lineH * 3) {
            skipped = ++count;
            ++count;
            y = h;
        } else {
            y = h;
            count = 1;
            skipped = -1;
        }
        if (count == 4) {
            int first = skipped > 0 ? i - 2 : i - 3;
            x0 = 1000000000;
            x1 = 0;
            for (int j = first; j <= i; ++j) {
                line &l = lines[j];
                if (l.x0 < x0) x0 = l.x0;
                if (l.x1 > x1) x1 = l.x1;
            }
            for (int j = first; j <= i; ++j) {
                line &l = lines[j];
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
            penta pen = penta(lines[first], lines[i]);
            pentalines.push_back(pen);
            count = 0;
        }
    }
    for (int i = badLines.size() - 1; i >= 0; --i) {
        lines.erase(lines.begin() + badLines[i]);
    }
    return 0;
}

void drawLines() {
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        if (l.x0 >= l.x1) continue;
        for (int x = l.x0; x <= l.x1; ++x) {
            float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
            float f0 = 1.0f - f1;
            int y0 = (l.m0 - 3 * l.th0 / 3) * f0 + (l.m1 - 3 * l.th1 / 3) * f1;
            int y1 = (l.m0 + 3 * l.th0 / 3) * f0 + (l.m1 + 3 * l.th1 / 3) * f1;
            imageptr ptr = transposedImg + x * originalH + y0;
            for (int y = y0; y <= y1; ++y) {
                *(ptr) |= 32;
                *(ptr++) &= 224;
            }
        }
    }
    return;
    for (int L = 0; L < lines.size(); ++L) {
        line l = lines[L];
        if (l.x0 >= l.x1) continue;
        for (int x = l.x0; x <= l.x1; ++x) {
            int y0 = l.y0 + (l.y1 - l.y0) * (x - l.x0) / (l.x1 - l.x0);
            int y1 = l.h0 + (l.h1 - l.h0) * (x - l.x0) / (l.x1 - l.x0);
            imageptr ptr = transposedImg + x * originalH + y0;
            for (int y = y0; y <= y1; ++y) {
                *(ptr++) &= 192;
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
    findBlobs();
    //removeBlackLines();
    //correctBlobs();
    drawBlobs();
    drawLines();
    postProcess();
    return best;
}
