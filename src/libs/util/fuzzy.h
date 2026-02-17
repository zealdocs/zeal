// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_UTIL_FUZZY_H
#define ZEAL_UTIL_FUZZY_H

#include <QString>
#include <QVector> // TODO: [Qt 6] Use QList.

namespace Zeal {
namespace Util {
namespace Fuzzy {

// Fuzzy matching is based on https://github.com/jhawthorn/fzy by John Hawthorn, MIT License.

/**
 * @brief Computes fuzzy match score for QString inputs, optionally returning match positions
 *
 * Convenience wrapper around computeScore() with pre-filtering for performance.
 * Returns raw fzy scores - caller should filter results (e.g., WHERE score > 0 in SQL).
 *
 * Score ranges:
 * - Exact matches (needle == haystack): infinity
 * - Valid fuzzy matches: unbounded; ~0.9 + (n-1) for a perfect length-n consecutive match
 * - Poor matches: negative scores (many gaps outweigh bonuses)
 * - No match: -infinity
 *
 * @param needle Search query
 * @param haystack Text to search in
 * @param positions Optional output list of matched haystack indices for highlighting
 * @return Match score (higher is better, -infinity for no match)
 */
double score(const QString &needle, const QString &haystack, QVector<int> *positions = nullptr);

/**
 * @brief Computes fuzzy match score, optionally returning match positions
 *
 * Low-level scoring function using the fzy algorithm (https://github.com/jhawthorn/fzy).
 * Uses case-insensitive matching with Unicode support via Qt. When positions are requested,
 * backtracks through the DP matrices to find the exact character positions that produced
 * the best score.
 *
 * @param needle Search query
 * @param haystack Text to search in
 * @param positions Optional output list of haystack character indices
 * @return Fuzzy match score (-infinity if no match possible, infinity if needle == haystack,
 *         otherwise unbounded: ~0.9 + (n-1) for a perfect length-n consecutive match)
 */
double computeScore(const QString &needle, const QString &haystack, QVector<int> *positions = nullptr);

/**
 * @brief Main scoring function for use in SQLite callbacks
 *
 * Thin wrapper around score() that provides fzy-based fuzzy matching.
 * Returns raw fzy scores - use WHERE score > 0 in SQL to filter results.
 *
 * Score ranges:
 * - Exact matches (needle == haystack): infinity
 * - Valid fuzzy matches: unbounded; ~0.9 + (n-1) for a perfect length-n consecutive match
 * - Poor matches: negative scores (many gaps outweigh bonuses)
 * - No match: -infinity
 *
 * @param needle Search query
 * @param haystack Text to search in
 * @return Match score (higher is better, -infinity for no match)
 */
double scoreFunction(const QString &needle, const QString &haystack);

/**
 * @brief Legacy C-string wrapper for scoreFunction
 *
 * Provided for compatibility with SQLite callbacks. Converts C strings to QString
 * and calls the QString version.
 *
 * @param needle Search query (UTF-8 C string)
 * @param haystack Text to search in (UTF-8 C string)
 * @return Match score (higher is better, -infinity for no match)
 */
double scoreFunction(const char *needle, const char *haystack);

} // namespace Fuzzy
} // namespace Util
} // namespace Zeal

#endif // ZEAL_UTIL_FUZZY_H
