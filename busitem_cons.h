#ifndef BUSITEMCONS_H
#define BUSITEMCONS_H

#include "busitem_interface.h"

class BusItemCons : public QObject
{
	Q_OBJECT

public:
	BusItemCons(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

	QVariant getValue();
	int setValue(const QVariant & val);

public slots:
	void PropertiesChanged(const QVariantMap &changes);

signals:
	void valueChanged(BusItemCons * const busitem);
	void descriptionChanged();
	void bindChanged();
	void minChanged();
	void maxChanged();
	void textChanged();
	void validChanged();

private:
	BusItemInterface* mBusItem;
//	QDBusServiceWatcher *mWatcher;

	QVariant mValue;
	bool mValueValid;

	QString mText;
	bool mTextValid;

	QString mDescription;
	bool mDescriptionValid;

	QVariant mMin;
	bool mMinValid;

	QVariant mMax;
	bool mMaxValid;

	//bool mValid;
	//bool mValidValid; // yup sound strange, if the the remote valid flag is valid
};

#endif // BUSITEMCONS_H
