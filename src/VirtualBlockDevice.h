#pragma once
#include "ExFatLib/ExFatLib.h"
#include "FatLib/FatFormatter.h"
#include "FatLib/FatLib.h"
#include "FatLib/FatPartition.h"
#include "FsLib/FsLib.h"
#include "OpenKNX.h"
#include "SDFS.h"
#include "SdCard/SdCard.h"
#include "common/BlockDevice.h"
#include <string>

class VirtualBlockDevice : public BlockDevice
{
  public:
    VirtualBlockDevice(std::string id, OpenKNX::Flash::Driver* flash, uint32_t size);
    ~VirtualBlockDevice();
    bool isBusy() override;
    bool readSector(uint32_t sector, uint8_t* dst) override;
    bool readSectors(uint32_t sector, uint8_t* dst, size_t ns) override;
    uint32_t sectorCount() override;
    bool syncDevice() override;
    bool writeSector(uint32_t sector, const uint8_t* src) override;
    bool writeSectors(uint32_t sector, const uint8_t* src, size_t ns) override;

    bool read(uint32_t sector, uint32_t offset, uint32_t size, uint8_t* dst);
    bool write(uint32_t sector, uint32_t offset, uint32_t size, const uint8_t* src);
    inline uint32_t flashSectorCount() { return (_flash->size() / 512); }
    std::string logPrefix();

  private:
    uint32_t _sectorCount = 0;
    uint32_t _size = 0;
    std::string _prefix;
    OpenKNX::Flash::Driver* _flash;
    std::unordered_map<uint32_t, uint32_t> _sectorMap = {};
    uint32_t _sectorCurrent = 0;
};