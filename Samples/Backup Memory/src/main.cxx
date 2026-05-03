#include <srl.hpp>
#include <backup.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Backup;

// Using to shorten names for input
using namespace SRL::Input;

const char *deviceName[] = { // only used for testing / this demo - not neccesary for using the Bup library
    "InternalMemory",
    "CartridgeMemory",
    "ExternalDevice"
};

const char *deviceStatus[] = { // only used for testing / this demo - not neccesary for using the Bup library
        "Success",
        "Failed",
        "Unformatted",
        "WriteProtected",
        "NotEnoughMemory",
        "NotFound",
        "Found",
        "NoMatch",
        "Broken"
};

typedef struct {
    bool debug_mode;
    bool debug_display;
    bool testCollision;
    bool mesh_display;
    bool mosaic_display;
    bool use_rtc;
    bool unlockBigHeadMode;
    bool bigHeadMode;
    bool enableItems;
    bool enableMeows;
    bool reservedBool;
    unsigned int bombTouchCounter;
    unsigned int fishTouchCounter;
    unsigned int redShroomTouchCounter;
    unsigned int blueShroomTouchCounter;
    unsigned int craigTouchCounter;
    unsigned int garfTouchCounter;
    unsigned int reservedInt;
} GameOptions;

typedef struct _SAVEGAME 
{
    // zzz
    GameOptions Options;  // I think the disconnect is it is not actuall yreading the current game options struct
    bool characterUnlocked[12];
    int32_t highScores[10];
    uint8_t inputSettings[12];
} SaveGame;

SaveGame saveGame = {};

// work around - set filename to 13 chars.
// next attempt:  use original Sega BUP library
unsigned char FileName[13] = "Test1234567"; /* (11 ASCII characters + NUL, total 12 bytes) */
unsigned char Comment[11] = "Comment12"; /* (10 ASCII characters + NUL, total 11 bytes) */
// Main program entry 
int main()
{    
    // Initialize library
    SRL::Core::Initialize(HighColor::Colors::Black);
        
    Digital port0(0);
    
    Device* bup = new Device(FileName, Comment, sizeof(SaveGame), English);
    // Device* bup = new Device();
    BupDevice currentDevice = ExternalDeviceBackup;
    
    while(1)
    {
        if (port0.IsConnected())
        {
            // Mount / Unmount
            if (port0.WasPressed(Digital::Button::START))
            {
                // if (bup->Initialized) {
                    // bup->Initialized = false;
                // }
                // else {
                    // bup = new Device(FileName, Comment, sizeof(saveGame));
                // }
                SRL::Debug::PrintClearScreen();
                // bup->Device(FileName, Comment, sizeof(saveGame), English);
            }
            
            // Change currentDevice
            if (port0.WasPressed(Digital::Button::L))
            {
                currentDevice = static_cast<BupDevice>(static_cast<int>(currentDevice) - 1);
                if (currentDevice > ExternalDeviceBackup)
                    currentDevice = InternalMemoryBackup;
                SRL::Debug::PrintClearScreen();
            }
            if (port0.WasPressed(Digital::Button::R))
            {
                currentDevice = static_cast<BupDevice>(static_cast<int>(currentDevice) + 1);
                if (currentDevice > ExternalDeviceBackup)
                    currentDevice = ExternalDeviceBackup;
                SRL::Debug::PrintClearScreen();
            }

            // // Save
            // if (port0.WasPressed(Digital::Button::A))
            // {                
                // SRL::Debug::PrintClearScreen();
                // SRL::Debug::Print(3, 20, "Saved: %s             ", deviceStatus[bup->Save(currentDevice, &saveGame)]);
                
            // }            
            if (port0.WasPressed(Digital::Button::A))
            {
                SRL::Debug::PrintClearScreen();
                SRL::Debug::Print(3, 20, "Saved: %s",
                    deviceStatus[bup->Save(currentDevice, &saveGame, true)]);
            }

            // Read
            // if (port0.WasPressed(Digital::Button::B))
            // {
                // SRL::Debug::PrintClearScreen();
                // // SRL::Debug::Print(3, 20, "Read: %s               ", deviceStatus[bup->Read(currentDevice, FileName)]);
                // SRL::Debug::Print(3, 20, "Read: %s               ", deviceStatus[bup->Read(currentDevice, FileName, (uint8_t*)&saveGame)]);
            // }
            if (port0.WasPressed(Digital::Button::B))
            {
                SRL::Debug::PrintClearScreen();

                // int result = bup->Read(currentDevice, FileName, (uint8_t*)&saveGame);

                // if (result == Success)
                    // g_GameOptions = saveGame.g_GameOptions;

                SRL::Debug::Print(3, 20, "Read: %s", deviceStatus[bup->Read(currentDevice, FileName, (uint8_t*)&saveGame)]);
            }

            if (port0.WasPressed(Digital::Button::C))
            {
                // Delete
                SRL::Debug::PrintClearScreen();
                SRL::Debug::Print(3, 20, "Deleted: %s               ", deviceStatus[bup->Delete(currentDevice, FileName)]);
            }
            
            // Check status
            if (port0.WasPressed(Digital::Button::X))
            {                
                SRL::Debug::PrintClearScreen();
                SRL::Debug::Print(3, 20, "Check Status: %d             ", bup->ReturnStatus(currentDevice));
                
            }
            
            // Format
            if (port0.WasPressed(Digital::Button::Z))
            {                
                SRL::Debug::PrintClearScreen();
                SRL::Debug::Print(3, 20, "Format: %d             ", bup->Format(currentDevice));
                
            }
              
            // Options
            if (port0.WasPressed(Digital::Button::Left) || port0.WasPressed(Digital::Button::Right))
            {
                saveGame.Options.debug_mode = !saveGame.Options.debug_mode;
            }
            /* TOGGLE VALUE */
            if (SRL::Input::Digital(0).WasPressed(SRL::Input::Digital::Button::Up))
            {
                saveGame.Options.reservedInt = 0xDEADBEEF;
            }
            if (SRL::Input::Digital(0).WasPressed(SRL::Input::Digital::Button::Down))
            {
                saveGame.Options.reservedInt = 0x12345678;
            }
            
        }
        
        SRL::Debug::Print(1,1, "Saturn Backup");
    
        SRL::Debug::Print(2, 4, "LibrarySpace:     0x%08X", bup->LibrarySpace);
        SRL::Debug::Print(2, 5, "WorkSpace:        0x%08X", bup->WorkSpace);
        
            SRL::Debug::Print(1, 7, "%s:", deviceName[currentDevice]);
            SRL::Debug::Print(2, 8, "id: %d", currentDevice);
            SRL::Debug::Print(2, 9, "partition: %d", bup->BupConfig[currentDevice].partition);
            SRL::Debug::Print(2, 10, "Status (%d): %s", bup->BupState[currentDevice].Status, deviceStatus[bup->BupState[currentDevice].Status]);
            SRL::Debug::Print(2, 11, bup->BupState[currentDevice].isMounted ? "Mounted: True" : "Mounted: False");
            SRL::Debug::Print(2, 12, "BupStatusTable:");
            SRL::Debug::Print(3, 13, "TotalSize %d", bup->BupState[currentDevice].StatTable.TotalSize);
            SRL::Debug::Print(3, 14, "TotalBlocks %d", bup->BupState[currentDevice].StatTable.TotalBlocks);
            SRL::Debug::Print(3, 15, "BlockSize %d", bup->BupState[currentDevice].StatTable.BlockSize);
            SRL::Debug::Print(3, 16, "FreeSize %d", bup->BupState[currentDevice].StatTable.FreeSize);
            SRL::Debug::Print(3, 17, "FreeBlocks %d", bup->BupState[currentDevice].StatTable.FreeBlocks);
            SRL::Debug::Print(3, 18, "WritableCount %d", bup->BupState[currentDevice].StatTable.WritableCount);
        
            SRL::Debug::Print(3, 22, saveGame.Options.debug_mode ? "Debug Mode: On " : "Debug Mode: Off");
            SRL::Debug::Print(3, 23, "reservedInt: %08X", saveGame.Options.reservedInt);
            
        // Refresh screen
        SRL::Core::Synchronize();
    }

    return 0;
}
