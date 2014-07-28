#include "busitem_cons.h"
#define QS_LOG_DISABLE
#include "QsLog.h"

BusItemCons::BusItemCons(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent) :
	QObject(parent),
	mBusItem(0)
{
	mValueValid = false;

	mBusItem = new BusItemInterface(service, path, connection, this);
	connect(mBusItem, SIGNAL(PropertiesChanged(const QVariantMap &)), this, SLOT(PropertiesChanged(const QVariantMap &)));
	mValue = mBusItem->getValue();
	if (mValue.isValid()) {
		const int variantType = mValue.userType();
		if (qMetaTypeId<QDBusArgument>() != variantType)
			mValueValid = true;
	}
}

QVariant BusItemCons::getValue()
{
	if (mValueValid)
		return mValue;
	else
		return QVariant();
}

int BusItemCons::setValue(const QVariant & val)
{
	return mBusItem->setValue(val);
}

void BusItemCons::PropertiesChanged(const QVariantMap &changes)
{
	//QLOG_TRACE() << "[BusItemCons::PropertiesChanged]";

	QMapIterator<QString, QVariant> i(changes);

	while (i.hasNext()) {
		i.next();
		//QLOG_TRACE() << "Item: " << i.key() << ":" << i.value().toString();
		if (i.key() == "Value") {
			if (i.value().isValid()) {
				mValueValid = true;
				if (i.value() != mValue) {
					mValue = i.value();
					emit valueChanged(this);
				}
			}
		}
		//else if (i.key() == "Text") {
		//}
	}
}
