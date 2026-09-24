#pragma once

#include "dataformat.h"

// Байты числами в заданной системе счисления, через пробел.
// HEX и BIN - это NumericFormat с разными параметрами; так же добавляется, например, DEC.
class NumericFormat : public DataFormat
{
public:
    // base - основание (2, 10, 16), digits - число цифр на байт при выводе,
    // prefix - необязательный префикс при вводе ("0x", "0b").
    NumericFormat(const QString &name, int base, int digits, const QString &prefix,
                  const QString &example);

    QString name() const override;
    QString inputHint() const override;
    QString toText(const QByteArray &data) const override;
    bool fromText(const QString &text, QByteArray *data, QString *error) const override;

private:
    QString m_name;
    int m_base;
    int m_digits;
    QString m_prefix;
    QString m_example;
};
