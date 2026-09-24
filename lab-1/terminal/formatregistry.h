#pragma once

#include "dataformat.h"

#include <memory>
#include <vector>

// Список доступных форматов. Из него заполняются выпадающие списки окна,
// поэтому новый формат достаточно добавить в createDefault().
class FormatRegistry
{
public:
    static FormatRegistry createDefault();

    void add(std::unique_ptr<DataFormat> format);

    int count() const;
    const DataFormat *at(int index) const;

private:
    std::vector<std::unique_ptr<DataFormat>> m_formats;
};
