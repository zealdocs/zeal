#include "zrelevancy.h"
#include "docsettoken.h"
#include "core/lcs.h"

using namespace Zeal;

// To avoid recalculating lcs and get around sqlite not having table valued functions
// return both the relevancy score and match type in a single packed int, that still 
// maintains correct order. Assumes 32 bit int size and two's complement representation
// bit 0 is unused to avoid overflow if two's complement
// bits 1-29 store the relevancy score
// bits 30-31 store the match type
int ZRelevancy::encode(double relevancy, SearchResult::MatchType matchType)
{
    return (int(relevancy * 0x10000000) << 2) + matchType;
}

double ZRelevancy::decodeRelevancy(int zrelevancy)
{
    return double(zrelevancy >> 2) / 0x10000000;
}

SearchResult::MatchType ZRelevancy::decodeMatchType(int zrelevancy)
{
    return SearchResult::MatchType(zrelevancy & 0x3);
}

extern "C" void zrelevancy(sqlite3_context* ctx, int argc, sqlite3_value** argv)
{
    Q_UNUSED(argc);

    // Parameter ordering for ZRELEVANCY extension is important
    const QString rawToken = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_value_text(argv[0]))).toLower();
    const QString rawQuery = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_value_text(argv[1]))).toLower();

    SearchResult::MatchType match = SearchResult::NoMatch;
    Core::LCS nameLcs, parentNameLcs;

    DocsetToken token = DocsetToken(rawToken);

    // Attempts to match the whole query against the name, then parentName, and
    // finally both name and parentName as applied by the same parsing rules.
    nameLcs = Core::LCS(token.name(), rawQuery);
    if (nameLcs.length() > 0 && nameLcs.length() == rawQuery.length()) {
        match = SearchResult::NameMatch;
    } else {
        parentNameLcs = Core::LCS(token.parentName(), rawQuery);
        if (parentNameLcs.length() > 0 && parentNameLcs.length() == rawQuery.length()) {
            match = SearchResult::ParentNameMatch;
        } else {
            DocsetToken query = DocsetToken(rawQuery);
            nameLcs = Core::LCS(token.name(), query.name());
            parentNameLcs = Core::LCS(token.parentName(), query.parentName());
            int bothLcsLength = nameLcs.length() + parentNameLcs.length();
            if (bothLcsLength > 0 && bothLcsLength == query.name().length() + query.parentName().length()) {
                match = SearchResult::BothMatch;
            }
        }
    }

    // Normalized to 1 relevancy score heuristic bringing good results.
    double relevancy = 0;
    switch (match) {
    case SearchResult::NoMatch:
        break;
    case SearchResult::NameMatch:
        relevancy = (nameLcs.calcDensity(0) 
                     + nameLcs.calcSpread(0)) / 2.0;
        break;
    case SearchResult::ParentNameMatch:
        relevancy = (parentNameLcs.calcDensity(0) 
                     + parentNameLcs.calcSpread(0)) / 2.0;
        break;
    case SearchResult::BothMatch:
        relevancy = (nameLcs.calcDensity(0) + parentNameLcs.calcDensity(0) 
                     + nameLcs.calcSpread(0) + parentNameLcs.calcSpread(0)) / 4.0;
    }

    sqlite3_result_int(ctx, ZRelevancy::encode(relevancy, match));
}

extern "C" int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    SQLITE_EXTENSION_INIT2(pApi);
    Q_UNUSED(pzErrMsg);

    sqlite3_create_function(db, "ZRELEVANCY", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &zrelevancy, NULL, NULL);
    return 0;
}
