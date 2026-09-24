#include "numericformat.h"

#include <QObject>
#include <QRegularExpression>

NumericFormat::NumericFormat(const QString &name, int base, int digits, const QString &prefix,
                             const QString &example)
    : m_name(name)
    , m_base(base)
    , m_digits(digits)
    , m_prefix(prefix)
    , m_example(example)
{
}

QString NumericFormat::name() const
{
    return m_name;
}

QString NumericFormat::inputHint() const
{
    return QObject::tr("Байты через пробел, например: %1").arg(m_example);
}

QString NumericFormat::toText(const QByteArray &data) const
{
    QString result;
    for (const char c : data) {
        const auto byte = static_cast<unsigned char>(c);
        result += QString::number(byte, m_base).rightJustified(m_digits, '0').toUpper() + ' ';
    }
    return result;
}

bool NumericFormat::fromText(const QString &text, QByteArray *data, QString *error) const
{
    static const QRegularExpression separators("\\s+");
    const QStringList tokens = text.split(separators, Qt::SkipEmptyParts);

    QByteArray result;
    for (const QString &token : tokens) {
        QString digits = token;
        if (!m_prefix.isEmpty() && digits.startsWith(m_prefix, Qt::CaseInsensitive))
            digits.remove(0, m_prefix.size());

        bool ok = false;
        const uint value = digits.toUInt(&ok, m_base);
        if (!ok || digits.size() > m_digits || value > 0xFF) {
            *error = QObject::tr("«%1» - не байт в формате %2").arg(token, m_name);
            return false;
        }
        result.append(static_cast<char>(value));
    }

    *data = result;
    return true;
}
