#pragma once
#include "OpenKNX.h"
#include "VirtualBlockDevice.h"
#include "common/SysCall.h"
#include <LittleFS.h>

class UsbExchangeModule : public OpenKNX::Module
{
  public:
    const std::string name() override;
    const std::string version() override;
    void loop(bool configured) override;
    void setup(bool configured) override;
    bool processCommand(const std::string cmd, bool diagnoseKo);

    void activity();

    bool mscReady();
    void mscFlush();
    int32_t mscRead(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t size);
    int32_t mscWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t size);
    bool mscStartStop(uint8_t power_condition, bool start, bool load_eject);
    void deactivate();
    void activate();
    void toggleExchangeMode();

  private:
    bool _firstRun = true;
    uint32_t _activity = 0;
    bool _status = false;
    VirtualBlockDevice* _blockDevice = nullptr;
    OpenKNX::Flash::Driver _flash;
    OpenKNX::Log::VirtualSerial* _loggerFat = nullptr;

    void writeSupportFile(FatVolume& vol);
};

extern UsbExchangeModule openknxUsbExchangeModule;