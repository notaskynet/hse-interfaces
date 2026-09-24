#include "asciiformat.h"

#include <QObject>

QString AsciiFormat::name() const
{
    return "ASCII";
}

QString AsciiFormat::inputHint() const
{
    return QObject::tr("Текст, например: Hello");
}

QString AsciiFormat::toText(const QByteArray &data) const
{
    QString result;
    for (const char c : data) {
        const auto byte = static_cast<unsigned char>(c);
        if (byte == '\n')
            result += '\n';
        else if (byte == '\r')
            ;                   // строки с платы приходят с "\r\n", хватает '\n'
        else if (byte >= 0x20 && byte < 0x7F)
            result += QChar(byte);
        else
            result += '.';      // непечатаемый символ
    }
    return result;
}

bool AsciiFormat::fromText(const QString &text, QByteArray *data, QString *error) const
{
    for (const QChar c : text) {
        if (c.unicode() >= 0x80) {
            *error = QObject::tr("Символ «%1» не входит в ASCII").arg(c);
            return false;
        }
    }
    *data = text.toLatin1();
    return true;
}
