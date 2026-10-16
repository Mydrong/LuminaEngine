#include "RuntimePCH.h"
#include "ImportPaths.h"

#include "Containers/HashTable.h"
#include "Containers/StringFormat.h"
#include "Core/Object/ObjectCore.h"
#include "Core/Object/Package/Package.h"
#include "FileSystem/FileSystem.h"
#include "Core/Threading/Thread.h"
#include "Core/Threading/Sync.h"

namespace Lumina::Import::PathReservations
{
    namespace
    {
        // Import runs on workers, and the content browser reserves from the main thread.
        FMutex               GMutex;
        THashSet<FFixedString> GReserved;

        constexpr uint32 kMaxSuffix = 10000;

        // Neither the extension nor case tells two packages apart, so neither may tell two reservations apart.
        FFixedString KeyOf(FStringView Path)
        {
            FFixedString Key(Path.data(), Path.size());
            if (VFS::HasExtension(Key, ".lasset"))
            {
                Key.resize(Key.size() - FStringView(".lasset").size());
            }
            for (char& Character : Key)
            {
                Character = (char)std::tolower((unsigned char)Character);
            }
            return Key;
        }

        bool IsOwnedUnlocked(const FFixedString& Path)
        {
            // What CreatePackage actually refuses on.
            if (FindObject<CPackage>(Path) != nullptr)
            {
                return true;
            }

            // An asset already on disk that nothing has loaded this session.
            FFixedString OnDisk = Path;
            CPackage::AddPackageExt(OnDisk);
            if (VFS::Exists(OnDisk))
            {
                return true;
            }

            return GReserved.find(KeyOf(Path)) != GReserved.end();
        }

        FFixedString FindFreeUnlocked(const FFixedString& Base)
        {
            if (!IsOwnedUnlocked(Base))
            {
                return Base;
            }

            for (uint32 N = 1; N < kMaxSuffix; ++N)
            {
                FFixedString Candidate = Base;
                Candidate.append("_").append(Format("{}", N).c_str());
                if (!IsOwnedUnlocked(Candidate))
                {
                    return Candidate;
                }
            }
            return {};
        }
    }

    bool IsFree(FStringView Path)
    {
        if (Path.empty())
        {
            return false;
        }

        FScopeLock Lock(GMutex);
        return !IsOwnedUnlocked(FFixedString(Path.data(), Path.size()));
    }

    FFixedString FindFree(FStringView Path)
    {
        if (Path.empty())
        {
            return {};
        }

        FScopeLock Lock(GMutex);
        return FindFreeUnlocked(FFixedString(Path.data(), Path.size()));
    }

    FFixedString Reserve(FStringView Path)
    {
        if (Path.empty())
        {
            return {};
        }

        FScopeLock Lock(GMutex);
        FFixedString Free = FindFreeUnlocked(FFixedString(Path.data(), Path.size()));
        if (!Free.empty())
        {
            GReserved.insert(KeyOf(Free));
        }
        return Free;
    }

    void Release(FStringView Path)
    {
        if (Path.empty())
        {
            return;
        }

        FScopeLock Lock(GMutex);
        GReserved.erase(KeyOf(Path));
    }

    void ReleaseAll()
    {
        FScopeLock Lock(GMutex);
        GReserved.clear();
    }

    FScopedReservation::FScopedReservation(FStringView Path)
        : Reserved(Reserve(Path))
    {
    }

    FScopedReservation::~FScopedReservation()
    {
        if (!bCommitted)
        {
            Release(Reserved);
        }
    }
}
