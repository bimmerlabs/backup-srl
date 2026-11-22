// STN-9 https://docs.exodusemulator.com/Archives/SSDDV25/segahtml/index.html?page=info/hon/stn09.htm
// Calculation formula for “blocks” (use in game to show free blocks)
// ([Number of bytes used] + 32 / 64) Round up to the nearest whole number.
#pragma once
#include <smpc.hpp>

using namespace SRL::Math::Types;

namespace SRL::Backup {
    inline volatile uint32_t &BupLibAdress = *reinterpret_cast<volatile uint32_t *>(0x6000358); // I got confused here because jo engine hard-codes this address

    // Constants
    // ok, I figured out why these are +1 from Sega's doc
    // if you have a filename or comment that is the max length, the null terminator
    // gets automatically clipped by the bup driver
    // in order to check the max length, I'll make these match the documentation
    // and add +1 in the array
    constexpr size_t MAX_FILENAME_LENGTH = 11; // must be 11 characters or less
    constexpr size_t MAX_COMMENT_LENGTH  = 10; // must be 10 characters or less
    constexpr size_t DEVICE_COUNT        = 3;
    constexpr size_t LIB_SPACE_SIZE      = 16384;
    constexpr size_t WORK_SPACE_SIZE     = 8312; // see STN-52 "Backup library work area size"

    // for BupDeviceConfig "id"
    enum Unit : uint8_t
    {
        Internal   = 1,
        Cartridge  = 2,
        Serial     = 3
    };

    // Device Config
    struct BupDeviceConfig
    {
        uint16_t id;
        uint16_t partition;
    };

    // Device Types
    enum BupDevice : uint8_t
    {
        InternalMemoryBackup = 0,
        CartridgeMemoryBackup,
        ExternalDeviceBackup // floppy drive
    };

    // Language types
    enum Language : uint8_t
    {
        Japanese   = 0,
        English,
        Francais,
        Deutsch,
        Espanol,
        Italiano
    };

    // Machine states (from BIOS)
    enum BupStatus : uint8_t
    {
        Success = 0,
        Failed,
        Unformatted,    // only applicable to FDD
        WriteProtected, // only applicable to FDD
        NotEnoughMemory,
        NotFound,
        Found,
        NoMatch,
        Broken
    };

    struct BupStatusTable
    {
        uint32_t TotalSize;
        uint32_t TotalBlocks;
        uint32_t BlockSize;
        uint32_t FreeSize;
        uint32_t FreeBlocks;
        uint32_t WritableCount;
    };

    // this is different from BupWriteTable, because it includes *Data
    struct BupFile
    {
        unsigned char Name[MAX_FILENAME_LENGTH + 1] = {};
        unsigned char Comment[MAX_COMMENT_LENGTH + 1] = {};
        void *Data = nullptr;
        uint32_t Datasize = 0;
        uint8_t  Language;
    };

    // use SRL::Types::DateTime::BupDate instead
    // struct BupDate {
        // uint8_t year, month, day;
        // uint8_t hour, minute, weekday;
    // };

    // Device struct
    struct BupDeviceState {
        BupStatusTable StatTable{};
        int Status = -1;
        bool isMounted = false;
    };

    class Device
    {
    private:
        // Directory information table (needed for write)
        struct BupWriteTable
        {
            // make array one larger than the max length to include null terminator
            uint8_t	    filename[MAX_FILENAME_LENGTH + 1];	/* File name */
            uint8_t	    comment[MAX_COMMENT_LENGTH + 1];	/* Comments */
            uint8_t	    language;	    /* The language type of the comment */
            uint32_t    date;		    /* Time stamp */
            uint32_t	datasize;	    /* Data size (Byte) */
            uint16_t	blocksize;	    /* Data size (blocks) */
        };

        // Standard Sega BUP Function Offsets
        using BupInit    = void   (*)(volatile uint32_t* lib, uint32_t* work, BupDeviceConfig config[3]);
        using BupSelPart = int32_t (*)(uint32_t device, uint16_t partition);
        using BupFormat  = int32_t (*)(uint32_t device);
        using BupStat    = int32_t (*)(uint32_t device, uint32_t dummy, BupStatusTable *table);
        using BupWrite   = int32_t (*)(uint32_t device, BupWriteTable *table, volatile uint8_t *data, bool overwrite);
        using BupExists  = int32_t (*)(uint32_t device, uint8_t *filename, volatile uint8_t *dummy);
        using BupRead    = int32_t (*)(uint32_t device, uint8_t *filename, volatile uint8_t *data);
        using BupDelete  = int32_t (*)(uint32_t device, uint8_t *filename);
        using BupDir     = int32_t (*)(uint32_t device, uint8_t *filename, uint16_t tbsize, BupWriteTable *table);
        using BupVerify  = int32_t (*)(uint32_t device, uint8_t *filename, uint8_t *data);
        using BupGetDate = void   (*)(uint32_t date, BupDate *table);
        using BupSetDate = uint32_t (*)(BupDate *table);

        // Private Helper Functions / pointers
        template<typename T>
        static auto GetBupFunction(uint32_t offset)
        {
            auto ptr = reinterpret_cast<volatile uint8_t*>(WorkSpace) + offset;
            return reinterpret_cast<T>(
                *reinterpret_cast<volatile uint32_t*>(ptr)
            );
        }

        // Load BUP driver from BIOS to HWRAM and setup BupDeviceConfig
        static void DriverInit(uint32_t* lib, uint32_t* work, BupDeviceConfig* config) {
            reinterpret_cast<BupInit>(BupLibAdress)(lib, work, config);
        }

        // Select partition on BUP device
        // returns BupStatusTable
        static int32_t SelectPartition(BupDevice device, uint16_t partition) {
            auto func = GetBupFunction<BupSelPart>(4);
            return func(device, partition);
        }

        // Format BUP device
        // Returns BupStatus
        static int32_t FormatDevice(BupDevice device) {
            auto func = GetBupFunction<BupFormat>(8);
            return func(device);
        }

        // Return device status
        static int32_t Status(BupDevice device, BupStatusTable* outstat)
        {
            auto func = GetBupFunction<BupStat>(12);
            return func(device, 1, outstat);
        }

        // Write file to BUP device
        // returns BupStatusTable
        static int32_t Write(BupDevice device, BupFile *file, bool overwrite)
        {
            BupWriteTable WriteTable = {};

            strcpy((char*)WriteTable.filename, (const char*)file->Name);
            strcpy((char*)WriteTable.comment, (const char*)file->Comment);

            // Jo engine calls SGL to get the SMPC language
            // (which is in a different order from BUP)
            // I don't want to do that.  The user can just set the language.
            WriteTable.language = file->Language;
            // jo engine complicates this for no reason
            DateTime time = DateTime::Now();
            // use SRL::Types::DateTime::BupDate instead
            BupDate dateTable = time.ToBackupUnitDate();
            // BupDate dateTable
            // {
                // static_cast<uint8_t>(time.Year() - DateTime::BaseYear),
                // time.Month(),
                // time.Day(),
                // time.Hour(),
                // time.Minute(),
                // time.Week()
            // };
            WriteTable.date = SetDate(&dateTable);

            WriteTable.datasize = file->Datasize;

            // because to Sega, true is false and false is true.
            overwrite = !overwrite;

            auto func = GetBupFunction<BupWrite>(16);
            return func(
                device,
                &WriteTable,
                static_cast<volatile unsigned char*>(file->Data),
                overwrite
            );
        }
                // Read file from BUP device
        static int32_t ReadFile(BupDevice device, unsigned char *filename, uint8_t *data)
        {
            auto func = GetBupFunction<BupRead>(20);
            return func(device, (uint8_t*)filename, data);
        }

        // Delete file from BUP device
        static int32_t DeleteFile(BupDevice device, unsigned char *filename)
        {
            auto func = GetBupFunction<BupDelete>(24);
            return func(device, (uint8_t*)filename);
        }

        // Get directory entries from BUP device
        static int32_t Dir(BupDevice device, const unsigned char *filename, BupWriteTable *outTable, uint16_t maxEntries)
        {
            auto func = GetBupFunction<BupDir>(28);
            return func(device, const_cast<uint8_t*>(filename), maxEntries, outTable);
        }

        // Verify file data on BUP device (checksum)
        static int32_t VerifyFile(BupDevice device, uint8_t *filename, uint8_t *data)
        {
            auto func = GetBupFunction<BupVerify>(32);
            return func(device, filename, data);
        }

        // Expands the date and time data in the BupDate table
        // Get backup device last edit date
        // returns nothing
        static void GetDate(uint32_t date, BupDate *table)
        {
            auto func = GetBupFunction<BupGetDate>(36);
            func(date, table);
        }

        // Compresses the date and time data into the BupDate table
        // Prepares date for writetable
        // returns nothing
        static uint32_t SetDate(BupDate *table)
        {
            auto func = GetBupFunction<BupSetDate>(40);
            return func(table);
        }

        static bool Init()
        {
            if (Initialized) return true;

            LibrarySpace = static_cast<uint32_t*>(SRL::Memory::HighWorkRam::Malloc(LIB_SPACE_SIZE));
            if (!LibrarySpace) return false;

            WorkSpace = static_cast<uint32_t*>(SRL::Memory::LowWorkRam::Malloc(WORK_SPACE_SIZE));
            if (!WorkSpace) return false;

            SRL::SMPC::DisableReset();
            DriverInit(LibrarySpace, WorkSpace, BupConfig);
            SRL::SMPC::EnableReset();

            for (auto& device : BupState)
                device.isMounted = false;

            Initialized = true;
            return (Initialized);
        }

        static bool Mount(BupDevice device)
        {
            if (!Initialized) return false;

            BupState[device].Status = Status(device, &BupState[device].StatTable);

            switch (BupState[device].Status)
            {
                case BupStatus::Success:
                    BupState[device].isMounted = true;
                    break;

                case BupStatus::Unformatted:
                    // SRL::Debug::Print(2, 20, "FDD Disk Unformatted");
                    BupState[device].isMounted = false;
                    break;

                case BupStatus::WriteProtected:
                    // Media can be read from, but write operations will fail
                    // SRL::Debug::Print(2, 20, "FDD Disk Write Protected");
                    BupState[device].isMounted = true;
                    break;

                default:
                    BupState[device].isMounted = false;
                    break;
            }

            return BupState[device].isMounted;
        }

    public:
        static inline bool Initialized = false;
        static inline BupDeviceConfig BupConfig[DEVICE_COUNT] = {
            { Unit::Internal,  1 }, // 1 Partition
            { Unit::Cartridge, 1 }, // 1 Partition
            { Unit::Serial,    3 }  // 3 Partitions for FDD
        };
        static inline BupDeviceState BupState[DEVICE_COUNT];

        static inline uint32_t* LibrarySpace = nullptr;
        static inline uint32_t* WorkSpace = nullptr;

        BupFile File;
                        // constructor
        Device
        (
            unsigned char* const name,
            unsigned char* const comment,
            uint32_t datasize,
            uint8_t language = English
        ) : File()
        {
            if (Init())
            {
                // try to mount devices and get their status
                Mount(InternalMemoryBackup);
                Mount(CartridgeMemoryBackup);
                Mount(ExternalDeviceBackup); // FDD
            }
                                    // Init BUP file
            memcpy(File.Name, name, strlen((char*)name) + 1);
            memcpy(File.Comment, comment, strlen((char*)comment) + 1);
            File.Datasize = datasize;
            File.Language = language;
        }
                // Refreshes and returns the BupStatusTable for the specified device
        static bool CheckStatus(BupDevice device)
        {
            int32_t result = Status(device, &BupState[device].StatTable);
            return (result == Success);
        }

        int32_t Save(BupDevice device, void* data, bool overwrite = true)
        {
            if (!BupState[device].isMounted)
                return 1;
            File.Data = data;
            SRL::SMPC::DisableReset();
            int32_t ret = Write(device, &File, overwrite);
            SRL::SMPC::EnableReset();
            return ret;
        }

        int32_t FileExists(BupDevice device, const unsigned char *filename)
        {
            BupWriteTable tempTable{};

            // Determine how many partitions this device has
            uint16_t partitionCount = BupConfig[device].partition;

            for (uint16_t p = 0; p < partitionCount; ++p)
            {
                // Select target partition on the device
                SelectPartition(device, p);

                int32_t count = Dir(device, filename, &tempTable, 1);
                if (count > 0)
                {
                    // Restore back to partition 0 before exiting
                    if (p != 0) SelectPartition(device, 0);
                    return BupStatus::Success;
                }
            }

            // Reset active partition back to default
            if (partitionCount > 1) SelectPartition(device, 0);

            return BupStatus::NotFound;
        }

        int32_t Read(BupDevice device, unsigned char *filename, uint8_t *data)
        {
            return ReadFile(device, filename, data);
        }

        int32_t Delete(BupDevice device, unsigned char *filename)
        {
            return DeleteFile(device, filename);
        }

        int32_t ChangePartition(BupDevice device, uint16_t partition)
        {
            if (partition >= BupConfig[device].partition) {
                return BupStatus::NoMatch; // partition is out of bounds
            }
            return SelectPartition(device, partition);
        }
                bool Format(BupDevice device)
        {
            if (!Initialized) return false;

            BupState[device].Status = Status(device, &BupState[device].StatTable);
                        if (BupState[device].Status == Unformatted)
            {
                if (FormatDevice(device) == Failed)
                {
                    // SRL::Debug::Print(2, 20, "Backup Device Not Formatted");
                    return false;
                }
                else
                {
                    BupState[device].isMounted = (BupState[device].Status == Success); // true or false
                    return BupState[device].isMounted;
                }
            }
            return false;
        }
        
        static bool ReturnStatus(BupDevice device)
        {
            if (!Initialized) return false;
                        BupState[device].Status = Status(device, &BupState[device].StatTable);
            BupState[device].isMounted = (BupState[device].Status == Success); // true or false
            return BupState[device].isMounted;
        }
        
        // Gets directory information for one or more files from the BUP device
        // Pass filename prefix (or "" for all files) and an array of BupWriteTable
        // Returns total matching files found
        static int32_t DirInfo(BupDevice device, const unsigned char *filename, BupWriteTable *outTable, uint16_t maxEntries)
        {
            if (!outTable || maxEntries == 0)
                return BupStatus::Failed;

            return Dir(device, filename, outTable, maxEntries);
        }
                // // example for usage of DirInfo
        // // Read up to 16 files from internal backup memory
        // BupWriteTable dirList[16]{};
        // int32_t fileCount = Device::DirInfo(InternalMemoryBackup, (const unsigned char*)"", dirList, 16);

        // if (fileCount > 0) {
            // for (int i = 0; i < fileCount; ++i) {
                // // dirList[i].filename, dirList[i].datasize, dirList[i].date are now populated
            // }
        // }
        
        // Expands the raw backup timestamp into SRL::Types::DateTime
        static DateTime GetFileDateTime(uint32_t rawDate)
        {
            BupDate dateTable{}; 
            GetDate(rawDate, &dateTable);
            return DateTime(&dateTable); // use SRL::Types::DateTime::BupDate instead
        }

        // Verifies data written to the BUP device (checksum)
        // returns BupStatusTable
        static int32_t Verify(BupDevice device, uint8_t *filename, uint8_t *data)
        {
            return VerifyFile(device, filename, data);
        }

        static void Unmount(void)
        {
            for (auto& device : BupState)
                device.isMounted = false;

            SRL::Memory::Free(LibrarySpace);
            SRL::Memory::Free(WorkSpace);

            Initialized = false;
        }

        // need "delete" for object?
    };
}