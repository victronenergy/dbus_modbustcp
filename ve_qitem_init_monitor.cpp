#include <QsLog.h>
#include <velib/qt/ve_qitem.hpp>
#include "ve_qitem_init_monitor.h"

VeQItemInitMonitor::VeQItemInitMonitor(QObject *parent):
	QObject(parent)
{
}

void VeQItemInitMonitor::addItem(VeQItem *item)
{
	if (item->isLeaf()) {
		mItems.append(item);
	} else {
		for (int i=0;;++i) {
			VeQItem *child = item->itemChild(i);
			if (child == 0)
				break;
			addItem(child);
		}
	}
}

void VeQItemInitMonitor::start()
{
	updateState();
	foreach (VeQItem *item, mItems) {
		switch (item->getState()) {
		case VeQItem::Idle:
			item->getValue();
			// Fall through
		case VeQItem::Requested:
			connect(item, SIGNAL(stateChanged(VeQItem *, State)), this, SLOT(onStateChanged()));
			break;
		default:
			// Do nothing
			break;
		}
	}
}

bool VeQItemInitMonitor::checkState()
{
	if (mItems.isEmpty())
		return true;
	foreach (VeQItem *item, mItems) {
		VeQItem::State state = item->getState();
		if (state == VeQItem::Idle || state == VeQItem::Requested)
			return false;
	}
	return true;
}

void VeQItemInitMonitor::monitor(VeQItem *item, QObject *dest, const char *slot,
								 Qt::ConnectionType connectionType)
{
	VeQItemInitMonitor *monitor = new VeQItemInitMonitor(dest);
	connect(monitor, SIGNAL(initialized()), dest, slot, connectionType);
	connect(monitor, SIGNAL(initialized()), monitor, SLOT(deleteLater()), connectionType);
	monitor->addItem(item);
	monitor->start();
}

void VeQItemInitMonitor::monitor(const QList<VeQItem *> &items, QObject *dest, const char *slot,
								 Qt::ConnectionType connectionType)
{
	VeQItemInitMonitor *monitor = new VeQItemInitMonitor(dest);
	connect(monitor, SIGNAL(initialized()), dest, slot, connectionType);
	connect(monitor, SIGNAL(initialized()), monitor, SLOT(deleteLater()), connectionType);
	foreach(VeQItem *item, items)
		monitor->addItem(item);
	monitor->start();
}

void VeQItemInitMonitor::onStateChanged()
{
	updateState();
}

void VeQItemInitMonitor::updateState()
{
	if (mItems.isEmpty() || !checkState())
		return;
	mItems.clear();
	emit initialized();
}
