#include "formatregistry.h"

#include "asciiformat.h"
#include "numericformat.h"

FormatRegistry FormatRegistry::createDefault()
{
    FormatRegistry registry;
    registry.add(std::make_unique<AsciiFormat>());
    registry.add(std::make_unique<NumericFormat>("BIN", 2, 8, "0b", "00110001 11111111"));
    registry.add(std::make_unique<NumericFormat>("HEX", 16, 2, "0x", "31 FF 0A"));
    return registry;
}

void FormatRegistry::add(std::unique_ptr<DataFormat> format)
{
    m_formats.push_back(std::move(format));
}

int FormatRegistry::count() const
{
    return static_cast<int>(m_formats.size());
}

const DataFormat *FormatRegistry::at(int index) const
{
    return m_formats.at(index).get();
}
