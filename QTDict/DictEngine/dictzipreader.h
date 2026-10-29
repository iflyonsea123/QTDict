#ifndef DICTZIPREADER_H
#define DICTZIPREADER_H

#include <QByteArray>
#include <QString>

class DictZipReader
{
public:
    static bool decompress(const QByteArray &compressed, QByteArray *uncompressed,
                           QString *errorMessage = nullptr);
};

#endif // DICTZIPREADER_H
