#include "yaml_config.h"
#include "yaml_config_impl.h"

namespace NYamlConfig {

inline const TMap<TString, EYamlConfigLabelTypeClass> ClassMapping{
    std::pair{TString("enum"), EYamlConfigLabelTypeClass::Closed},
    std::pair{TString("string"), EYamlConfigLabelTypeClass::Open},
};

inline const TStringBuf inheritMapTag{"!inherit"};
inline const TStringBuf inheritSeqTag{"!inherit:"};
inline const TStringBuf inheritMapInSeqTag{"!inherit"};
inline const TStringBuf removeTag{"!remove"};

TString GetKey(const NFyaml::TNodeRef& node, TString key) {
    auto map = node.Map();
    auto k = map.at(key).Scalar();
    return k;
}

TString CalcHash(const NFyaml::TDocument& resolved) {
    TStringStream ss;
    ss << resolved;
    TString s = ss.Str();
    SHA256_CTX sha;
    SHA256_Init(&sha);
    SHA256_Update(&sha, s.data(), s.size());
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha);
    return TString(reinterpret_cast<char*>(hash), sizeof(hash));
}

bool Fit(const TSelector& selector, const TSet<TNamedLabel>& labels) {
    bool result = true;
    size_t matched = 0;
    for (auto& label : labels) {
        if (auto it = selector.NotIn.find(label.Name); it != selector.NotIn.end()
            && it->second.Values.contains(label.Value)) {

            return false;
        }

        if (auto it = selector.In.find(label.Name); it != selector.In.end()) {
            if (!it->second.Values.contains(label.Value)) {
                result = false;
            } else {
                ++matched;
            }
        }
    }
    return (matched == selector.In.size()) && result;
}

TSelector ParseSelector(const NFyaml::TNodeRef& selectors) {
    TSelector result;
    if (selectors) {
        auto selectorsMap = selectors.Map();

        for (auto it = selectorsMap.begin(); it != selectorsMap.end(); ++it) {
            switch (it->Value().Type()) {
                case NFyaml::ENodeType::Scalar:
                {
                    auto [label, _] = result.In.try_emplace(
                        it->Key().Scalar(),
                        TLabelValueSet{});
                    label->second.Values.insert(it->Value().Scalar());
                }
                break;
                case NFyaml::ENodeType::Mapping:
                {
                    auto in = it->Value().Map()["in"];
                    auto notIn = it->Value().Map()["not_in"];
                    if (in && notIn) {
                        ythrow TYamlConfigEx() << "Using both in and not_in for same label: "
                            << it->Value().Path();
                    }

                    if (in) {
                        auto inSeq = in.Sequence();
                        auto [label, _] = result.In.try_emplace(
                            it->Key().Scalar(),
                            TLabelValueSet{});
                        for(auto it3 = inSeq.begin(); it3 != inSeq.end(); ++it3) {
                            label->second.Values.insert(it3->Scalar());
                        }
                    }

                    if (notIn) {
                        auto notInSeq = notIn.Sequence();
                        auto [label, _] = result.NotIn.try_emplace(
                            it->Key().Scalar(),
                            TLabelValueSet{});
                        for(auto it3 = notInSeq.begin(); it3 != notInSeq.end(); ++it3) {
                            label->second.Values.insert(it3->Scalar());
                        }
                    }
                }
                break;
                default:
                {
                    ythrow TYamlConfigEx() << "Selector should be scalar, \"in\" or \"not_in\": "
                        << it->Value().Path();
                }
                break;
            }
        }
    } else {
        ythrow TYamlConfigEx() << "Selector shouldn't be empty";
    }
    return result;
}

TYamlConfigModel ParseConfig(NFyaml::TDocument& doc) {
    TYamlConfigModel res{.Doc = doc};
    auto root = doc.Root().Map();
    res.Config = root.at("config");

    auto allowedLabels = root.at("allowed_labels").Map();

    for (auto it = allowedLabels.begin(); it != allowedLabels.end(); ++it) {
        auto type = it->Value().Map().at("type");
        if (!type || type.Type() != NFyaml::ENodeType::Scalar) {
            ythrow TYamlConfigEx() << "Label type should be Scalar";
        }

        EYamlConfigLabelTypeClass classType;

        if (auto classIt = ClassMapping.find(type.Scalar()); classIt != ClassMapping.end()) {
            classType = classIt->second;
        } else {
            ythrow TYamlConfigEx() << "Unsupported label type: " << type.Scalar();
        }

        auto label = res.AllowedLabels.try_emplace(
            it->Key().Scalar(),
            TLabelType{classType, TSet<TString>{""}});

        if (auto labelDesc = it->Value().Map()["values"]; labelDesc) {
            auto values = labelDesc.Map();
            for(auto it2 = values.begin(); it2 != values.end(); ++it2) {
                label.first->second.Values.insert(it2->Key().Scalar());
            }
        }
    }

    auto selectorConfig = root.at("selector_config").Sequence();

    for (auto it = selectorConfig.begin(); it != selectorConfig.end(); ++it) {
        TYamlConfigModel::TSelectorModel selector;

        auto selectorRoot = it->Map();
        selector.Description = selectorRoot.at("description").Scalar();
        selector.Config = selectorRoot.at("config");
        selector.Selector = ParseSelector(selectorRoot.at("selector"));

        res.Selectors.push_back(selector);
    }

    return res;
}

TMap<TString, TLabelType> CollectLabels(NFyaml::TDocument& doc) {

    auto config = ParseConfig(doc);

    TMap<TString, TLabelType> result = config.AllowedLabels;

    for (auto& selector : config.Selectors) {
        for (auto& [name, valueSet] : selector.Selector.In) {
            result[name].Values.insert(valueSet.Values.begin(), valueSet.Values.end());
        }

        for (auto& [name, valueSet] : selector.Selector.NotIn) {
            result[name].Values.insert(valueSet.Values.begin(), valueSet.Values.end());
        }
    }

    return result;
}

bool IsMapInherit(const NFyaml::TNodeRef& node) {
    if (auto tag = node.Tag(); tag) {
        switch (node.Type()) {
            case NFyaml::ENodeType::Mapping:
                return *tag == inheritMapTag;
            case NFyaml::ENodeType::Sequence:
                return tag->StartsWith(inheritSeqTag);
            case NFyaml::ENodeType::Scalar:
                return false;
        }
    }
    return false;
}

bool IsSeqInherit(const NFyaml::TNodeRef& node) {
    if (auto tag = node.Tag(); tag) {
        switch (node.Type()) {
            case NFyaml::ENodeType::Mapping:
                return *tag == inheritMapInSeqTag;
            case NFyaml::ENodeType::Sequence:
                return false;
            case NFyaml::ENodeType::Scalar:
                return false;
        }
    }
    return false;
}

bool IsRemove(const NFyaml::TNodeRef& node) {
    if (auto tag = node.Tag(); tag) {
        return *tag == removeTag;
    }
    return false;
}

void Inherit(NFyaml::TMapping& toMap, const NFyaml::TMapping& fromMap) {
    for (auto it = fromMap.begin(); it != fromMap.end(); ++it) {
        if (auto toEntry = toMap.pair_at_opt(it->Key().Scalar()); toEntry) {
            auto fromNode = it->Value();
            auto toNode = toEntry.Value();

            if (IsMapInherit(fromNode)) {
                Apply(toNode, fromNode);
            } else {
                toMap.Remove(toEntry.Key());
                toMap.Append(it->Key().Copy().Ref(), it->Value().Copy().Ref());
            }
        } else {
            toMap.Append(it->Key().Copy().Ref(), it->Value().Copy().Ref());
        }
    }
}

void Inherit(NFyaml::TSequence& toSeq, const NFyaml::TSequence& fromSeq, const TString& key) {
    TMap<TString, NFyaml::TNodeRef> nodes;

    for (auto it = toSeq.begin(); it != toSeq.end(); ++it) {
        nodes[GetKey(*it, key)] = *it;
    }

    for (auto it = fromSeq.begin(); it != fromSeq.end(); ++it) {
        auto fromKey = GetKey(*it, key);

        if (nodes.contains(fromKey)) {
            if (IsSeqInherit(*it)) {
                Apply(nodes[fromKey], *it);
            } else if (IsRemove(*it)) {
                toSeq.Remove(nodes[fromKey]);
                nodes.erase(fromKey);
            } else {
                auto newNode = it->Copy();
                toSeq.InsertAfter(nodes[fromKey], newNode.Ref());
                toSeq.Remove(nodes[fromKey]);
                nodes[fromKey] = newNode.Ref();
            }
        } else {
            auto newNode = it->Copy();
            toSeq.Append(newNode.Ref());
            nodes[fromKey] = newNode.Ref();
        }
    }
}

void Apply(NFyaml::TNodeRef& to, const NFyaml::TNodeRef& from) {
    Y_ENSURE_EX(to, TYamlConfigEx() << "Overriding empty value: "
                << to.Path() << " <- " << from.Path());
    Y_ENSURE_EX(to.Type() == from.Type(), TYamlConfigEx() << "Overriding value with different types: "
                << to.Path() << " <- " << from.Path());

    switch (from.Type()) {
        case NFyaml::ENodeType::Mapping:
        {
            auto toMap = to.Map();
            auto fromMap = from.Map();
            Inherit(toMap, fromMap);
        }
        break;
        case NFyaml::ENodeType::Sequence:
        {
            auto tag = from.Tag();
            auto key = tag->substr(inheritSeqTag.length());
            auto toSeq = to.Sequence();
            auto fromSeq = from.Sequence();
            Inherit(toSeq, fromSeq, key);
        }
        break;
        case NFyaml::ENodeType::Scalar:
        {
            ythrow TYamlConfigEx() << "Override with scalar: "
                << to.Path() << " <- " << from.Path();
        }
        break;
    }
}

void RemoveTags(NFyaml::TDocument& doc) {
    for (auto it = doc.begin(); it != doc.end(); ++it) {
        it->RemoveTag();
    }
}

TDocumentConfig Resolve(
    const NFyaml::TDocument& doc,
    const TSet<TNamedLabel>& labels)
{
    TDocumentConfig res{doc.Clone(), NFyaml::TNodeRef{}};
    res.first.Resolve();

    auto rootMap = res.first.Root().Map();
    auto config = rootMap.at("config");
    auto selectorConfig = rootMap.at("selector_config").Sequence();

    for (auto it = selectorConfig.begin(); it != selectorConfig.end(); ++it) {
        auto selectorMap = it->Map();
        auto desc = selectorMap.at("description").Scalar();
        auto selectorNode = selectorMap.at("selector");
        auto selector = ParseSelector(selectorNode);
        if (Fit(selector, labels)) {
            Apply(config, selectorMap.at("config"));
        }
    }

    RemoveTags(res.first);

    res.second = config;

    return res;
}

void Combine(
    TVector<TVector<TLabel>>& labelCombinations,
    TVector<TLabel>& combination,
    const TVector<std::pair<TString, TSet<TLabel>>>& labels,
    size_t offset)
{
    if (offset == labels.size()) {
        labelCombinations.push_back(combination);
        return;
    }

    for (auto& label : labels[offset].second) {
        combination[offset] = label;
        Combine(labelCombinations, combination, labels, offset + 1);
    }
}

bool Fit(
    const TSelector& selector,
    const TVector<TLabel>& labels,
    const TVector<std::pair<TString, TSet<TLabel>>>& names)
{
    for (size_t i = 0; i < labels.size(); ++ i) {
        auto& label = labels[i];
        auto& name = names[i].first;
        switch(label.Type) {
            case TLabel::EType::Negative:
                if (selector.In.contains(name)) {
                    return false;
                }
                break;
            case TLabel::EType::Empty: [[fallthrough]];
            case TLabel::EType::Common:
                if (auto it = selector.In.find(name); it != selector.In.end()
                    && !it->second.Values.contains(label.Value)) {

                    return false;
                }
                if (auto it = selector.NotIn.find(name); it != selector.NotIn.end()
                    && it->second.Values.contains(label.Value)) {

                    return false;
                }
                break;
        }
    }

    return true;
}

TResolvedConfig ResolveAll(NFyaml::TDocument& doc)
{
    TVector<TString> labelNames;
    TVector<std::pair<TString, TSet<TLabel>>> labels;

    auto config = ParseConfig(doc);
    auto namedLabels = CollectLabels(doc);

    for (auto& [name, values]: namedLabels) {
        TSet<TLabel> set;
        if (values.Class == EYamlConfigLabelTypeClass::Open) {
            set.insert(TLabel{TLabel::EType::Negative, {}});
        }
        for (auto& value: values.Values) {
            if (value != "") {
                set.insert(TLabel{TLabel::EType::Common, value});
            } else {
                set.insert(TLabel{TLabel::EType::Empty, {}});
            }
        }
        labels.push_back({name, set});
        labelNames.push_back(name);
    }

    TVector<TVector<TLabel>> labelCombinations;

    TVector<TLabel> combination;
    combination.resize(labels.size());

    Combine(labelCombinations, combination, labels, 0);

    auto cmp = [](const TVector<int>& lhs, const TVector<int>& rhs) {
        auto lhsIt = lhs.begin();
        auto rhsIt = rhs.begin();

        while (lhsIt != lhs.end() && rhsIt != rhs.end() && (*lhsIt == *rhsIt)) {
            lhsIt++;
            rhsIt++;
        }

        if (lhsIt == lhs.end()) {
            return false;
        } else if (rhsIt == rhs.end()) {
            return true;
        }

        return *lhsIt < *rhsIt;
    };

    using TTriePath = TVector<int>;

    struct TTrieNode {
        TSimpleSharedPtr<TDocumentConfig> ResolvedConfig;
        TVector<TVector<TLabel>> LabelCombinations;
    };

    std::map<TTriePath, TSimpleSharedPtr<TDocumentConfig>, decltype(cmp)> selectorsTrie(cmp);
    std::map<TTriePath, TTrieNode, decltype(cmp)> appliedSelectors(cmp);

    auto rootConfig = TTrieNode {
        MakeSimpleShared<TDocumentConfig>(std::move(doc), config.Config),
        {},
    };

    selectorsTrie[{0}] = rootConfig.ResolvedConfig;

    for (size_t j = 0; j < labelCombinations.size(); ++j) {
        TSimpleSharedPtr<TDocumentConfig> cur = rootConfig.ResolvedConfig;
        TTriePath triePath({0});

        for (size_t i = 0; i < config.Selectors.size(); ++i) {
            if (Fit(config.Selectors[i].Selector, labelCombinations[j], labels)) {
                triePath.push_back(i + 1);
                if (auto it = selectorsTrie.find(triePath); it != selectorsTrie.end()) {
                    cur = it->second;
                } else {
                    auto clone = cur->first.Clone();
                    auto cloneConfig = ParseConfig(clone);

                    Apply(cloneConfig.Config, cloneConfig.Selectors[i].Config);

                    cur = MakeSimpleShared<std::pair<NFyaml::TDocument, NFyaml::TNodeRef>>(
                        std::move(clone),
                        cloneConfig.Config),
                    selectorsTrie[triePath] = cur;
                }
            }
        }

        if (auto it = appliedSelectors.find(triePath); it != appliedSelectors.end()) {
            it->second.LabelCombinations.push_back(labelCombinations[j]);
        } else {
            appliedSelectors.try_emplace(triePath, TTrieNode{
                    cur,
                    {labelCombinations[j]}
                });
        }
    }

    selectorsTrie.clear();

    TMap<TSet<TVector<TLabel>>, TDocumentConfig> configs;

    for (auto& [_, value]: appliedSelectors) {
        configs.try_emplace(
            TSet<TVector<TLabel>>(
                value.LabelCombinations.begin(),
                value.LabelCombinations.end()),
            std::make_pair(std::move(value.ResolvedConfig->first), value.ResolvedConfig->second));
    }

    return {labelNames, std::move(configs)};
}

} // namespace NYamlConfig

template <>
void Out<NYamlConfig::TLabel>(IOutputStream& out, const NYamlConfig::TLabel& value) {
    out << (int)value.Type << ":" << value.Value;
}