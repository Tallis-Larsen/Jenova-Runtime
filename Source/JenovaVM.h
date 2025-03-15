#pragma once

// Jenova VM SDK
namespace jenova::vm
{
    // Settings
    static constexpr bool BypassMode                        = true;
    static constexpr bool SnapshotMode                      = true;

    // Solid Settings
    constexpr uint64_t DEFAULT_EMULATOR_CONSOLE_INDEX       = 0;
    constexpr uint64_t PASS_ITERATION_RESET_LIMIT           = 1000000000;
    constexpr uint64_t BLOCK_INPUT_BUFFER_SIZE              = 512;
    constexpr uint64_t BLOCK_OUTPUT_BUFFER_SIZE             = 512;

    // Command IDs
    constexpr auto JENOVA_VMCMD_INPUT                       = 0x0500;
    constexpr auto JENOVA_VMCMD_RELEASE_KEYS                = 0x0520;
    constexpr auto JENOVA_VMCMD_START                       = 0x0014;
    constexpr auto JENOVA_VMCMD_SHUTDOWN                    = 0x0015;
    constexpr auto JENOVA_VMCMD_KILL                        = 0x0666;
    constexpr auto JENOVA_ENABLE_CONSOLE                    = 0x0001;

    // Enums
    enum class JenovaOperatorState
    {
        Unknown,
        Idle,
        Busy,
        Error,
        Wait,
        Successed,
        Done,
        Release
    };
    enum class JenovaInputDataType
    {
        UpdateButtons,
        IsAbsolute,
        QueueAbsolute,
        QueueRelative,
        QueueButton,
        EventSync,
        SendKey,
        ReleaseKeys,
        ReleaseModifierKeys
    };
    enum class JenovaInputAxis 
    {
        INPUT_AXIS_X,
        INPUT_AXIS_Y,
        INPUT_AXIS__MAX
    };
    enum class JenovaInputButton 
    {
        INPUT_BUTTON_LEFT,
        INPUT_BUTTON_MIDDLE,
        INPUT_BUTTON_RIGHT,
        INPUT_BUTTON_WHEEL_UP,
        INPUT_BUTTON_WHEEL_DOWN,
        INPUT_BUTTON_SIDE,
        INPUT_BUTTON_EXTRA,
        INPUT_BUTTON_WHEEL_LEFT,
        INPUT_BUTTON_WHEEL_RIGHT,
        INPUT_BUTTON_TOUCH,
        INPUT_BUTTON__MAX,
    };

    // Structs
    struct JenovaDisplayData
    {
        uint8_t* framebuffer = 0;
        int width, height, stride;
        bool valid = false;
    };
    struct JenovaInputData
    {
        JenovaInputDataType dataType;
        void* console;
        uint32_t* buttonMap;
        uint32_t prevState;
        uint32_t currState;
        JenovaInputAxis axis;
        int value;
        int minVal;
        int maxVal;
        JenovaInputButton button;
        bool pressed;
        uint32_t keyCode;
    };
    struct JenovaDataPacket
    {
        void* dataPtr = nullptr;
        int dataSize = 0;
        int extra;
    };
    struct JenovaCommandParams
    {
        int command;
        JenovaDataPacket dataPacket;
    };
    
    // Packed Structs
    volatile struct alignas(8) JenovaMemoryBlock
    {
        // Encrypted for Operator
        #ifdef JENOVA_OPERATOR
            char signature[16] = { 0x6F, 0x60, 0x6B, 0x6A, 0x73, 0x64, 0x67, 0x69, 0x6A, 0x66, 0x6E, 0x67, 0x60, 0x62, 0x6C, 0x6B };
        #else
            const char signature[16] = { 'J', 'E', 'N', 'O', 'V', 'A', 'B', 'L', 'O', 'C', 'K', 'B', 'E', 'G', 'I', 'N' };
        #endif

        const char developer[16] = { 'H', 'A', 'M', 'I', 'D', '.', 'M', 'E', 'M', 'A', 'R', '-', '2', '0', '2', '4' };
        size_t blockSize = 0;
        size_t metaCount = 0;

        // Guest Operator
        JenovaOperatorState guestOperatorState = JenovaOperatorState::Unknown;
        uint64_t currentGuestCommandID = 0;
        uint64_t currentGuestPassIterationCount = 0;
        int32_t currentGuestErrorCode = 0;

        // Host Operator
        JenovaOperatorState hostOperatorState = JenovaOperatorState::Unknown;
        uint64_t currentHostCommandID = 0;
        uint64_t currentHostPassIterationCount = 0;
        int32_t currentHostErrorCode = 0;

        // Storages
        uint8_t currentGuestCommandInputData[BLOCK_INPUT_BUFFER_SIZE]           = { 0 };
        uint8_t currentGuestCommandOutputData[BLOCK_OUTPUT_BUFFER_SIZE]         = { 0 };
        uint8_t currentHostCommandInputData[BLOCK_INPUT_BUFFER_SIZE]            = { 0 };
        uint8_t currentHostCommandOutputData[BLOCK_OUTPUT_BUFFER_SIZE]          = { 0 };
    };

    // External Functions Definitions
    typedef int (*JenovaAPI_InitializeEmulatorFunc)(void* interfacePtr);
    typedef int (*JenovaAPI_SetDisplayUpdateStateFunc)(int state);
    typedef JenovaDisplayData(*JenovaAPI_GetDisplayDataFunc)(int consoleIndex);
    typedef int(*JenovaAPI_GetConsoleCountFunc)(void);
    typedef void*(*JenovaAPI_GetConsolePtrFunc)(int consoleIndex);
    typedef JenovaDataPacket(*JenovaAPI_ExecuteCommandFunc)(int commandID, JenovaDataPacket data);
    typedef void*(*JenovaAPI_GetRAMBaseAddressFunc)(void);
    typedef size_t(*JenovaAPI_GetRAMSizeFunc)(void);

    // External Functions
    static jenova::vm::JenovaAPI_InitializeEmulatorFunc            JenovaAPI_InitializeEmulator            = nullptr;
    static jenova::vm::JenovaAPI_ExecuteCommandFunc                JenovaAPI_ExecuteCommand                = nullptr;
    static jenova::vm::JenovaAPI_SetDisplayUpdateStateFunc         JenovaAPI_SetDisplayUpdateState         = nullptr;
    static jenova::vm::JenovaAPI_GetDisplayDataFunc                JenovaAPI_GetDisplayData                = nullptr;
    static jenova::vm::JenovaAPI_GetConsoleCountFunc               JenovaAPI_GetConsoleCount               = nullptr;
    static jenova::vm::JenovaAPI_GetConsolePtrFunc                 JenovaAPI_GetConsolePtr                 = nullptr;
    static jenova::vm::JenovaAPI_ExecuteCommandFunc                JenovaAPI_ExecuteDisplayCommand         = nullptr;
    static jenova::vm::JenovaAPI_GetRAMBaseAddressFunc             JenovaAPI_GetRAMBaseAddress             = nullptr;
    static jenova::vm::JenovaAPI_GetRAMSizeFunc                    JenovaAPI_GetRAMSize                    = nullptr;

    // Error Codes
    constexpr uint64_t OPERATOR_ERROR_INVALID_COMMAND_ID = 8000100;

    // Command IDs
    constexpr uint64_t OPERATOR_COMMAND_SHOW_MESSAGE_BOX = 1000100;

    // Command Input Data Packages
    struct CMDIDATA_SHOW_MESSSAGE_BOX
    {
        int messageBoxFlags = 0;
        char message[2048] = { 0 };
        char title[128] = { 0 };
    };

    // Command Output Data Packages
    struct CMDODATA_GENERIC_OUTPUT
    {
        bool result_bool = false;
        int result_int = 0;
        float result_float = 0;
        uint64_t result_uint64 = 0;
    };
    struct CMDODATA_GENERIC_BOOLEAN
    {
        bool result = false;
    };

    // Shared Functions
    std::string GetErrorReason(int32_t errorCode);
}