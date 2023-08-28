#include "integer_converter.h"
#include "helper.h"

#include <yt/yt/client/table_client/schema.h>
#include <yt/yt/client/table_client/versioned_row.h>

#include <library/cpp/yt/coding/zig_zag.h>

namespace NYT::NConverters {

////////////////////////////////////////////////////////////////////////////////

namespace {

ui64 EncodeValue(i64 value)
{
    return ZigZagEncode64(value);
}

ui64 EncodeValue(ui64 value)
{
    return value;
}

template <class TValue>
typename std::enable_if<std::is_signed<TValue>::value, TValue>::type
GetValue(const NTableClient::TUnversionedValue& value)
{
    return value.Data.Int64;
}

template <class TValue>
typename std::enable_if<std::is_unsigned<TValue>::value, TValue>::type
GetValue(const NTableClient::TUnversionedValue& value)
{
    return value.Data.Uint64;
}

////////////////////////////////////////////////////////////////////////////////

void FillColumnarIntegerValues(
    NTableClient::IUnversionedColumnarRowBatch::TColumn* column,
    i64 startIndex,
    i64 valueCount,
    NTableClient::EValueType valueType,
    ui64 baseValue,
    TRef data)
{
    column->StartIndex = startIndex;
    column->ValueCount = valueCount;

    auto& values = column->Values.emplace();
    values.BaseValue = baseValue;
    values.BitWidth = 64;
    values.ZigZagEncoded = (valueType == NTableClient::EValueType::Int64);
    values.Data = data;
}

////////////////////////////////////////////////////////////////////////////////

// TValue - i64 or ui64.
template <class TValue>
class TIntegerColumnConverter
    : public IColumnConverter
{
public:
    TIntegerColumnConverter(int columnIndex,
                            NTableClient::EValueType ValueType,
                            NTableClient::TColumnSchema columnSchema)

        : ColumnIndex_(columnIndex),
          ValueType_(ValueType),
          ColumnSchema_(columnSchema)
    {}

    TConvertedColumn Convert(TRange<NTableClient::TUnversionedRow> rows) override
    {
        Reset();
        AddValues(rows);
        for (i64 index = 0; index < std::ssize(Values_); ++index) {
            if (!NullBitmap_[index]) {
                Values_[index] -= MinValue_;
            }
        }

        auto nullBitmapRef = NullBitmap_.Flush<TConverterTag>();
        auto valuesRef = TSharedRef::MakeCopy<TConverterTag>(TRef(Values_.data(), sizeof(ui64) * Values_.size()));
        auto column = std::make_shared<TBatchColumn>();

        FillColumnarIntegerValues(
            column.get(),
            0,
            RowCount_,
            ValueType_,
            MinValue_,
            valuesRef);

        FillColumnarNullBitmap(
            column.get(),
            0,
            RowCount_,
            nullBitmapRef);

        column->Type = ColumnSchema_.LogicalType();
        column->Id = ColumnIndex_;

        TOwningColumn owner = {
            std::move(column),
            std::move(nullBitmapRef),
            std::move(valuesRef),
            /*stringBuffer*/ std::nullopt
        };

        TConvertedColumn res = {{owner}, owner.Column.get()};
        return res;
    }


private:
    const int ColumnIndex_;
    NTableClient::EValueType ValueType_;
    i64 RowCount_ = 0;
    NTableClient::TColumnSchema ColumnSchema_;

    TBitmapOutput NullBitmap_;
    std::vector<ui64> Values_;

    ui64 MaxValue_;
    ui64 MinValue_;

    // TODO: Dictionary column
    // THashMap<ui64, int> DistinctValues_;

    void Reset()
    {
        Values_.clear();
        RowCount_ = 0;
        MaxValue_ = 0;
        MinValue_ = std::numeric_limits<ui64>::max();
        NullBitmap_ = TBitmapOutput();
    }

    void AddValues(TRange<NTableClient::TUnversionedRow> rows)
    {
        for (auto row : rows) {
            const auto& value = row[ColumnIndex_];
            bool isNull = value.Type == NTableClient::EValueType::Null;
            ui64 data = 0;
            if (!isNull) {
                data = EncodeValue(GetValue<TValue>(value));
            }
            Values_.push_back(data);
            NullBitmap_.Append(isNull);
            ++RowCount_;
        }
    }
};

} // namespace

////////////////////////////////////////////////////////////////////////////////

IColumnConverterPtr CreateInt64ColumnConverter(int columnIndex, const NTableClient::TColumnSchema& columnSchema)
{
    return std::make_unique<TIntegerColumnConverter<i64>>(columnIndex, NTableClient::EValueType::Int64, columnSchema);
}


IColumnConverterPtr CreateUint64ColumnConverter(int columnIndex, const NTableClient::TColumnSchema& columnSchema)
{
    return std::make_unique<TIntegerColumnConverter<ui64>>(columnIndex, NTableClient::EValueType::Uint64, columnSchema);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NConverters
