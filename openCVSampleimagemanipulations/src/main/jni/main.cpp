#include "org_opencv_samples_musicrecognition_ImageManipulationsActivity.h"

#include <vector>
#include <set>
#include <utility>
#include <algorithm>

using namespace std;
typedef unsigned char *imageptr;

#define UPDATE(a, b, op) if(b op a) a=b
#define CHECKY if (y0 < 0 || y0 >= y1 || y1 >= originalH)
#define RANGECHECK (x >= 0 && x < originalW && y >= 0 && y < originalH)

int EXIT_STATUS = 0;

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

unsigned char guardian = 255;

inline unsigned char &ORG(int x, int y) {
    //if (!RANGECHECK) return guardian;
    return *(originalImg + originalW * y + x);
}

inline unsigned char &IMG(int x, int y) {
    //if (!RANGECHECK) return guardian;
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

JNICALL Java_org_opencv_samples_musicrecognition_ImageManipulationsActivity_colorizeLine
        (JNIEnv *, jobject, jint currentSound) {
    if (currentSound >= music.size()) return 1;
    sound &s = music[currentSound];
    penta &l = pentalines[s.y];
    if (lastY < s.y) {
        penta &l1 = pentalines[s.y - 1];
        if (s.y > 0)
            for (int x = lastX + 1; x < l1.x1; ++x) {
                float f1 = ((float) x - l1.x0) / (l1.x1 - l1.x0);
                float f0 = 1.0f - f1;
                int y0 = (l1.m0 - l1.th0) * f0 + (l1.m1 - l1.th1) * f1;
                int y1 = (l1.m0 + l1.th0) * f0 + (l1.m1 + l1.th1) * f1;
                for (int y = y0; y <= y1; ++y) {
                    if ((x & y & 1) == 0) IMG(x, y) ^= 255;
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
            if ((x & y & 1) == 0) IMG(x, y) ^= 255;
        }
    }
    lastX = s.x;
    lastY = s.y;
    return 0;
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
                if (yy < 4 || yy + 4 >= originalH) continue;
                imageptr baseptr = transposedImg + x * originalH + yy;
                int count = 0;
                for (int y = -4; y <= 4; ++y) {
                    if (*(baseptr + y) == 0) count += (1 + abs(y / 2));
                }
                if (count < 10) {
                    for (int y = -3; y <= 3; ++y) {
                        *(baseptr + y) = 255;
                    }
                    *baseptr = 200;
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
            sums[L][x - l.x0] = ((float) count) / (y1 - y0 + 1);
            all += sums[L][x - l.x0];
        }
        meanSums[L] = all / (l.x1 - l.x0 + 1);
    }
}

struct blob {
    int x0, x1;
    float mean;
    float max;
    unsigned char color;

    blob(int x0, int x1, float mean, int max, unsigned char color) :
            x0(x0), x1(x1), mean(mean), max(max), color(color) {
    }
};

vector<vector<blob> > blobs;

vector<pair<int, int> > findTops(int L, int x0, int x1) {
    vector<pair<int, int> > ans;
    penta l = pentalines[L];
    int last0 = x0 - 1;
    for (int i = x0; i <= x1 + 1; ++i) {
        if (i > x1 //element after last
            || sums[L][i] < 4 * meanSums[L] || //sum is small
            (last0 == i - 1 && i - 3 >= 0 &&
             sums[L][i - 3] * 1.5 > sums[L][i])) { //no hill on start
            if (i > last0 + 2) {
                ans.push_back(make_pair(last0 + 1, i - 1));
            }
            last0 = i;
        }
    }
    return ans;
}

int removeHorizontals(int L, int x0, int x1) {
    penta &l = pentalines[L];
    vector<line> lines;
    for (int x = x0 + l.x0; x <= x1 + l.x0; ++x) {
        //if (x - l.x0 < 0 || x - l.x0 >= sums[L].size())continue;
        if (sums[L][x - l.x0] >= 3 * meanSums[L]) {
            continue;//ignore tops
        }
        int currY = -1;
        int index = -1;
        int initSize = lines.size();

        float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
        if (f1 > 1 || f1 < 0) continue;
        float f0 = 1.0f - f1;

        int y0 = (l.m0 - 2 * l.th0) * f0 + (l.m1 - 2 * l.th1) * f1;
        int y1 = (l.m0 + 2 * l.th0) * f0 + (l.m1 + 2 * l.th1) * f1;

        CHECKY continue;

        vector<pair<int, int> > data;
        int lastWhite = y0 - 1;

//        for(int y = y0; y <= y1; ++y) if ((y ^ x) & 1) IMG(x, y) ^= 255;
//        continue;

        for (int y = y0; y <= y1 + 1; ++y) {
            if (y > y1 || IMG(x, y) > 0) {
                if (y - 1 > lastWhite + 1) {
                    data.push_back(make_pair(lastWhite + 1, y - 1));
                }
                lastWhite = y;
            }
        }
        for (int j = 0; j < data.size(); ++j) {
            while (currY < data[j].first) {
                if (++index < initSize)
                    currY = (lines[index].h1 + lines[index].y1) / 2;
                else {
                    break;
                }
            }
            if (index < initSize && currY <= data[j].second) {
                lines[index].set(x, data[j]);
            } else if (x - x0 - l.x0 < lineH) {
                lines.push_back(line(x, data[j]));
            }
        }
        for (int j = 0; j < lines.size(); ++j) {
            if (lines[j].x1 < x) {
                swap(lines[j], lines[lines.size() - 1]);
                lines.pop_back();
            }
        }
        sort(lines.begin(), lines.end());
    }
    for (int i = 0; i < lines.size(); ++i) {
        for (int x = lines[i].x0; x <= lines[i].x1; ++x) {
            float f1 = ((float) x - lines[i].x0) / (lines[i].x1 - lines[i].x0);
            float f0 = 1.0f - f1;
            int y0 = lines[i].y0 * f0 + lines[i].y1 * f1 - lineH / 2;
            int y1 = lines[i].h0 * f0 + lines[i].h1 * f1 + lineH / 2;
            for (int y = y0; y <= y1; ++y) {
                IMG(x, y) |= 44;
            }
        }
    }
}

struct cand {
    int y0, y1, totalCount;

    cand(int y0, int y1, int totalCount) : y0(y0), y1(y1), totalCount(totalCount) { }
};

int handleNote(blob note, int L, vector<pair<int, int> > &tops) {
    blobs[L].push_back(note);
    penta &l = pentalines[L];
    float f1 = (0.5f * (note.x0 + note.x1) - l.x0) / (l.x1 - l.x0);
    float f0 = 1.f - f1;
    if (f1 > 1 || f1 < 0) return 0;

    int y0 = (l.m0 - 1.66 * l.th0) * f0 + (l.m1 - 1.66 * l.th1) * f1;
    int y1 = (l.m0 + 1.66 * l.th0) * f0 + (l.m1 + 1.66 * l.th1) * f1;

    CHECKY return 0;

    int big = 0;
    int small = 0;

    vector<cand> candidates;

    int lastNotBig = y0 - 1;
    int totalCount = 0;

//    for (int x = note.x0 + l.x0; x <= note.x1 + l.x0; ++x)
//        for (int y = y0; y <= y1; ++y)
//            if (((x ^ y) & 5) == 0)
//                IMG(x, y) = 128;

    for (int y = y0; y <= y1; ++y) {
        int count = 0;
        for (int x = note.x0 + l.x0; x <= note.x1 + l.x0; ++x) {
            if (ORG(x, y) == 0) ++count;
        }
        if (2 * count > lineH) {
            totalCount += count;
            ++big;
        } else {
            if (4 * count > lineH) {
                ++small;
            }
            if (lastNotBig + 1 < y - 1) {
                candidates.push_back(cand(lastNotBig + 1, y - 1, totalCount));
                totalCount = 0;
            }
            lastNotBig = y;
        }
    }

    if (big > 2 * small) return 0; //not a note.

    int divide = 1;
    int chosen = -1;
    for (int c = 0; c < candidates.size(); ++c) {
        if (candidates[c].y1 - candidates[c].y0 > 0.5 * lineH) {
//            if (candidates[c].y1 - candidates[c].y0 > 100) {
//                if (candidates[c].y1 - candidates[c].y0 > EXIT_STATUS) {
//                    EXIT_STATUS = candidates[c].y1 - candidates[c].y0;
//                }
//                return 0;
//            }
//            for (int x = note.x0 + l.x0; x <= note.x1 + l.x0; ++x) {
//                for (int y = candidates[c].y0; y <= candidates[c].y1; ++y) {
//                    IMG(x, y) ^= 255;
//                }
//            }
            if (chosen > -1) {
                divide = 2;
            }
            chosen = c;
        }
    }

    if (chosen == -1) return 0;

    for (int y = candidates[chosen].y0; y <= candidates[chosen].y1; ++y)
        for (int x = note.x0 + l.x0; x <= note.x1 + l.x0; ++x) {
            if (RANGECHECK) {
                if (IMG(x, y) == 0 && (x & y & 1)) IMG(x, y) = 31;
            }
        }


    sound s;
    s.x = note.x1 + l.x0;
    s.y = L;
    s.len = 4 / divide;

    //y0 = (l.m0 - l.th0) * f0 + (l.m1 - l.th1) * f1;
    y1 = (l.m0 + l.th0) * f0 + (l.m1 + l.th1) * f1;
    float y = 0.5f * (candidates[chosen].y0 + candidates[chosen].y1);
    y = (y1 - y) / l.th0;
    y = y * 6 + 1.5;
    if (y < 0) y = 0;
    if (y > 14) y = 14;

    s.hei = y;
    music.push_back(s);
    return 1;
}

void addBlob(int x0, int x1, float count, float max, int L) {
    int len = (x1 - x0 + 1);
    float mean = count / len;

    unsigned char color = 0;

    vector<pair<int, int> > tops = findTops(L, x0, x1);
    int topsLen = 0;
    for (int i = 0; i < tops.size(); ++i)
        topsLen += tops[i].second - tops[i].first + 1;
    if (len < lineH && max < 0.03f) {
        //noise
//        color = 200;
//        blobs[L].push_back(blob(x0, x1, mean, max, color));
    } else if (mean > 2.5 * meanSums[L] && topsLen > len / 3) {
        //violin key, tempo, bars
        color = 223;
        blobs[L].push_back(blob(x0, x1, mean, max, color));
    } else if (max > 3 * meanSums[L]) {
        if (tops.size() == 1 || (x1 - x0) < 3 * lineH) {
            color = 50; //single note
            handleNote(blob(x0, x1, mean, max, color), L, tops);
        }
        if (tops.size() == 2 || ((x1 - x0) > 3 * lineH) && (x1 - x0) < 6 * lineH) {
            color = 50; //many notes
            int mid = (x0 + x1) / 2;
            //int horizontalsCount = removeHorizontals(L, tops.front().first, tops.back().second);
            int added = 0;
            added += handleNote(blob(x0, mid - 1, mean / 2, max, color), L, tops);
            added += handleNote(blob(mid + 1, x1, mean / 2, max, color), L, tops);
//            for (int i = 1; i <= added; ++i) {
//                for (int j = 0; j < horizontalsCount; ++j)
//                    music[music.size() - i].len /= 2;
//            }
        }
        else {
            //color = (tops.size() == 0) ? 0 : 200; //dots
            //blobs[L].push_back(blob(x0, x1, mean, max, color));
            //TODO: Handle 3 or more connected notes.
        }
    } else if (max < 0.075) {
        color = 100; //dots
        blobs[L].push_back(blob(x0, x1, mean, max, color));
        if (!music.empty() && music.back().y == L && music.back().x - x0 < 2 * lineH) {
            music.back().len *= 1.5;
        }
    }
}

void findBlobs() {
    music.clear();
    blobs.clear();
    blobs.resize(pentalines.size());
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        int last0 = -1;
        float count = 0;
        float max = 0;
        for (int i = 0; i < sums[L].size(); ++i) {
            if (sums[L][i] == 0) {
                if (i > last0 + 2) {
                    addBlob(last0 + 1, i - 1, count, max, L);
                }
                last0 = i;
                count = 0;
                max = 0;
            } else {
                count += sums[L][i];
                UPDATE(max, sums[L][i], >);
            }
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
//                imageptr p = transposedImg + GRID * originalH * i;
//                for (int k = data[i][j].first; k < data[i][j].second; ++k) {
//                    if (k % 2) *(p + k) = 0;
//                }
                lines.push_back(line(i * GRID, data[i][j]));
            } else {
//                imageptr p = transposedImg + GRID * originalH * i;
//                for (int k = data[i][j].first; k < data[i][j].second; ++k) {
//                    if (k % 3 == 0) *(p + k) = 255;
//                }
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
    //Draw pentalines
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
                *(ptr++) &= 223;
            }

            //Draw sums under line
            int count = (y1 - y0) * (sums[L][x - l.x0]);
            int color = 150;
            if (sums[L][x - l.x0] > 3 * meanSums[L]) color = 50;
            else if (sums[L][x - l.x0] > 0.75f * meanSums[L]) color = 100;

            for (int y = 0; y <= count; ++y) {
                *(ptr++) &= color;
            }
        }
    }
}

void drawBlobs() {
    for (int L = 0; L < pentalines.size(); ++L) {
        penta l = pentalines[L];
        if (l.x0 >= l.x1) continue;
        for (int i = 0; i < blobs[L].size(); ++i)
            for (int x = blobs[L][i].x0 + l.x0; x <= blobs[L][i].x1 + l.x0; ++x) {
                float f1 = ((float) x - l.x0) / (l.x1 - l.x0);
                float f0 = 1.0f - f1;
                int y0 = (l.m0 - l.th0) * f0 + (l.m1 - l.th1) * f1;
                int y1 = (l.m0 + l.th0) * f0 + (l.m1 + l.th1) * f1;
                imageptr ptr = transposedImg + x * originalH + y0;

                for (int y = y0; y <= y1; ++y) {
                    //TODO change to |= to remove highlights
                    *(ptr++) |= blobs[L][i].color;
                }
            }
    }
    return;
}

const int MAX = 32;
const int MIN = 2;
const int SKIP = 100;
int counters[MAX];

JNIEXPORT jint

JNICALL Java_org_opencv_samples_musicrecognition_ImageManipulationsActivity_computeLineHeight
        (JNIEnv *, jobject, jlong matptr, jlong original, jint w, jint h) {
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
    originalImg = (imageptr) original;
    originalH = w;
    originalW = h;

    init();
    findLines();
    correctLines();
    
    removeBlackLines();
    findSums();

    findBlobs();

    drawBlobs();
    drawLines();

    //postProcess();

    //createMusic();
    drawMusic();

    //int allnotes = 0;
    //for (int i = 0; i < notes.size(); ++i) allnotes += notes[i].size();
    //return allnotes;
    lastX = lastY = -1;
    return EXIT_STATUS;
}
