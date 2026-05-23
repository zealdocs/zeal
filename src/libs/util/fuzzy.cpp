// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fuzzy.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace Zeal::Util::Fuzzy {

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
    const int haystackLen = static_cast<int>(haystack.length());

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

double score(const QString &needle, const QString &haystack, QList<int> *positions)
{
    // Pre-filter: check if all needle characters exist in haystack (performance optimization)
    // This avoids expensive DP computation on unmatchable strings
    if (!needle.isEmpty() && !haystack.isEmpty() && hasMatch(needle, haystack)) {
        return computeScore(needle, haystack, positions);
    }

    // No match - return -infinity (SQL will filter with WHERE score > 0)
    if (positions != nullptr) {
        positions->clear();
    }

    return -std::numeric_limits<double>::infinity();
}

// ============================================================================
// Low-level API implementation
// ============================================================================

double computeScore(const QString &needle, const QString &haystack, QList<int> *positions)
{
    const int needleLen = static_cast<int>(needle.length());
    const int haystackLen = static_cast<int>(haystack.length());

    if (needleLen == 0 || haystackLen == 0 || needleLen > haystackLen) {
        if (positions != nullptr) {
            positions->clear();
        }

        return -std::numeric_limits<double>::infinity();
    }

    if (needleLen == haystackLen) {
        // Equal length strings get infinity score only if they actually match (case-insensitive)
        if (needle.compare(haystack, Qt::CaseInsensitive) == 0) {
            if (positions != nullptr) {
                // Fill positions for exact match: [0, 1, 2, ..., n-1]
                positions->resize(needleLen);
                for (int i = 0; i < needleLen; ++i) {
                    (*positions)[i] = i;
                }
            }

            return std::numeric_limits<double>::infinity();
        }

        // Otherwise return no match - equal length non-matching strings can't fuzzy match
        if (positions != nullptr) {
            positions->clear();
        }

        return -std::numeric_limits<double>::infinity();
    }

    if (haystackLen > FZY_MAX_LEN || needleLen > FZY_MAX_LEN) {
        if (positions != nullptr) {
            positions->clear();
        }

        return -std::numeric_limits<double>::infinity();
    }

    double matchBonus[FZY_MAX_LEN] = {}; // Zero-initialize to satisfy static analyzer
    precomputeBonus(haystack, matchBonus);

    const double SCORE_MIN = -std::numeric_limits<double>::infinity();

    // One flat buffer reused across calls on this thread; layout [D rows | M rows].
    // Grows monotonically; bounded by 2 * FZY_MAX_LEN^2 doubles.
    static thread_local std::vector<double> dp;
    const std::size_t cells = static_cast<std::size_t>(needleLen) * haystackLen;
    if (dp.size() < 2 * cells) {
        dp.resize(2 * cells);
    }
    double *const D = dp.data();
    double *const M = dp.data() + cells;

    // Lower the haystack once instead of once per needle row.
    char16_t hsLower[FZY_MAX_LEN];
    for (int j = 0; j < haystackLen; ++j) {
        hsLower[j] = haystack[j].toLower().unicode();
    }

    auto idx = [haystackLen](int i, int j) {
        return (i * haystackLen) + j;
    };

    // Forward pass: compute scores
    for (int i = 0; i < needleLen; ++i) {
        double prevScore = SCORE_MIN;
        const double gapScore = (i == needleLen - 1) ? SCORE_GAP_TRAILING : SCORE_GAP_INNER;

        const char16_t needleCh = needle[i].toLower().unicode();

        for (int j = 0; j < haystackLen; ++j) {
            if (needleCh == hsLower[j]) {
                double score = SCORE_MIN;

                if (i == 0) {
                    score = (j * SCORE_GAP_LEADING) + matchBonus[j];
                } else if (j > 0) {
                    const double prevM = M[idx(i - 1, j - 1)];
                    const double prevD = D[idx(i - 1, j - 1)];
                    score = std::max(prevM + matchBonus[j], prevD + SCORE_MATCH_CONSECUTIVE);
                }

                D[idx(i, j)] = score;
                M[idx(i, j)] = prevScore = std::max(score, prevScore + gapScore);
            } else {
                D[idx(i, j)] = SCORE_MIN;
                M[idx(i, j)] = prevScore = prevScore + gapScore;
            }
        }
    }

    const double result = M[idx(needleLen - 1, haystackLen - 1)];

    // Backtrack to find positions if requested (fzy algorithm)
    // Only backtrack if we have a valid match (not SCORE_MIN)
    if (positions != nullptr && result != SCORE_MIN) {
        positions->resize(needleLen);
        bool matchRequired = false;

        for (int i = needleLen - 1, j = haystackLen - 1; i >= 0; --i) {
            for (; j >= 0; --j) {
                const double dij = D[idx(i, j)];
                // Check if this is a valid match position on the optimal path
                if (dij != SCORE_MIN && (matchRequired || dij == M[idx(i, j)])) {
                    // Check if we used consecutive match bonus to get here.
                    // Use D (score at this specific position), not M
                    // (global prefix optimum), which may reflect a different path entirely.
                    matchRequired = (i > 0 && j > 0 && dij == D[idx(i - 1, j - 1)] + SCORE_MATCH_CONSECUTIVE);
                    (*positions)[i] = j;
                    --j;
                    break;
                }
            }
        }
    }

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

} // namespace Zeal::Util::Fuzzy
