#ifndef HOVERPANEL_HPP
#define HOVERPANEL_HPP

#include <QVector>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QAbstractItemView>
#include <QModelIndex>

namespace Zeal {

class HoverButton : public QObject
{
	Q_OBJECT
public:
	HoverButton(QIcon icon);
	const QIcon& icon() const;
private:
	QIcon m_icon;
signals:
	void pressed(const QModelIndex& index);
};

class HoverPanel
{
public:
	HoverPanel(const QAbstractItemView& relative_widget);
	void paint(QPainter* painter, const QStyleOptionViewItem& option) const;
	void addButton(QSharedPointer<HoverButton> button);
	bool handlePressed(const QModelIndex& index);
private:
	const QAbstractItemView& m_relativeWidget;
	QVector<QSharedPointer<HoverButton>> m_Buttons;
};

} // namespace Zeal

#endif // HOVERPANEL_HPP
