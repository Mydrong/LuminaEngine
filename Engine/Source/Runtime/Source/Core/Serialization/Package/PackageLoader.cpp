#include "RuntimePCH.h"
#include "PackageLoader.h"

#include "Core/Object/ObjectArray.h"
#include "Core/Object/Package/Package.h"
#include "Core/Versioning/CoreVersion.h"
#include "Core/Serialization/PortableBytes.h"
#include "Memory/Memcpy.h"

namespace Lumina
{
    FArchive& FPackageLoader::operator<<(CObject*& Value)
    {
        FObjectPackageIndex Index;
        FArchive& Ar = *this;
        Ar << Index;
        
        Value = Package->IndexToObject(Index);
        
        return Ar;
    }

    bool FPackageLoader::DescribeCutBytes(FPortableBytes& Cut) const
    {
        if (Package == nullptr)
        {
            return false;
        }

        const bool bNameSlots = Names != nullptr && Cut.FileVersion >= (int32)ELuminaEngineVersion::PACKAGE_NAME_TABLE;
        const int32 NumImports = (int32)Package->ImportTable.size();
        const int32 NumExports = (int32)Package->ExportTable.size();

        THashSet<uint32> SeenNames;
        THashSet<int32> SeenObjects;
        for (size_t Offset = 0; Offset + sizeof(uint32) <= Cut.Bytes.size(); ++Offset)
        {
            uint32 Word = 0;
            Memory::Memcpy(&Word, Cut.Bytes.data() + Offset, sizeof(uint32));

            const uint32 Slot = Word & kPackageNameSlotMask;
            if (bNameSlots && Slot < Names->Num() && SeenNames.insert(Slot).second)
            {
                Cut.Names.push_back({ Slot, Names->Resolve(Slot, false, 0) });
            }

            const FObjectPackageIndex Index((int32)Word);
            const int64 Raw = Index.GetRaw();
            if (Raw == 0 || Raw < -(int64)NumImports || Raw > (int64)NumExports || !SeenObjects.insert(Index.GetRaw()).second)
            {
                continue;
            }
            const FGuid& Object = Index.IsImport()
                ? Package->ImportTable[Index.GetArrayIndex()].ObjectGUID
                : Package->ExportTable[Index.GetArrayIndex()].ObjectGUID;
            Cut.Objects.push_back({ Index.GetRaw(), Object });
        }
        return true;
    }

    FArchive& FPackageLoader::operator<<(FName& Value)
    {
        if (Names == nullptr || GetFileVersion() < (int32)ELuminaEngineVersion::PACKAGE_NAME_TABLE)
        {
            return FArchive::operator<<(Value);
        }

        SerializePackageName(*this, Value, nullptr, Names.get());
        return *this;
    }

    FArchive& FPackageLoader::operator<<(FObjectHandle& Value)
    {
        FObjectPackageIndex Index;
        FArchive& Ar = *this;
        Ar << Index;

        CObject* Object = Package->IndexToObject(Index);
        Value = GObjectArray.GetHandleByObject(Object);
        Package->LoadObject(Object);
        
        return Ar;    
    }
}
