#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <QMap>
#include <QStringList>

class Arguments
{
public:
	Arguments();

	bool contains(const QString& switchName) const { return mArgList.contains(switchName); }
	QString value(const QString &switchName) const { return mArgList.value(switchName); }

	void print();
	void help();
	void addArg(const QString & arg, const QString & description);
private:
	QString mAppName;
	QMap<QString, QString> mArgList;
	QMap<QString, QString> mHelp;
};

#endif // ARGUMENTS_H
