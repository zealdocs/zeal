#include "docsettoken.h"

#include <QStringList>

using namespace Zeal;

DocsetToken::DocsetToken(const QString &token) :
    full(token)
{
    parseArgs(token);

    // '.' for long Django docset values like django.utils.http
    // '::' for long C++ docset values like std::set
    // '/' for long Go docset values like archive/tar
    const QStringList separators = {QStringLiteral("."), QStringLiteral("::"), QStringLiteral("/")};
    int i, maxI, pos, maxPos;
    do {
        // Find which separator would give the smallest name
        maxI = 0, maxPos = -1;
        for (i = 0; i < separators.length(); i++) {
            pos = name.lastIndexOf(separators[i]);
            if (pos > maxPos) {
                maxPos = pos;
                maxI = i;
            }
        }
        if (maxPos != -1) {
            // Never generate empty name when separator at the end like 'std::'
            if (maxPos == name.length() - separators[maxI].length()) {
                name = name.mid(0, maxPos);
            } else {
                parentName = name.mid(0, maxPos);
                separator = separators[maxI];
                name = name.mid(maxPos + separators[maxI].length());
            }
        }
    } while (!name.isEmpty() && maxPos != -1);
}

void DocsetToken::parseArgs(const QString &token)
{
    // Ensure only the last matched pair of parentheses is removed.
    // Regex can't deal with nested parenthesis.
    QString stripped = QString(token);
    int parensLeftOpen = 0;
    if (stripped.endsWith(')')) {
        for (int i = stripped.length() - 1; i >= 0; i--) {
            if (stripped[i] == QChar(')'))
                parensLeftOpen++;
            else if (stripped[i] == QChar('('))
                parensLeftOpen--;

            if (parensLeftOpen == 0) {
                if (i != 0 && stripped[i - 1] != QChar(' ')) {
                    args = stripped.mid(i);
                    name = stripped.remove(i, args.length());
                    return;
                }
                break;
            }
        }
    }
    name = stripped;
}
