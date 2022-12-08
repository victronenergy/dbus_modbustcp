#ifndef _LOGGING_H
#define _LOGGING_H

// Some logging routines to make this compatible with QT4, 5 and 6.

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)

#include <QTextStream>

// This is a quick and dirty replacement for qInfo on QT4.
class qInfo : public QTextStream
{
public:
	qInfo() : QTextStream(stderr) {
	}

	template<typename T> qInfo& operator<<(const T &s)
	{
		QTextStream::operator<<(s) << " ";
		return *this;
	}

	~qInfo()
	{
		*this << "\n";
		flush();
	}
};
#endif
#endif
