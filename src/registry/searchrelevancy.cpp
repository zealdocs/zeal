#include "searchrelevancy.h"
#include "core/lcs.h"

using namespace Zeal;

SearchRelevancy SearchRelevancy::fromQuery(const DocsetToken &token, const DocsetToken &query)
{
    MatchType matchType = NoMatch;
    double relevancy = 0;

    if (query.full.isEmpty())
        return SearchRelevancy{matchType, relevancy};

    DocsetToken tokenLower(token.full.toLower());

    Core::LCS nameLcs, parentNameLcs;
    // Attempts to match the whole query against the name, then parentName, and
    // finally both name and parentName as applied by the same parsing rules.
    nameLcs = Core::LCS(tokenLower.name, query.full);
    if (nameLcs.length() > 0 && nameLcs.length() == query.full.length()) {
        matchType = NameMatch;
    } else {
        parentNameLcs = Core::LCS(tokenLower.parentName, query.full);
        if (parentNameLcs.length() > 0 && parentNameLcs.length() == query.full.length()) {
            matchType = ParentNameMatch;
        } else {
            nameLcs = Core::LCS(tokenLower.name, query.name);
            parentNameLcs = Core::LCS(tokenLower.parentName, query.parentName);
            int bothLcsLength = nameLcs.length() + parentNameLcs.length();
            if (bothLcsLength > 0 && bothLcsLength == query.name.length() + query.parentName.length()) {
                matchType = BothMatch;
            }
        }
    }

    // Normalized to 1 relevancy score heuristic bringing good results.
    switch (matchType) {
    case NoMatch:
        break;
    case NameMatch:
        relevancy = (nameLcs.calcDensity(0) 
                    + nameLcs.calcSpread(0)) / 2.0;
        break;
    case ParentNameMatch:
        relevancy = (parentNameLcs.calcDensity(0) 
                    + parentNameLcs.calcSpread(0)) / 2.0;
        break;
    case BothMatch:
        relevancy = (nameLcs.calcDensity(0) + parentNameLcs.calcDensity(0) 
                    + nameLcs.calcSpread(0) + parentNameLcs.calcSpread(0)) / 4.0;
        // slight boosting (10%) of both matches due to removal of separator 
        // affecting density, but never boost above near perfect matches (95%)
        relevancy = relevancy != 1 ? std::min(0.95, relevancy * 1.1) : 1;
    }

    return SearchRelevancy{matchType, relevancy};
}
