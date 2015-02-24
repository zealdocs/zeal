#ifndef SEARCHRELEVANCY_H
#define SEARCHRELEVANCY_H

#include "docsettoken.h"

namespace Zeal {

struct SearchRelevancy
{
    enum MatchType {
        NoMatch = 0,
        ParentNameMatch,
        NameMatch,
        BothMatch
    };

    MatchType matchType;
    double relevancy;

    static SearchRelevancy fromQuery(const DocsetToken &token, const DocsetToken &query);
};

} // namespace Zeal

#endif // SEARCHRELEVANCY_H
