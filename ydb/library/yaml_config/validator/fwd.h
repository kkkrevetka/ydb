#pragma once

#include <util/generic/ptr.h>

namespace NYamlConfig::NValidator {

enum class ENodeType {
    Generic, Map, Array, Int64, String, Bool
};

class TGenericBuilder;
class TMapBuilder;
class TArrayBuilder;
class TInt64Builder;
class TStringBuilder;
class TBoolBuilder;

class TValidator;
class TGenericValidator;
class TMapValidator;
class TArrayValidator;
class TInt64Validator;
class TStringValidator;
class TBoolValidator;

class TNodeWrapper;
class TGenericNodeWrapper;
class TMapNodeWrapper;
class TArrayNodeWrapper;
class TInt64NodeWrapper;
class TStringNodeWrapper;
class TBoolNodeWrapper;

class TCheckContext;
class TGenericCheckContext;
class TMapCheckContext;
class TArrayCheckContext;
class TInt64CheckContext;
class TStringCheckContext;
class TBoolCheckContext;

class TValidationResult;

namespace NDetail {

class TBuilder;
TSimpleSharedPtr<TValidator> CreateValidatorPtr(const TSimpleSharedPtr<TBuilder>& builder);
template <typename TThis, typename TContext> class TCommonBuilderOps;
template <typename TThis, typename TContext> class TValidatorCommonOps;

} // namespace NDetail
} // namespace NYamlConfig::NValidator
