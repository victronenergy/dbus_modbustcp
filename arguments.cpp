#include "arguments.h"
#include <QCoreApplication>

//#define QS_LOG_DISABLE
#include "QsLog.h"

Arguments::Arguments()
{
	QStringList argList = QCoreApplication::arguments();
	QString switchName;

	const int listSize =argList.size();
	for (int i = 1; i < listSize; i++) {
		QString arg = argList.at(i);
		if (arg.startsWith('-')) {
			arg.remove(0,1);
			if (arg.startsWith('-'))
				arg.remove(0,1);
			if (!arg.isEmpty()) {
				if (switchName.isEmpty())
					switchName = arg;
				else {
					mArgList.insert(switchName, QString());
					switchName = arg;
				}
			}
		} else {
			mArgList.insert(switchName, arg);
			switchName.clear();
		}
	}
}

void Arguments::print()
{
	QMap<QString, QString>::const_iterator i = mArgList.constBegin();
	while (i != mArgList.constEnd()) {
		QLOG_DEBUG() << "key = " << i.key() << " value = " << i.value();
		++i;
	}
}
