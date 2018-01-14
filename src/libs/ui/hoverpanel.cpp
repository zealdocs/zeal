#include "hoverpanel.h"

namespace Zeal {

HoverButton::HoverButton(QIcon icon) :
    m_icon(icon)
{
}

const QIcon &HoverButton::icon() const
{
    return m_icon;
}

HoverPanel::HoverPanel(const QAbstractItemView &relative_widget) :
        m_relativeWidget(relative_widget)
{
}

void HoverPanel::paint(QPainter *painter, const QStyleOptionViewItem &option) const
{
    auto rect = option.rect;
    for(auto& button : m_Buttons) {
        button->icon().paint(painter, rect, Qt::AlignRight);
        rect.setWidth(rect.width() - rect.height());
        if (rect.width() < 0)
            break;
    }
}

void HoverPanel::addButton(QSharedPointer<HoverButton> button)
{
    m_Buttons.push_back(button);
}

bool HoverPanel::handlePressed(const QModelIndex& index)
{
    QPoint mouse_pos = m_relativeWidget.mapFromGlobal(QCursor::pos());
    QRect itemrect = m_relativeWidget.visualRect(index);
    itemrect.setLeft(itemrect.right());
    auto icon_width = itemrect.height();
    for(auto& button : m_Buttons) {
        itemrect.setLeft(itemrect.left() - icon_width);
        if(itemrect.contains(mouse_pos)) {
            emit button->pressed(index);
            return true;
        }
    }
    return false;
}

} // namespace Zeal
