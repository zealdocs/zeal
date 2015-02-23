#ifndef ZRELEVANCY_H
#define ZRELEVANCY_H

#include "searchresult.h"

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1;

namespace Zeal {

class ZRelevancy
{
public:
    static int encode(double relevancy, SearchResult::MatchType matchType);
    static double decodeRelevancy(int zrelevancy);
    static SearchResult::MatchType decodeMatchType(int zrelevancy);
};

extern "C" void zrelevancy(sqlite3_context* ctx, int argc, sqlite3_value** argv);

extern "C" int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

} // namespace Zeal

#endif // ZRELEVANCY_H
