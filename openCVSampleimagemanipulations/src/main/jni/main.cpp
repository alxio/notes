#include "org_opencv_samples_imagemanipulations_ImageManipulationsActivity.h"

#include <vector>
#include <set>
#include <utility>
#include <algorithm>

using namespace std;
typedef unsigned char *imageptr;

#define UPDATE(a, b, op) if(b op a) a=b

const int TOLERANCE = 2;
const int GRID = 10;
const int MAX_HOLE = 10;
const int INFINITY = 1000000000;

const int VERTICAL_COLOR = 80;
const int NOTE_COLOR = 255;
const int HORIZONTAL_COLOR = 200;
const int OTHER_COLOR = 0;

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

struct sound {
    int len;
    int hei;
    int x, y;
};

vector<vector<pair<int, int> > > data; //[x][(y)], y1, y2
vector<line> lines;
vector<penta> pentalines;
vector<line> oldLines;
vector<sound> music;

vector<vector<float> > sums;
vector<float> meanSums;

int lastX = -1;
int lastY = -1;

JNIEXPORT jint

JNICALL Java_org_opencv_samples_imagemanipulations_ImageManipulationsActivity_colorizeLine
        (JNIEnv *, jobject, jlong matptr, jint w, jint h, jint currentSound) {
    if (currentSound >= music.size()) return 1;
    sound &s = music[currentSound];
    penta &l = pentalines[s.y];
    if (lastY < s.y) {
        penta &l1 = pentalines[s.y - 1];
        if (s.y > 0)
            for (int x = lastX + 1; x < originalW; ++x) {
                float f1 = ((float) x - l1.x0) / (l1.x1 - l1.x0);
                float f0 = 1.0f - f1;
                int y0 = (l1.m0 - l1.th0) * f0 + (l1.m1 - l1.th1) * f1;
                int y1 = (l1.m0 + l1.th0) * f0 + (l1.m1 + l1.th1) * f1;
                for (int y = y0; y <= y1; ++y) {
                    IMG(x, y) ^= 255;
                }
            }
        lastY = s.y;
        lastX = l.x0;
    }
    for (int x = lastX + 1; x <= s.x; ++x) {
        float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
        float f0 = 1.0f - f1;
        int y0 = (l.m0 - l.th0) * f0 + (l.m1 - l.th1) * f1;
        int y1 = (l.m0 + l.th0) * f0 + (l.m1 + l.th1) * f1;
        for (int y = y0; y <= y1; ++y) {
            IMG(x, y) ^= 255;
        }
    }
    lastX = s.x;
    lastY = s.y;
    return 0;
}

/*

void createMusic() {
    music.clear();
    for (int L = 0; L < pentalines.size(); ++L) {
        penta &l = pentalines[L];
        int note;
        int hbar = -1;
        int vbar = -1;
        int vpos = -100;
        int hpos = -100;
        for (note = 0; note < notes[L].size(); ++note) {
            sound s;
            blob &n = notes[L][note];
            while (vpos < n.x0 - lineH / 2) {
                if (++vbar < vbars[L].size()) {
                    vpos = vbars[L][vbar].x1;
                } else {
                    break;
                }
            }
            while (hpos < n.x0 - lineH / 2) {
                if (++hbar < hbars[L].size()) {
                    hpos = hbars[L][hbar].x1;
                } else {
                    break;
                }
            }
            //note has horizontal staff
            if (vbar < vbars[L].size() && vpos < n.x1 + lineH / 2) {
                if (hbar < hbars[L].size() && hbars[L][hbar].x0 < n.x1 + lineH / 2) {
                    if (hbar + 1 < hbars[L].size() && hbars[L][hbar + 1].x0 < n.x1 + lineH / 2) {
                        s.len = 1;
                    } else {
                        s.len = 2;
                    }
                } else {
                    s.len = 4;
                }
            } else {
                s.len = 8;
            }
            float y = (n.y0 + n.y1) / 2;
            float x = (n.x0 + n.x1) / 2;
            float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
            float f0 = 1.0f - f1;
            int y0 = (l.m0 - l.th0) * f0 + (l.m1 - l.th1) * f1;
            int y1 = (l.m0 + l.th0) * f0 + (l.m1 + l.th1) * f1;

            if (y0 >= y1) continue;

            y = (y1 - y) / l.th0;
            y = y * 6 + 1.5;
            if (y < 0) y = 0;
            if (y > 14) y = 14;

            s.hei = (int) y;
            s.x = n.x1;
            s.y = L;
            music.push_back(s);
        }
    }
}

void drawMusic() {
    imageptr ptr = transposedImg;
    *ptr++ = music.size() / 128;
    *ptr++ = music.size() % 128;
    for (int i = 0; i < music.size(); ++i) {
        *ptr++ = music[i].hei;
        *ptr++ = music[i].len;
    }
}

*/

void removeBlackLines() {
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        if (l.x0 >= l.x1) continue;
        for (int base = -2; base <= 2; ++base) {
            float y0 = l.m0 + base * l.th0 / 3.0f;
            float y1 = l.m1 + base * l.th1 / 3.0f;
            int lastCount = 0;
            for (int x = l.x0; x <= l.x1; ++x) {
                float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
                float f0 = 1.0f - f1;
                int yy = (int) (y0 * f0 + y1 * f1);
                imageptr baseptr = transposedImg + x * originalH + yy;
                int count = 0;
                for (int y = -4; y <= 4; ++y) {
                    if (*(baseptr + y) == 0) count += (1 + abs(y / 2));
                }
                if (count < 10) {
                    for (int y = -3; y <= 3; ++y) {
                        *(baseptr + y) = 255;
                    }
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

void findSums() {
    meanSums.clear();
    meanSums.resize(pentalines.size());
    sums.clear();
    sums.resize(pentalines.size());
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        if (l.x0 >= l.x1) continue;
        sums[L].resize(l.x1 - l.x0 + 1);
        float all = 0;
        for (int x = l.x0; x <= l.x1; ++x) {
            sums[L][x - l.x0] = 0;
            float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
            float f0 = 1.0f - f1;
            int y0 = (l.m0 - 3 * l.th0 / 2) * f0 + (l.m1 - 3 * l.th1 / 2) * f1;
            int y1 = (l.m0 + 3 * l.th0 / 2) * f0 + (l.m1 + 3 * l.th1 / 2) * f1;
            if (y0 >= y1) continue;
            imageptr ptr = transposedImg + x * originalH + y0;
            int count = 0;
            for (int y = y0; y <= y1; ++y) {
                if (ptr < transposedImg || ptr >= transposedImg + originalW * originalH) break;
                if (*ptr++ == 0) {
                    ++count;
                }
            }
            sums[L][x - l.x0] = ((float) count) / (y1 - y0);
            all += sums[L][x - l.x0];
        }
        meanSums[L] = all / (l.x1 - l.x0 + 1);
    }
}

void findBlobs() {
    blobs.clear();
    blobs.resize(pentalines.size());
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        vector<blob> lines;
        if (l.x0 >= l.x1) continue;
        for (int x = l.x0; x <= l.x1; ++x) {
            float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
            float f0 = 1.0f - f1;
            int y0 = (l.m0 - 3 * l.th0 / 2) * f0 + (l.m1 - 3 * l.th1 / 2) * f1;
            int y1 = (l.m0 + 3 * l.th0 / 2) * f0 + (l.m1 + 3 * l.th1 / 2) * f1;
            if (y0 >= y1) continue;
            int currY = -1;
            int index = -1;
            int initSize = lines.size();
            int count = 0;
            imageptr ptr = transposedImg + x * originalH + y0;
            for (int y = y0; y <= y1 + 1; ++y) {
                if (ptr < transposedImg || ptr >= transposedImg + originalW * originalH) break;
                if (y > y1 || *ptr++ == 0) {
                    ++count;
                } else {
                    if (count > MIN_H * lineH) {
                        while (currY < y - count) {
                            if (++index < initSize) {
                                if (index < 0 || index >= lines.size())return;
                                currY = (lines[index].y0 + lines[index].y1) / 2;
                            } else {
                                break;
                            }
                        }
                        if (index >= initSize || currY >= y ||
                            abs(y - (1 + count) / 2 - currY) > lineH / 2) {
                            lines.push_back(blob(x, x, y - count, y - 1));
                        } else {
                            if (index < 0 || index >= lines.size())return;
                            lines[index].x1 = x;
                            lines[index].y0 = y - count;
                            lines[index].y1 = y - 1;
                            UPDATE(lines[index].h, count, >);
                        }
                    }
                    count = 0;
                }
            }
            int toRemove = 0;
            for (int j = 0; j < lines.size(); ++j) {
                if (lines[j].x1 < x) {
                    blobs[L].push_back(lines[j]);
                    lines[j].y0 = INFINITY;
                    toRemove++;
                }
            }
            sort(lines.begin(), lines.end());
            lines.erase(lines.end() - toRemove, lines.end());
        }
        for (int j = 0; j < lines.size(); ++j) {
            blobs[L].push_back(lines[j]);
        }
        lines.clear();
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

            if (x1 + GRID < originalW) x1 += GRID;
            if (x1 + GRID < originalW) x1 += GRID;

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

            int count = (y1 - y0) * (sums[L][x - l.x0]);
            int color = 150;
            if (sums[L][x - l.x0] > 3 * meanSums[L]) color = 50;
            else if (sums[L][x - l.x0] > 0.75f * meanSums[L]) color = 100;

            for (int y = 0; y <= count; ++y) {
                *(ptr++) &= color;
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

    removeBlackLines();

    //findBlobs();
    findSums();
    drawLines();
    //classifyBlobs();
    //drawBlobs();
    //postProcess();

    //createMusic();
    //drawMusic();

    int allnotes = 0;
    for (int i = 0; i < notes.size(); ++i) allnotes += notes[i].size();
    return allnotes;
}
