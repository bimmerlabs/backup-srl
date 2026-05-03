// TODO: check STN-9 https://docs.exodusemulator.com/Archives/SSDDV25/segahtml/index.html?page=info/hon/stn09.htm
#pragma once
#include <smpc.hpp>

using namespace SRL::Math::Types;

namespace SRL::Backup {
    inline volatile uint32_t &BupLibAdress = *reinterpret_cast<volatile uint32_t *>(0x6000358);

    // Constants
    constexpr size_t MAX_FILENAME_LENGTH = 12; // must be 11 characters or less
    constexpr size_t MAX_COMMENT_LENGTH  = 11; // must be 10 characters or less
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
        Unformatted,    // probably only applicable to FDD
        WriteProtected, // probably only applicable to FDD
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
        unsigned char Name[MAX_FILENAME_LENGTH] = {};
        unsigned char Comment[MAX_COMMENT_LENGTH] = {};
        void *Data = nullptr;
        uint32_t Datasize = 0;
        uint8_t  Language;
        // uint16_t BlockSize = 0; // from jo engine, probably not needed
    };

    struct BupDate {
        uint8_t year, month, day;
        uint8_t hour, minute, weekday;
    };

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
        // not sure if this is ever used outside of core functions (see DirInfo)
        struct BupWriteTable
        {
            uint8_t	    filename[MAX_FILENAME_LENGTH];	/* File name */
            uint8_t	    comment[MAX_COMMENT_LENGTH];	/* Comments */
            uint8_t	    language;	    /* The language type of the comment */
            uint32_t    date;		    /* Time stamp */
            uint32_t	datasize;	    /* Data size (Byte) */
            uint16_t	blocksize;	    /* Data size (blocks) */
        };

        // Templates (see sega_bup.h)
        using BupInit = void (*)(volatile uint32_t* lib, uint32_t* work, BupDeviceConfig config[3]);
        using BupSelPart = int32_t (*)(uint32_t device, uint16_t partition);
        using BupFormat = int32_t (*)(uint32_t device);
        using BupStat = int32_t (*)(uint32_t device, uint32_t dummy, BupStatusTable *table);
        using BupWrite = int32_t (*)(uint32_t device, BupWriteTable *table, volatile uint8_t *data, bool overwrite);
        using BupExists = int32_t (*)(uint32_t device, uint8_t *filename, volatile uint8_t *dummy);
        using BupRead = int32_t (*)(uint32_t device, uint8_t *filename, volatile uint8_t *data);
        using BupDelete = int32_t (*)(uint32_t device, uint8_t *filename);
        using BupDir = int32_t (*)(uint32_t device, uint8_t *filename, uint16_t tbsize, BupWriteTable *table);
        using BupVerify = int32_t (*)(uint32_t device, uint8_t *filename, uint8_t *data);
        using BupGetDate = void (*)(uint32_t device, BupDate *table);
        using BupSetDate = int32_t (*)(BupDate *table);

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
                                    // get date (check implementation in jo engine)
            // jo engine complicates this for no reason
            DateTime time = DateTime::Now();
            // doesn't currently work, need to figure out how to get SetDate to take SRL::Types::DateTime   
            // WriteTable.date = SetDate(time.ToBackupUnitDate());
            BupDate dateTable
            {
                static_cast<uint8_t>(time.Year() - 1980),
                time.Month(),
                time.Day(),
                time.Hour(),
                time.Minute(),
                time.Week()
            };
            WriteTable.date = SetDate(&dateTable);            
            
            WriteTable.datasize = file->Datasize; 
            // WriteTable.blocksize = file->Datasize * 64;  // not in sample so leave it out?
                        // is this only needed for the FDD?
            // SelectPartition(device, 0);
            
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
        
        // Expands the date and time data in the BupDate table
        // Get backup device last edit date
        // returns nothing
        static void GetDate(BupDevice device, BupDate *table)
        {
            auto func = GetBupFunction<BupGetDate>(36);
            func(device, table);
        }

        // Compresses the date and time data into the BupDate table
        // Prepares date for writetable
        // returns nothing
        static uint8_t SetDate(BupDate *table)
        {
            auto func = GetBupFunction<BupSetDate>(40);
            return func(table);
        }
        
        static bool Init()
        {
            if (Initialized) return true; // causes it to crash
                        
            LibrarySpace = static_cast<uint32_t*>(SRL::Memory::HighWorkRam::Malloc(LIB_SPACE_SIZE));
            if (!LibrarySpace) return false;
            
            WorkSpace = static_cast<uint32_t*>(SRL::Memory::LowWorkRam::Malloc(WORK_SPACE_SIZE));
            if (!WorkSpace) return false;

            SRL::SMPC::DisableReset();

            DriverInit(LibrarySpace, WorkSpace, BupConfig);

            SRL::SMPC::EnableReset();

            for (auto& device : BupState)
                device.isMounted = false;

            Initialized = true; // causes it to crash            
            return (Initialized); // causes it to crash
            // return true; // works
        }        
                static bool Mount(BupDevice device)
        {
            if (Initialized)
            {
                // instead of doing it here, have a "check for free space/blocks" function
                BupState[device].Status = Status(device, &BupState[device].StatTable);
                
                if (BupState[device].Status == Unformatted)
                {
                    // if (Format(device) == Failed)
                    // {
                        SRL::Debug::Print(2, 20, "Backup Device Not Formatted");
                        return false;
                    // }
                }

                BupState[device].isMounted = (BupState[device].Status == Success); // true or false

                return BupState[device].isMounted;
            }
            else
            {
                return false;
            }
        }

    public:
        static inline bool Initialized = false;
        static inline BupDeviceConfig BupConfig[DEVICE_COUNT] = {};
        static inline BupDeviceState BupState[DEVICE_COUNT];
        // only for debug
        static inline uint32_t* LibrarySpace = nullptr;
        static inline uint32_t* WorkSpace = nullptr;
        
        BupFile File;
        
                // zzz
        // work around - set filename to 13 chars.
        // next attempt:  use original Sega BUP library
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
                // Mount(ExternalDeviceBackup);
            }
            
                        // Init BUP file
            // SRL::Debug::Print(1,22,"len=%d", strlen((char*)name));
            memcpy(File.Name, name, strlen((char*)name) + 1);
            memcpy(File.Comment, comment, strlen((char*)comment) + 1);
            // File.Data = nullptr;
            File.Datasize = datasize;
            File.Language = language;
        }
        
        // bool Save(BupDevice device, BupFile* file, bool overwrite = true)
        int32_t Save(BupDevice device, void* data, bool overwrite = true)
        {
            if (!BupState[device].isMounted)
                return 1;
            File.Data = data;
            SRL::SMPC::DisableReset();
            int32_t ret = Write(device, &File, overwrite);  // whats the status here?
            SRL::SMPC::EnableReset();
            return ret;
        }
        
        // void?
        int32_t FileExists(BupDevice device, unsigned char *filename)
        {
            auto func = GetBupFunction<BupExists>(20);
            return func(device, (uint8_t*)filename, reinterpret_cast<volatile uint8_t*>(1));
        }

        // void?
        int32_t Read(BupDevice device, unsigned char *filename, uint8_t *data)
        {
            auto func = GetBupFunction<BupRead>(20);
            // return func(device, (uint8_t*)filename, reinterpret_cast<volatile uint8_t*>(1));
            return func(device, (uint8_t*)filename, data);
        }
        
                // void?
        int32_t Delete(BupDevice device, unsigned char *filename)
        {
            
            auto func = GetBupFunction<BupDelete>(24);
            return func(device, (uint8_t*)filename);
        }
        
                bool Format(BupDevice device)
        {
            if (Initialized)
            {
                BupState[device].Status = Status(device, &BupState[device].StatTable);
                
                if (BupState[device].Status == Unformatted)
                {
                    if (FormatDevice(device) == Failed)
                    {
                        SRL::Debug::Print(2, 20, "Backup Device Not Formatted");
                        return false;
                    }
                    else
                    {
                        BupState[device].isMounted = (BupState[device].Status == Success); // true or false
                        return BupState[device].isMounted;
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        static bool ReturnStatus(BupDevice device)
        {
            if (Initialized)
            {
                BupState[device].Status = Status(device, &BupState[device].StatTable);
                
                BupState[device].isMounted = (BupState[device].Status == Success); // true or false

                return BupState[device].isMounted;
            }
            else
            {
                return false;
            }
        }        
        // Gets Directory information from BUP device (maybe for FDD use?)
        // returns Directory information
        static int32_t DirInfo(BupDevice device, uint8_t *filename)
        {
            BupWriteTable WriteTable = {};
            uint16_t tbsize; // not sure what should be in here

            auto func = GetBupFunction<BupDir>(28);
            return func(device, filename, tbsize, &WriteTable);
        }

        // Verifies data written to the BUP device (checksum)
        // returns BupStatusTable
        static int32_t Verify(BupDevice device, uint8_t *filename, uint8_t *data)
        {
            auto func = GetBupFunction<BupVerify>(32);
            return func(device, filename, data);
        }

        static void Unmount(void)
        {
            for (auto& device : BupState)
                device.isMounted = false;
                
            SRL::Memory::Free(LibrarySpace);                
            SRL::Memory::Free(WorkSpace);
            
            Initialized = false;
        }
        
        // need "delete" for object
    };
}