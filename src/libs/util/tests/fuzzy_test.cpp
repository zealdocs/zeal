// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fuzzy.h"

#include <QtTest>

#include <limits>

using namespace Zeal::Util::Fuzzy;

class FuzzyTest : public QObject
{
    Q_OBJECT

private slots:
    void testEmptyStrings();
    void testExactMatch();
    void testExactMatchAtWordBoundary();
    void testFuzzyMatch();
    void testPositionsFuzzy();
    void testPositionsExact();
    void testCamelCase();
    void testUnicode();
    void testPubTrimIssue();
    void testPubProtIssue();
    void testNoMatch();
    void testScoreOrdering();

    // Edge cases
    void testSingleCharacter();
    void testNeedleLongerThanHaystack();
    void testEqualLengthNonMatching();
    void testSpecialCharacters();
    void testMaxLength();

    // Bonus verification
    void testSlashBonus();
    void testBackslashBonus();
    void testColonBonus();
    void testDotBonus();
    void testWordBoundaryBonuses();
    void testConsecutiveBonus();

    // Negative scores
    void testManyGapsNegativeScore();

    // Position handling
    void testPositionsClearedOnNoMatch();
    void testPositionsForInfinityScore();

    // Return values
    void testInfinityForEqualLengthMatch();
    void testNegativeInfinityScenarios();

    // Space handling
    void testSpaceHandling();

    // Backtracking regression tests
    void testBacktrackingPrefixConflict();
    void testBacktrackingWordBoundaryWins();
};

void FuzzyTest::testEmptyStrings()
{
    // Empty needle or haystack should return -infinity
    QCOMPARE(score(QString(), QStringLiteral("test")), -std::numeric_limits<double>::infinity());
    QCOMPARE(score(QStringLiteral("test"), QString()), -std::numeric_limits<double>::infinity());
    QCOMPARE(score(QString(), QString()), -std::numeric_limits<double>::infinity());
}

void FuzzyTest::testExactMatch()
{
    // Exact match at start should score highest
    double scoreStart = score(QStringLiteral("test"), QStringLiteral("test"));
    double scoreMiddle = score(QStringLiteral("test"), QStringLiteral("prefix_test"));
    double scoreEnd = score(QStringLiteral("test"), QStringLiteral("prefix_middle_test"));

    QVERIFY(scoreStart > 0);
    QVERIFY(scoreMiddle > 0);
    QVERIFY(scoreEnd > 0);

    // Start should score higher than middle or end
    QVERIFY(scoreStart > scoreMiddle);
    QVERIFY(scoreMiddle > scoreEnd);
}

void FuzzyTest::testExactMatchAtWordBoundary()
{
    // Match after word boundary should score higher than match in middle of word
    double scoreWordBoundary = score(QStringLiteral("trim"), QStringLiteral("Publisher.prototype.toTrim"));
    double scoreMiddle = score(QStringLiteral("trim"), QStringLiteral("sometrimword"));

    QVERIFY(scoreWordBoundary > 0);
    QVERIFY(scoreMiddle > 0);
    QVERIFY(scoreWordBoundary > scoreMiddle);
}

void FuzzyTest::testFuzzyMatch()
{
    // Fuzzy match should still work
    double fuzzyScore = score(QStringLiteral("abc"), QStringLiteral("aXbXc"));
    QVERIFY(fuzzyScore > 0);

    // Consecutive match should score higher than non-consecutive
    double consecutive = score(QStringLiteral("abc"), QStringLiteral("abc"));
    double nonConsecutive = score(QStringLiteral("abc"), QStringLiteral("aXbXc"));
    QVERIFY(consecutive > nonConsecutive);
}

void FuzzyTest::testPositionsFuzzy()
{
    QVector<int> positions;
    double fuzzyScore = score(QStringLiteral("abc"), QStringLiteral("aXbXc"), &positions);

    QVERIFY(fuzzyScore > 0);
    QCOMPARE(positions.size(), 3);
    QCOMPARE(positions[0], 0); // 'a'
    QCOMPARE(positions[1], 2); // 'b'
    QCOMPARE(positions[2], 4); // 'c'
}

void FuzzyTest::testPositionsExact()
{
    QVector<int> positions;
    double exactScore = score(QStringLiteral("test"), QStringLiteral("prefix_test"), &positions);

    QVERIFY(exactScore > 0);
    QCOMPARE(positions.size(), 4);
    QCOMPARE(positions[0], 7);  // 't'
    QCOMPARE(positions[1], 8);  // 'e'
    QCOMPARE(positions[2], 9);  // 's'
    QCOMPARE(positions[3], 10); // 't'
}

void FuzzyTest::testCamelCase()
{
    // Should detect camelCase word boundaries
    QVector<int> positions;
    double camelScore = score(QStringLiteral("path"), QStringLiteral("HasPermissionForPath"), &positions);

    QVERIFY(camelScore > 0);
    QCOMPARE(positions.size(), 4);
    // Should match 'Path' at the end, not scattered through the string
    QCOMPARE(positions[0], 16); // 'P'
    QCOMPARE(positions[1], 17); // 'a'
    QCOMPARE(positions[2], 18); // 't'
    QCOMPARE(positions[3], 19); // 'h'
}

void FuzzyTest::testUnicode()
{
    // Should handle Unicode correctly
    double unicodeScore = score(QStringLiteral("café"), QStringLiteral("café"));
    QVERIFY(unicodeScore > 0);

    // Case-insensitive Unicode
    double caseScore = score(QStringLiteral("café"), QStringLiteral("CAFÉ"));
    QVERIFY(caseScore > 0);
}

void FuzzyTest::testPubTrimIssue()
{
    // Regression test: "pubtrim" should highlight Publisher + toTrim, not prototype
    QVector<int> positions;
    score(QStringLiteral("pubtrim"), QStringLiteral("Publisher.prototype.toTrim"), &positions);

    QCOMPARE(positions.size(), 7);
    // Should match "Pub" from Publisher
    QCOMPARE(positions[0], 0); // 'P'
    QCOMPARE(positions[1], 1); // 'u'
    QCOMPARE(positions[2], 2); // 'b'
    // Should match "trim" from toTrim, NOT from prototype
    QVERIFY(positions[3] >= 20); // 't' should be in "toTrim" (position 20+)
}

void FuzzyTest::testPubProtIssue()
{
    // Regression test: "pubprot" should highlight Publisher + prototype
    QVector<int> positions;
    score(QStringLiteral("pubprot"), QStringLiteral("Publisher.prototype.fieldsToTrim"), &positions);

    QCOMPARE(positions.size(), 7);
    // Should match "Pub" from Publisher
    QCOMPARE(positions[0], 0); // 'P'
    QCOMPARE(positions[1], 1); // 'u'
    QCOMPARE(positions[2], 2); // 'b'
    // Should match "prot" from prototype (around position 10-14), not from ToTrim
    QVERIFY(positions[3] >= 10 && positions[3] <= 14);
    QVERIFY(positions[6] >= 10 && positions[6] <= 14);
}

void FuzzyTest::testNoMatch()
{
    // No match should return -infinity
    QCOMPARE(score(QStringLiteral("xyz"), QStringLiteral("abc")), -std::numeric_limits<double>::infinity());
}

void FuzzyTest::testScoreOrdering()
{
    // Scores should order results correctly
    double score1 = score(QStringLiteral("array"), QStringLiteral("Array"));
    double score2 = score(QStringLiteral("array"), QStringLiteral("ArrayList"));
    double score3 = score(QStringLiteral("array"), QStringLiteral("someArrayMethod"));

    // Exact match at start > camelCase word boundary > middle of word
    QVERIFY(score1 > score2);
    QVERIFY(score2 > score3);
}

// ============================================================================
// Edge Cases
// ============================================================================

void FuzzyTest::testSingleCharacter()
{
    // Single character needle should work
    double s = score(QStringLiteral("a"), QStringLiteral("abc"));
    QVERIFY(s > 0);

    // Single character exact match
    double exact = score(QStringLiteral("a"), QStringLiteral("a"));
    QCOMPARE(exact, std::numeric_limits<double>::infinity());

    // Single character no match
    double noMatch = score(QStringLiteral("z"), QStringLiteral("abc"));
    QCOMPARE(noMatch, -std::numeric_limits<double>::infinity());

    // Single character in haystack
    double singleHay = score(QStringLiteral("ab"), QStringLiteral("a"));
    QCOMPARE(singleHay, -std::numeric_limits<double>::infinity());
}

void FuzzyTest::testNeedleLongerThanHaystack()
{
    // Needle longer than haystack should return -infinity
    double s = score(QStringLiteral("longstring"), QStringLiteral("short"));
    QCOMPARE(s, -std::numeric_limits<double>::infinity());
}

void FuzzyTest::testEqualLengthNonMatching()
{
    // Equal length strings that don't match should return -infinity
    double s = score(QStringLiteral("abcd"), QStringLiteral("efgh"));
    QCOMPARE(s, -std::numeric_limits<double>::infinity());

    // Case-insensitive equal length match should return infinity
    double match = score(QStringLiteral("test"), QStringLiteral("TEST"));
    QCOMPARE(match, std::numeric_limits<double>::infinity());
}

void FuzzyTest::testSpecialCharacters()
{
    // Should handle special characters
    QVector<int> positions;
    double s = score(QStringLiteral("a-b"), QStringLiteral("foo-a-bar-b"), &positions);
    QVERIFY(s > 0);
    QCOMPARE(positions.size(), 3);

    // Underscores
    double underscore = score(QStringLiteral("ab"), QStringLiteral("a_b"));
    QVERIFY(underscore > 0);

    // Spaces
    double space = score(QStringLiteral("ab"), QStringLiteral("a b"));
    QVERIFY(space > 0);
}

void FuzzyTest::testMaxLength()
{
    // FZY_MAX_LEN (1024) gates the DP algorithm only. Equal-length strings use
    // a simple strcmp shortcut and correctly return infinity regardless of length.
    // Use unequal lengths to exercise the DP length guard.
    QString longNeedle(1025, 'a');
    QString longHaystack(1026, 'a');

    double s = score(longNeedle, longHaystack);
    QCOMPARE(s, -std::numeric_limits<double>::infinity());

    // Equal-length exact match at the limit should work
    QString maxNeedle(1024, 'a');
    QString maxHaystack(1024, 'a');
    double atLimit = score(maxNeedle, maxHaystack);
    QCOMPARE(atLimit, std::numeric_limits<double>::infinity());
}

// ============================================================================
// Bonus Verification
// ============================================================================

void FuzzyTest::testSlashBonus()
{
    // Character after '/' should get SCORE_MATCH_SLASH bonus
    double afterSlash = score(QStringLiteral("test"), QStringLiteral("path/test"));
    double noSlash = score(QStringLiteral("test"), QStringLiteral("path_test"));

    QVERIFY(afterSlash > 0);
    QVERIFY(noSlash > 0);
    // Slash bonus should score higher than underscore (word boundary)
    QVERIFY(afterSlash > noSlash);
}

void FuzzyTest::testBackslashBonus()
{
    // Character after '\\' should get SCORE_MATCH_SLASH bonus (Windows path support)
    double afterBackslash = score(QStringLiteral("test"), QStringLiteral("path\\test"));
    double noBackslash = score(QStringLiteral("test"), QStringLiteral("path_test"));

    QVERIFY(afterBackslash > 0);
    QVERIFY(noBackslash > 0);
    // Backslash bonus should equal slash bonus
    QVERIFY(afterBackslash > noBackslash);
}

void FuzzyTest::testColonBonus()
{
    // Character after ':' should get SCORE_MATCH_DOT bonus (namespace delimiter)
    double afterColon = score(QStringLiteral("method"), QStringLiteral("Class:method"));
    double noColon = score(QStringLiteral("method"), QStringLiteral("Classmethod"));

    QVERIFY(afterColon > 0);
    QVERIFY(noColon > 0);
    // Colon bonus should score higher than no bonus
    QVERIFY(afterColon > noColon);
}

void FuzzyTest::testDotBonus()
{
    // Character after '.' should get SCORE_MATCH_DOT bonus
    double afterDot = score(QStringLiteral("ext"), QStringLiteral("file.ext"));
    double noDot = score(QStringLiteral("ext"), QStringLiteral("fileext"));

    QVERIFY(afterDot > 0);
    QVERIFY(noDot > 0);
    QVERIFY(afterDot > noDot);
}

void FuzzyTest::testWordBoundaryBonuses()
{
    // Test different word boundary bonuses in order
    // Slash (0.9) > Word boundary (0.8, underscore/dash/space) > Dot (0.6)
    double slashScore = score(QStringLiteral("t"), QStringLiteral("a/t"));
    double underscoreScore = score(QStringLiteral("t"), QStringLiteral("a_t"));
    double dashScore = score(QStringLiteral("t"), QStringLiteral("a-t"));
    double spaceScore = score(QStringLiteral("t"), QStringLiteral("a t"));
    double dotScore = score(QStringLiteral("t"), QStringLiteral("a.t"));

    // Slash should be highest
    QVERIFY(slashScore > underscoreScore);
    QVERIFY(slashScore > dotScore);

    // Word boundaries (underscore, dash, space) should be equal and higher than dot
    QVERIFY(underscoreScore > dotScore);
    QVERIFY(dashScore > dotScore);
    QVERIFY(spaceScore > dotScore);
}

void FuzzyTest::testConsecutiveBonus()
{
    // Consecutive matches should score higher than non-consecutive
    double consecutive = score(QStringLiteral("abc"), QStringLiteral("abc"));
    double withGaps = score(QStringLiteral("abc"), QStringLiteral("axbxc"));

    QVERIFY(consecutive > 0);
    QVERIFY(withGaps > 0);
    QVERIFY(consecutive > withGaps);
}

// ============================================================================
// Negative Scores
// ============================================================================

void FuzzyTest::testManyGapsNegativeScore()
{
    // The first haystack character always receives SCORE_MATCH_SLASH (0.9) because
    // precomputeBonus initialises lastCh to '/'. Each inner gap costs -0.01, so
    // 90+ gap characters are needed before the total score goes negative.
    const QString haystack = QStringLiteral("a") + QString(50, 'x')
                             + QStringLiteral("b") + QString(50, 'x')
                             + QStringLiteral("c");
    double manyGaps = score(QStringLiteral("abc"), haystack);

    // Should still match but with negative score
    QVERIFY(manyGaps < 0);
    QVERIFY(manyGaps > -std::numeric_limits<double>::infinity());
}

// ============================================================================
// Position Handling
// ============================================================================

void FuzzyTest::testPositionsClearedOnNoMatch()
{
    QVector<int> positions;
    positions.append(1);
    positions.append(2);
    positions.append(3);

    // No match should clear positions
    double s = score(QStringLiteral("xyz"), QStringLiteral("abc"), &positions);

    QCOMPARE(s, -std::numeric_limits<double>::infinity());
    QCOMPARE(positions.size(), 0);
}

void FuzzyTest::testPositionsForInfinityScore()
{
    QVector<int> positions;

    // Equal length exact match should return infinity and fill positions sequentially
    double s = score(QStringLiteral("test"), QStringLiteral("test"), &positions);

    QCOMPARE(s, std::numeric_limits<double>::infinity());
    QCOMPARE(positions.size(), 4);
    QCOMPARE(positions[0], 0);
    QCOMPARE(positions[1], 1);
    QCOMPARE(positions[2], 2);
    QCOMPARE(positions[3], 3);
}

// ============================================================================
// Return Values
// ============================================================================

void FuzzyTest::testInfinityForEqualLengthMatch()
{
    // Exact case-insensitive match of equal length strings should return infinity
    QCOMPARE(score(QStringLiteral("test"), QStringLiteral("test")),
             std::numeric_limits<double>::infinity());
    QCOMPARE(score(QStringLiteral("TeSt"), QStringLiteral("test")),
             std::numeric_limits<double>::infinity());
    QCOMPARE(score(QStringLiteral("TEST"), QStringLiteral("test")),
             std::numeric_limits<double>::infinity());
}

void FuzzyTest::testNegativeInfinityScenarios()
{
    // All scenarios that should return -infinity

    // Empty strings
    QCOMPARE(score(QString(), QStringLiteral("test")),
             -std::numeric_limits<double>::infinity());
    QCOMPARE(score(QStringLiteral("test"), QString()),
             -std::numeric_limits<double>::infinity());
    QCOMPARE(score(QString(), QString()),
             -std::numeric_limits<double>::infinity());

    // No match
    QCOMPARE(score(QStringLiteral("xyz"), QStringLiteral("abc")),
             -std::numeric_limits<double>::infinity());

    // Needle longer than haystack
    QCOMPARE(score(QStringLiteral("longer"), QStringLiteral("ab")),
             -std::numeric_limits<double>::infinity());

    // Equal length non-matching
    QCOMPARE(score(QStringLiteral("abcd"), QStringLiteral("efgh")),
             -std::numeric_limits<double>::infinity());

    // Strings too long (> FZY_MAX_LEN = 1024) — use unequal lengths to reach
    // the DP length guard (equal-length match shortcuts to +infinity)
    QString tooLong(1025, 'a');
    QString tooLongHaystack(1026, 'a');
    QCOMPARE(score(tooLong, tooLongHaystack),
             -std::numeric_limits<double>::infinity());
}

// ============================================================================
// Space Handling
// ============================================================================

void FuzzyTest::testSpaceHandling()
{
    // Spaces are treated as literal characters in fzy algorithm
    // They must match exactly and give word boundary bonus to the next character

    QVector<int> positions;

    // Space in needle matches space in haystack
    double spaceMatch = score(QStringLiteral("a b"), QStringLiteral("a b"), &positions);
    QCOMPARE(spaceMatch, std::numeric_limits<double>::infinity()); // Exact match
    QCOMPARE(positions.size(), 3);
    QCOMPARE(positions[0], 0); // 'a'
    QCOMPARE(positions[1], 1); // ' '
    QCOMPARE(positions[2], 2); // 'b'

    // Space in needle does NOT match underscore in haystack
    double noMatch = score(QStringLiteral("a b"), QStringLiteral("a_b"));
    QCOMPARE(noMatch, -std::numeric_limits<double>::infinity());

    // Character after space gets word boundary bonus.
    // Note: fzy scores can be slightly negative for valid matches due to leading
    // gap penalties — test relative ordering, not absolute sign.
    double afterSpace = score(QStringLiteral("b"), QStringLiteral("a b"));
    double noBonus = score(QStringLiteral("b"), QStringLiteral("ab"));
    QVERIFY(afterSpace > -std::numeric_limits<double>::infinity());
    QVERIFY(noBonus   > -std::numeric_limits<double>::infinity());
    QVERIFY(afterSpace > noBonus); // Space gives bonus

    // Fuzzy match with space - space must be present
    double fuzzyWithSpace = score(QStringLiteral("a c"), QStringLiteral("a b c"));
    QVERIFY(fuzzyWithSpace > 0); // Matches 'a' at 0, 'c' at 4

    positions.clear();
    score(QStringLiteral("a c"), QStringLiteral("a b c"), &positions);
    QCOMPARE(positions.size(), 3);
    QCOMPARE(positions[0], 0); // 'a'
    QCOMPARE(positions[1], 1); // ' ' (first space — 'a'+'space' are consecutive)
    QCOMPARE(positions[2], 4); // 'c'

    // Space in needle cannot match non-space in haystack
    double noSpaceMatch = score(QStringLiteral("a b"), QStringLiteral("abc"));
    QCOMPARE(noSpaceMatch, -std::numeric_limits<double>::infinity());
}

// ============================================================================
// Backtracking Regression Tests
// ============================================================================

void FuzzyTest::testBacktrackingPrefixConflict()
{
    // Bug: "string" in "str::to_string" was highlighting "st" + "ring" instead
    // of the complete "string" at the word boundary after '_'.
    //
    // Root cause: backtracking used M[i][j] instead of D[i][j] to determine
    // matchRequired. M[i][j] reflects the global prefix optimum (s,t,r at 0,1,2
    // decaying via gaps), not the path being backtracked, so matchRequired
    // incorrectly dropped to false and the algorithm fell back to j=0,1.
    QVector<int> positions;
    score(QStringLiteral("string"), QStringLiteral("str::to_string"), &positions);

    QCOMPARE(positions.size(), 6);
    // Must be the consecutive "string" from "to_string", not split across the string
    QCOMPARE(positions[0], 8);  // 's'
    QCOMPARE(positions[1], 9);  // 't'
    QCOMPARE(positions[2], 10); // 'r'
    QCOMPARE(positions[3], 11); // 'i'
    QCOMPARE(positions[4], 12); // 'n'
    QCOMPARE(positions[5], 13); // 'g'
}

void FuzzyTest::testBacktrackingWordBoundaryWins()
{
    // Bug: "string" in "Static String" was highlighting "S" + "tring" instead
    // of the complete "String" word (positions 7-12).
    //
    // The 'S' at position 0 (slash bonus 0.9) builds up an M score that is
    // higher than D at position 7 (word boundary bonus 0.8), causing the same
    // matchRequired bug to drop false and fall back to j=0 for the first char.
    QVector<int> positions;
    score(QStringLiteral("string"), QStringLiteral("Static String"), &positions);

    QCOMPARE(positions.size(), 6);
    // Must be the consecutive "String" word, not "S" from "Static" + "tring"
    QCOMPARE(positions[0], 7);  // 'S'
    QCOMPARE(positions[1], 8);  // 't'
    QCOMPARE(positions[2], 9);  // 'r'
    QCOMPARE(positions[3], 10); // 'i'
    QCOMPARE(positions[4], 11); // 'n'
    QCOMPARE(positions[5], 12); // 'g'
}

QTEST_MAIN(FuzzyTest)
#include "fuzzy_test.moc"
