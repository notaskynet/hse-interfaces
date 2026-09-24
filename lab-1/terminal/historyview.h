#pragma once

#include "datahistory.h"

#include <QPlainTextEdit>

class DataFormat;

// Окно, показывающее данные одного направления из DataHistory в выбранном формате.
class HistoryView : public QPlainTextEdit
{
    Q_OBJECT

public:
    enum class Layout {
        Stream,     // порции подряд, как пришли (приём)
        Lines       // каждая порция с новой строки (передача)
    };

    HistoryView(const DataHistory *history, Direction direction, Layout layout,
                QWidget *parent = nullptr);

    void setFormat(const DataFormat *format);

private:
    void appendChunk(const DataChunk &chunk);
    void rebuild();
    QString render(const DataChunk &chunk, bool first) const;

    const DataHistory *m_history;
    Direction m_direction;
    Layout m_layout;
    const DataFormat *m_format = nullptr;
    bool m_empty = true;
};
