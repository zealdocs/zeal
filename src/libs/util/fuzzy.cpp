// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fuzzy.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Zeal {
namespace Util {
namespace Fuzzy {

namespace {
constexpr double SCORE_GAP_LEADING = -0.005;
constexpr double SCORE_GAP_TRAILING = -0.005;
constexpr double SCORE_GAP_INNER = -0.01;
constexpr double SCORE_MATCH_CONSECUTIVE = 1.0;
constexpr double SCORE_MATCH_SLASH = 0.9;
constexpr double SCORE_MATCH_WORD = 0.8;
constexpr double SCORE_MATCH_CAPITAL = 0.7;
constexpr double SCORE_MATCH_DOT = 0.6;
constexpr int FZY_MAX_LEN = 1024;

void precomputeBonus(const QString &haystack, double *matchBonus)
{
    // Initialize to '/' so the first character of the haystack always receives
    // SCORE_MATCH_SLASH (0.9), the highest boundary bonus. This mirrors fzy's
    // original file-path design where every path component starts after a '/'.
    // For Zeal's symbol names the first character is conceptually a word
    // boundary, but the high bonus is kept intentionally: it strongly rewards
    // prefix matches.
    QChar lastCh = '/';
    for (int i = 0; i < haystack.length(); ++i) {
        const QChar ch = haystack[i];

        if (lastCh == '/' || lastCh == '\\') {
            matchBonus[i] = SCORE_MATCH_SLASH;
        } else if (lastCh == '-' || lastCh == '_' || lastCh == ' ') {
            matchBonus[i] = SCORE_MATCH_WORD;
        } else if (lastCh == '.' || lastCh == ':') {
            matchBonus[i] = SCORE_MATCH_DOT;
        } else if (lastCh.isLower() && ch.isUpper()) {
            matchBonus[i] = SCORE_MATCH_CAPITAL;
        } else {
            matchBonus[i] = 0.0;
        }

        lastCh = ch;
    }
}

// Check if all characters in needle exist in haystack (in order, case-insensitive)
// This is a pre-filter before running the expensive DP algorithm
bool hasMatch(const QString &needle, const QString &haystack)
{
    int haystackPos = 0;
    const int haystackLen = haystack.length();

    for (int i = 0; i < needle.length(); ++i) {
        const QChar needleCh = needle[i].toLower();
        bool found = false;

        while (haystackPos < haystackLen) {
            if (haystack[haystackPos].toLower() == needleCh) {
                found = true;
                ++haystackPos;
                break;
            }
            ++haystackPos;
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

} // anonymous namespace

// ============================================================================
// High-level Qt convenience API implementation
// ============================================================================

double score(const QString &needle, const QString &haystack, QVector<int> *positions)
{
    // Pre-filter: check if all needle characters exist in haystack (performance optimization)
    // This avoids expensive DP computation on unmatchable strings
    if (!needle.isEmpty() && !haystack.isEmpty() && hasMatch(needle, haystack)) {
        return computeScore(needle, haystack, positions);
    }

    // No match - return -infinity (SQL will filter with WHERE score > 0)
    if (positions) {
        positions->clear();
    }

    return -std::numeric_limits<double>::infinity();
}

// ============================================================================
// Low-level API implementation
// ============================================================================

double computeScore(const QString &needle, const QString &haystack, QVector<int> *positions)
{
    const int needleLen = needle.length();
    const int haystackLen = haystack.length();

    if (needleLen == 0 || haystackLen == 0 || needleLen > haystackLen) {
        if (positions) {
            positions->clear();
        }

        return -std::numeric_limits<double>::infinity();
    }

    if (needleLen == haystackLen) {
        // Equal length strings get infinity score only if they actually match (case-insensitive)
        if (needle.compare(haystack, Qt::CaseInsensitive) == 0) {
            if (positions) {
                // Fill positions for exact match: [0, 1, 2, ..., n-1]
                positions->resize(needleLen);
                for (int i = 0; i < needleLen; ++i) {
                    (*positions)[i] = i;
                }
            }

            return std::numeric_limits<double>::infinity();
        }

        // Otherwise return no match - equal length non-matching strings can't fuzzy match
        if (positions) {
            positions->clear();
        }

        return -std::numeric_limits<double>::infinity();
    }

    if (haystackLen > FZY_MAX_LEN || needleLen > FZY_MAX_LEN) {
        if (positions) {
            positions->clear();
        }

        return -std::numeric_limits<double>::infinity();
    }

    double matchBonus[FZY_MAX_LEN] = {};  // Zero-initialize to satisfy static analyzer
    precomputeBonus(haystack, matchBonus);

    const double SCORE_MIN = -std::numeric_limits<double>::infinity();

    // Always allocate 2D tables on heap (simpler, memory overhead negligible for typical searches)
    double **D = new double*[needleLen];
    double **M = new double*[needleLen];
    for (int i = 0; i < needleLen; ++i) {
        D[i] = new double[haystackLen];
        M[i] = new double[haystackLen];
    }

    // Forward pass: compute scores
    for (int i = 0; i < needleLen; ++i) {
        double prevScore = SCORE_MIN;
        const double gapScore = (i == needleLen - 1) ? SCORE_GAP_TRAILING : SCORE_GAP_INNER;

        const QChar needleCh = needle[i].toLower();

        for (int j = 0; j < haystackLen; ++j) {
            if (needleCh == haystack[j].toLower()) {
                double score = SCORE_MIN;

                if (i == 0) {
                    score = (j * SCORE_GAP_LEADING) + matchBonus[j];
                } else if (j > 0) {
                    const double prevM = M[i - 1][j - 1];
                    const double prevD = D[i - 1][j - 1];
                    score = std::max(prevM + matchBonus[j],
                                prevD + SCORE_MATCH_CONSECUTIVE);
                }

                D[i][j] = score;
                M[i][j] = prevScore = std::max(score, prevScore + gapScore);
            } else {
                D[i][j] = SCORE_MIN;
                M[i][j] = prevScore = prevScore + gapScore;
            }
        }
    }

    const double result = M[needleLen - 1][haystackLen - 1];

    // Backtrack to find positions if requested (fzy algorithm)
    // Only backtrack if we have a valid match (not SCORE_MIN)
    if (positions != nullptr && result != SCORE_MIN) {
        positions->resize(needleLen);
        bool matchRequired = false;

        for (int i = needleLen - 1, j = haystackLen - 1; i >= 0; --i) {
            for (; j >= 0; --j) {
                // Check if this is a valid match position on the optimal path
                if (D[i][j] != SCORE_MIN && (matchRequired || D[i][j] == M[i][j])) {
                    // Check if we used consecutive match bonus to get here.
                    // Use D[i][j] (score at this specific position), not M[i][j]
                    // (global prefix optimum), which may reflect a different path entirely.
                    matchRequired = (i > 0 && j > 0 &&
                                   D[i][j] == D[i - 1][j - 1] + SCORE_MATCH_CONSECUTIVE);
                    (*positions)[i] = j;
                    --j;
                    break;
                }
            }
        }
    }

    // Clean up
    for (int i = 0; i < needleLen; ++i) {
        delete[] D[i];
        delete[] M[i];
    }

    delete[] D;
    delete[] M;

    return result;
}

double scoreFunction(const QString &needle, const QString &haystack)
{
    return score(needle, haystack, nullptr);
}

// Legacy C-string wrapper for SQLite callback
double scoreFunction(const char *needle, const char *haystack)
{
    return scoreFunction(QString::fromUtf8(needle), QString::fromUtf8(haystack));
}

} // namespace Fuzzy
} // namespace Util
} // namespace Zeal
