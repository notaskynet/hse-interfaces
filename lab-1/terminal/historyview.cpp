#include "historyview.h"

#include "dataformat.h"

#include <QFontDatabase>

HistoryView::HistoryView(const DataHistory *history, Direction direction, Layout layout,
                         QWidget *parent)
    : QPlainTextEdit(parent)
    , m_history(history)
    , m_direction(direction)
    , m_layout(layout)
{
    setReadOnly(true);
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    connect(history, &DataHistory::appended, this, &HistoryView::appendChunk);
    connect(history, &DataHistory::cleared, this, &HistoryView::rebuild);
}

void HistoryView::setFormat(const DataFormat *format)
{
    m_format = format;
    rebuild();
}

void HistoryView::appendChunk(const DataChunk &chunk)
{
    if (!m_format || chunk.direction != m_direction)
        return;

    moveCursor(QTextCursor::End);
    insertPlainText(render(chunk, m_empty));
    moveCursor(QTextCursor::End);
    m_empty = false;
}

void HistoryView::rebuild()
{
    QString text;
    m_empty = true;
    if (m_format) {
        for (const DataChunk &chunk : m_history->chunks()) {
            if (chunk.direction != m_direction)
                continue;
            text += render(chunk, m_empty);
            m_empty = false;
        }
    }
    setPlainText(text);
    moveCursor(QTextCursor::End);
}

QString HistoryView::render(const DataChunk &chunk, bool first) const
{
    const QString text = m_format->toText(chunk.data);
    return (m_layout == Layout::Lines && !first) ? '\n' + text : text;
}
