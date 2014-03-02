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

private:
	QMap<QString, QString> mArgList;
};

#endif // ARGUMENTS_H
