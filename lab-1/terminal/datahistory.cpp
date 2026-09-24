#include "datahistory.h"

void DataHistory::append(Direction direction, const QByteArray &data)
{
    if (data.isEmpty())
        return;
    m_chunks.append({direction, data});
    emit appended(m_chunks.last());
}

void DataHistory::clear()
{
    m_chunks.clear();
    emit cleared();
}

const QList<DataChunk> &DataHistory::chunks() const
{
    return m_chunks;
}
