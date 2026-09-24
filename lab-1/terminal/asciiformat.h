#pragma once

#include "dataformat.h"

// Текст ASCII: печатаемые символы как есть, остальные - точкой.
class AsciiFormat : public DataFormat
{
public:
    QString name() const override;
    QString inputHint() const override;
    QString toText(const QByteArray &data) const override;
    bool fromText(const QString &text, QByteArray *data, QString *error) const override;
};
