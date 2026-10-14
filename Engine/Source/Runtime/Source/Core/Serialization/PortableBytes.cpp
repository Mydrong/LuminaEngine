#include "RuntimePCH.h"
#include "PortableBytes.h"

#include "Containers/Algorithm.h"
#include "Core/Object/Class.h"
#include "Core/Object/ObjectArray.h"
#include "Core/Object/ObjectCore.h"
#include "Core/Object/Package/Package.h"
#include "Core/Serialization/MemoryArchiver.h"
#include "Core/Serialization/Package/PackageNameTable.h"

namespace Lumina
{
    namespace
    {
        CObject* ResolveObject(const FGuid& Guid)
        {
            if (!Guid.IsValid())
            {
                return nullptr;
            }
            if (CObject* Found = FindObject<CObject>(Guid))
            {
                return Found;
            }
            return LoadObject<CObject>(Guid);
        }

        // The sparse slots laid out densely, so the package name reader reads them as it would a package's own table.
        TVector<FName> DenseNames(const FPortableBytes& Source)
        {
            TVector<FName> Slots(Source.Names.empty() ? 0 : Source.Names.back().Slot + 1);
            for (const FPortableBytes::FNameSlot& Entry : Source.Names)
            {
                Slots[Entry.Slot] = Entry.Name;
            }
            return Slots;
        }

        class FPortableReader final : public FPackageContainerReader
        {
        public:

            explicit FPortableReader(const FPortableBytes& InSource)
                : FPackageContainerReader(InSource.Bytes)
                , Source(InSource)
                , Names(DenseNames(InSource))
            {
                SetFileVersion(Source.FileVersion);
                SetNameTable(&Names);
            }

            using FPackageContainerReader::operator<<;

            FArchive& operator<<(CObject*& Value) override
            {
                FObjectPackageIndex Index;
                *this << Index;
                const auto It = Algo::LowerBound(Source.Objects, Index.GetRaw(),
                    [](const FPortableBytes::FObjectSlot& Entry, int32 Key) { return Entry.Index < Key; });
                Value = (It != Source.Objects.end() && It->Index == Index.GetRaw()) ? ResolveObject(It->Object) : nullptr;
                return *this;
            }

            FArchive& operator<<(FObjectHandle& Value) override
            {
                CObject* Object = nullptr;
                *this << Object;
                Value = GObjectArray.GetHandleByObject(Object);
                return *this;
            }

        private:

            const FPortableBytes& Source;
            FPackageNameTable     Names;
        };

        // Numbers every object as an import of these bytes alone, the way a package saver numbers its imports.
        class FPortableWriter final : public FPackageContainerWriter
        {
        public:

            using FPackageContainerWriter::FPackageContainerWriter;
            using FPackageContainerWriter::operator<<;

            FArchive& operator<<(CObject*& Value) override
            {
                FObjectPackageIndex Index;
                if (Value != nullptr)
                {
                    auto [It, bAdded] = ObjectIndices.try_emplace(Value, (int32)Objects.size());
                    if (bAdded)
                    {
                        Objects.push_back(Value->GetGUID());
                    }
                    Index = FObjectPackageIndex::FromImport(It->second);
                }
                *this << Index;
                return *this;
            }

            FArchive& operator<<(FObjectHandle& Value) override
            {
                CObject* Object = Value.Resolve();
                return *this << Object;
            }

            THashMap<CObject*, int32>    ObjectIndices;
            TVector<FGuid>               Objects;
        };
    }

    FArchive& operator<<(FArchive& Ar, FPortableBytes::FObjectSlot& Entry)
    {
        Ar << Entry.Index;
        CObject* Object = Ar.IsWriting() ? ResolveObject(Entry.Object) : nullptr;
        Ar << Object;
        if (Ar.IsReading())
        {
            Entry.Object = Object != nullptr ? Object->GetGUID() : FGuid::Invalid();
        }
        return Ar;
    }

    bool FPortableBytes::Cut(FArchive& Ar, int64 Size)
    {
        FileVersion = Ar.GetFileVersion();
        Names.clear();
        Objects.clear();
        Bytes.resize((size_t)Math::Max<int64>(Size, 0));
        if (!Bytes.empty())
        {
            Ar.Serialize(Bytes.data(), (int64)Bytes.size());
        }
        if (!Ar.DescribeCutBytes(*this))
        {
            return false;
        }
        Algo::Sort(Names, [](const FNameSlot& A, const FNameSlot& B) { return A.Slot < B.Slot; });
        Algo::Sort(Objects, [](const FObjectSlot& A, const FObjectSlot& B) { return A.Index < B.Index; });
        return true;
    }

    void FPortableBytes::Capture(const CStruct& Struct, const void* Value)
    {
        Bytes.clear();
        Names.clear();
        Objects.clear();

        FPortableWriter Writer(Bytes);
        Struct.SerializeProperties(Writer, const_cast<void*>(Value));
        FileVersion = Writer.GetFileVersion();

        const TVector<FName>& Slots = Writer.GetNameMap().GetSlots();
        for (uint32 Slot = 0; Slot < (uint32)Slots.size(); ++Slot)
        {
            Names.push_back({ Slot, Slots[Slot] });
        }
        // Imports count down from -1, so walking the list backward keeps the slots ascending.
        for (int32 Import = (int32)Writer.Objects.size() - 1; Import >= 0; --Import)
        {
            Objects.push_back({ FObjectPackageIndex::FromImport(Import).GetRaw(), Writer.Objects[Import] });
        }
    }

    TUniquePtr<FArchive> FPortableBytes::MakeReader() const
    {
        return MakeUnique<FPortableReader>(*this);
    }
}
