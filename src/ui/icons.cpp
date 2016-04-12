#include "icons.h"

#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QSize>

// Unicode encodings for different ionicon icons.
const QString Icons::Back = QString("\uf3cf");
const QString Icons::Forward = QString("\uf3d1");
const QString Icons::OpenLink = QString("\uf39c");
const QString Icons::Plus = QString("\uf218");
const QString Icons::CloseCircled = QString("\uf128");
const QString Icons::CloseRound = QString("\uf129");
const QString Icons::Close = QString("\uf12a");
const QString Icons::Search = QString("\uf2f5");
const QString Icons::DocumentText = QString("\uf12e");
const QString Icons::WorldOutline = QString("\uf4d2");

/**
 * @brief Icons::getIcon
 * Converts a text into an icon.
 *
 * @param text Text of the icon.
 */
QIcon Icons::getIcon(const QString text)
{
    static int iconSize = 32;
    QFont font("Ionicons", iconSize);
    QPainter painter;

    QSize pmSize = QSize(iconSize, iconSize);
    QRect rect(QPoint(0, 0), pmSize);
    QPixmap pm(pmSize);
    pm.fill(Qt::transparent);

    painter.begin(&pm);
    painter.setFont(font);
    painter.drawText(rect, Qt::AlignCenter | Qt::AlignHCenter, text);
    painter.end();

    return QIcon(pm);
}
