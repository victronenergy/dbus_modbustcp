#include <QCoreApplication>
#include <iostream>
#include "arguments.h"

using namespace std;

Arguments::Arguments()
{
	QStringList argList = QCoreApplication::arguments();
	QString switchName;

	mAppName = argList.at(0).section("/",-1);
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
	if (!switchName.isEmpty())
		mArgList.insert(switchName, QString());
}

void Arguments::print()
{
	QMap<QString, QString>::const_iterator i = mArgList.constBegin();
	while (i != mArgList.constEnd()) {
		cout << "key = " << i.key().toStdString() << " value = " << i.value().toStdString() << "\n";
		++i;
	}
}

void Arguments::help()
{
	cout << mAppName.toStdString() << "\n\n";
	cout << "OPTIONS:" << "\n\n";
	QMap<QString, QString>::iterator i;
	for (i = mHelp.begin(); i != mHelp.end(); ++i) {
		cout << i.key().toStdString() << "\n";
		cout << "\t" << i.value().toStdString() << "\n";
	}
	cout << endl;
}

void Arguments::addArg(const QString & arg, const QString & description)
{
	mHelp.insert(arg,description);
}
