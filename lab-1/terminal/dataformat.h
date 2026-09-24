#pragma once

#include <QByteArray>
#include <QString>

// Формат данных: как показать байты текстом и как разобрать текст, введённый пользователем.
// Один и тот же объект используется и для окон истории, и для строки ввода.
// Чтобы добавить формат, нужно унаследоваться от DataFormat (или создать ещё один
// NumericFormat) и зарегистрировать его в FormatRegistry::createDefault().
class DataFormat
{
public:
    virtual ~DataFormat() = default;

    // Название в выпадающих списках.
    virtual QString name() const = 0;

    // Подсказка в пустой строке ввода.
    virtual QString inputHint() const = 0;

    virtual QString toText(const QByteArray &data) const = 0;

    // Разбирает введённую строку. При ошибке возвращает false и пишет причину в error.
    virtual bool fromText(const QString &text, QByteArray *data, QString *error) const = 0;
};
