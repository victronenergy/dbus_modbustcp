#ifndef NOSTORAGE_QITEM_PRODUCER_H
#define NOSTORAGE_QITEM_PRODUCER_H

#include <velib/qt/ve_qitems_dbus.hpp>

/**
 * A VeQItemDbus that forces the item's state back to Synchronized after a call to setValue.
 * This is a workaround for the problem that a call to setValue will not trigger a SetValue on the
 * D-Bus if the previous setValue used the same value and there was no PropertiesChange from
 * the D-Bus object after the first setValue.
 */
class NoStorageQItem: public VeQItemDbus
{
	Q_OBJECT
public:
	NoStorageQItem(VeQItemDbusProducer *producer):
		VeQItemDbus(producer)
	{
	}

	virtual int setValue(QVariant const &value)
	{
		int i = VeQItemDbus::setValue(value);
		if (i != 0)
			return i;
		VeQItemDbus::setState(VeQItem::Synchronized);
		return 0;
	}
};

/**
 * A VeQItemDbusProducer that creates NoStorageQItem instead of VeQItemDbus
 */
class NostorageQItemProducer: public VeQItemDbusProducer
{
	Q_OBJECT
public:
	NostorageQItemProducer(VeQItem *root, QString id, bool findVictronServices = true,
						   bool bulkInitOfNewService = true, QObject *parent = 0):
		VeQItemDbusProducer(root, id, findVictronServices, bulkInitOfNewService, parent)
	{
	}

	virtual VeQItem *createItem()
	{
		return new NoStorageQItem(this);
	}
};


#endif // NOSTORAGE_QITEM_PRODUCER_H
