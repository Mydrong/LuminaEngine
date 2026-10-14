#pragma once
#include <string>
#include <string_view>
#include <vector>

struct FMetadataPair
{
    std::string Key;
    std::string Value;
};

// The value stored under Key, or null when there is none.
inline const std::string* FindMetadata(const std::vector<FMetadataPair>& Metadata, std::string_view Key)
{
    for (const FMetadataPair& Pair : Metadata)
    {
        if (Pair.Key == Key)
        {
            return &Pair.Value;
        }
    }
    return nullptr;
}

class FMetadataParser
{
public:

    explicit FMetadataParser(std::string_view Raw)
    {
        Parse(Raw);
    }

    void Parse(std::string_view Raw);

    auto begin() { return Metadata.begin(); }
    auto end() { return Metadata.end(); }

    auto begin() const { return Metadata.begin(); }
    auto end() const { return Metadata.end(); }

    std::vector<FMetadataPair> Metadata;

};
