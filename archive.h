#ifndef FILAMENT_ARCHIVE_H
#define FILAMENT_ARCHIVE_H

#include <QDate>
#include <QList>
#include <QStringList>

namespace Archive {
enum Kind { Filament, Tubes, On, Off };
struct Paths {
    QStringList directories[4];
    Paths();
};
struct TubeRecord {
    QString removedSerial, installedSerial;
    QDate removedSince, installedSince;
};
struct Day {
    QDate date;
    QString path, error;
    qint64 seconds;
    Day() : seconds(0) {}
};
struct Period {
    QDate start, end;
    qint64 seconds;
    int expectedDays, validDays, missingDays, invalidDays;
    bool known;
    Period() : seconds(0), expectedDays(0), validDays(0), missingDays(0),
               invalidDays(0), known(false) {}
};
struct Report {
    TubeRecord record;
    QString snapshotPath, error;
    Period removed, installed;
    QList<Day> days;
};
QString fileName(Kind kind, const QDate &date);
QString resolve(Kind kind, const QDate &date, const Paths &paths);
bool readOn(const QString &path, const QString &device, qint64 *seconds, QString *error);
Report load(const QString &device, const QString &position, const QDate &asOf, const Paths &paths);
QString duration(qint64 seconds);
}
#endif
