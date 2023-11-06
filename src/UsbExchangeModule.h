#pragma once
#include "OpenKNX.h"
#include "VirtualBlockDevice.h"
#include "common/SysCall.h"
#include <LittleFS.h>

typedef FatFile UsbExchangeFile;
typedef std::function<void(UsbExchangeFile*)> FileOnLoadCallback;
typedef std::function<bool(UsbExchangeFile*)> FileOnEjectCallback;

class UsbExchangeModule : public OpenKNX::Module
{
  public:
    const std::string name() override;
    const std::string version() override;
    void loop(bool configured) override;
    void setup(bool configured) override;
    bool processCommand(const std::string cmd, bool diagnoseKo);
    void processEjecting();
    void processLoading();

    void activity();
    void eject();
    void load();
    void toggle();

    void onLoad(std::string filename, FileOnLoadCallback callback);
    void onEject(std::string filename, FileOnEjectCallback callback);

    static void fillReadmeFile(UsbExchangeFile* file);
    static void fillSupportFile(UsbExchangeFile* file);
    static void fillFlashFile(UsbExchangeFile* file);

    bool mscReady();
    void mscFlush();
    int32_t mscRead(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t size);
    int32_t mscWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t size);
    bool mscStartStop(uint8_t power_condition, bool start, bool load_eject);

  private:
    uint32_t _activity = 0;
    bool _status = false;
    bool _ready = false;
    uint8_t _loading = 0;
    uint8_t _ejecting = 0;
    bool _ejectingError = false;
    FatVolume _vol;
    VirtualBlockDevice* _blockDevice = nullptr;
    OpenKNX::Flash::Driver _flash;
    OpenKNX::Log::VirtualSerial* _logger = nullptr;

    std::vector<std::pair<std::string, FileOnLoadCallback>> _filesOnLoad;
    std::vector<std::pair<std::string, FileOnEjectCallback>> _filesOnEject;

    void writeSupportFile(FatVolume& vol);
    void doFormat();
};

extern UsbExchangeModule openknxUsbExchangeModule;