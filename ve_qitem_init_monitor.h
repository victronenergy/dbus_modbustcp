#ifndef VEQITEMINITMONITOR_H
#define VEQITEMINITMONITOR_H

#include <QObject>
#include <QList>

class VeQItem;

/**
 * Monitors one or more VeQItems for initialization.
 *
 * This class monitors the state of a set of VeQItems and emits the `initialized` signal when none
 * of the items has the Idle or Requested state. Use this class if you need several VeQItem to
 * perform an operation, and you need the values of all of the items.
 *
 * Usage:
 * - Create a VeQItemInitMonitor instance
 * - pass all items to be monitored to the `addItem` function
 * - connect to the `initialized` signal
 * - call the `start` function.
 *
 * Alternatively you can call one of the static `monitor` functions which will to the same.
 */
class VeQItemInitMonitor : public QObject
{
	Q_OBJECT
public:
	VeQItemInitMonitor(QObject *parent = 0);

	/**
	 * Add an item to be monitored.
	 * @param item The item to be monitored. If the item is not a leaf, all leaf items beflow this
	 * item will be added instead.
	 */
	void addItem(VeQItem *item);

	/**
	 * Start monitoring the items passed to `addItem`.
	 *
	 * If all items have been initialized, the`initialized` signal will be emitted. If all items
	 * have already been initialized, the signal will be emitted during the function call.
	 *
	 * The `getValue` function will be called on all items that have the `VeQItem::Idle` state.
	 */
	void start();

	/**
	 * Checks the current state of the items.
	 *
	 * The `initialized` signal will not be emitted in this function.
	 * @return True if all items have been initialized
	 */
	bool checkState();

	/**
	 * Monitors one or more items.
	 *
	 * This is a convenience function: it will setup monitoring of `item`.
	 * @param item The item to be monitored (or the leafs in its subtree if the item is not	a leaf).
	 * @param dest The instance that will be notified when the item is initialized.
	 * @param slot The slot that will be called when the item is initialized.
	 * @param connectionType See `QObject::connect`.
	 */
	static void monitor(VeQItem *item, QObject *dest, const char *slot,
						Qt::ConnectionType connectionType = Qt::AutoConnection);

	/**
	 * Works like to other `monitor` function. This function accepts multiple VeQItems.
	 */
	static void monitor(const QList<VeQItem *> &items, QObject *dest, const char *slot,
						Qt::ConnectionType connectionType = Qt::AutoConnection);

signals:
	/**
	 * This signal is emitted when all items have been initialied.
	 *
	 * An item are considered initialized if its state is not Idle or Requested. So if the state
	 * is offline, an item is considered initialized.
	 * Do not delete the `VeQItemInitMonitor` instance in the slot connected to this signal,
	 * use the `deleteLater` function instead.
	 */
	void initialized();

private slots:
	void onStateChanged();

private:
	void updateState();

	QList<VeQItem *> mItems;
};

#endif // VEQITEMINITMONITOR_H
