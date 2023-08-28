#include "boolean_converter.h"
#include "helper.h"

#include <yt/yt/client/table_client/schema.h>
#include <yt/yt/client/table_client/versioned_row.h>

namespace NYT::NConverters {

////////////////////////////////////////////////////////////////////////////////

namespace {

void FillColumnarBooleanValues(
    TBatchColumn* column,
    i64 startIndex,
    i64 valueCount,
    TRef bitmap)
{
    column->StartIndex = startIndex;
    column->ValueCount = valueCount;

    auto& values = column->Values.emplace();
    values.BitWidth = 1;
    values.Data = bitmap;
}

////////////////////////////////////////////////////////////////////////////////

class TBooleanColumnConverter
    : public IColumnConverter
{
public:
    TBooleanColumnConverter(int columnIndex, const NTableClient::TColumnSchema& columnSchema)
        : ColumnIndex_(columnIndex)
        , ColumnSchema_(columnSchema)
    { }

    TConvertedColumn Convert(TRange<NTableClient::TUnversionedRow> rows) override
    {
        Reset();
        AddValues(rows);

        auto column = std::make_shared<TBatchColumn>();
        auto nullBitmapRef = NullBitmap_.Flush<TConverterTag>();
        auto valuesRef = Values_.Flush<TConverterTag>();

        FillColumnarBooleanValues(column.get(), 0, rows.size(), valuesRef);
        FillColumnarNullBitmap(column.get(), 0, rows.size(), nullBitmapRef);

        column->Type = ColumnSchema_.LogicalType();
        column->Id = ColumnIndex_;

        TOwningColumn owner = {
            std::move(column),
            std::move(nullBitmapRef),
            std::move(valuesRef),
            /*stringBuffer*/ std::nullopt
        };

        return {{owner}, owner.Column.get()};
    }


private:
    const int ColumnIndex_;
    NTableClient::TColumnSchema ColumnSchema_;

    TBitmapOutput Values_;
    TBitmapOutput NullBitmap_;

    void Reset()
    {
        Values_ = TBitmapOutput();
        NullBitmap_ = TBitmapOutput();
    }

    void AddValues(TRange<NTableClient::TUnversionedRow> rows)
    {
        for (auto row : rows) {
            const auto& value = row[ColumnIndex_];
            bool isNull = value.Type == NTableClient::EValueType::Null;
            bool data = isNull ? false : value.Data.Boolean;
            NullBitmap_.Append(isNull);
            Values_.Append(data);
        }
    }
};

} // namespace

////////////////////////////////////////////////////////////////////////////////

IColumnConverterPtr CreateBooleanColumnConverter(int columnIndex, const NTableClient::TColumnSchema& columnSchema)
{
    return std::make_unique<TBooleanColumnConverter>(columnIndex, columnSchema);
}

////////////////////////////////////////////////////////////////////////////////


} // namespace NYT::NConverters
