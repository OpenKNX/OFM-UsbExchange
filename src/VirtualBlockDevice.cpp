#include "VirtualBlockDevice.h"

VirtualBlockDevice::VirtualBlockDevice(std::string id, OpenKNX::Flash::Driver* flash, uint32_t size)
{
    _size = size;
    _sectorCount = size / 512;
    _flash = flash;
    _prefix = openknx.logger.buildPrefix("VBD", id);
    _sectorCurrent = millis() % flashSectorCount();
}

VirtualBlockDevice::~VirtualBlockDevice()
{
}
void VirtualBlockDevice::clearSectorMap()
{
    _sectorMap = {};
}

std::string VirtualBlockDevice::logPrefix()
{
    return _prefix;
}

bool VirtualBlockDevice::isBusy()
{
    return false;
}

bool VirtualBlockDevice::readSector(uint32_t sector, uint8_t* dst)
{
    return read(sector, 0, 512, dst);
}

bool VirtualBlockDevice::readSectors(uint32_t sector, uint8_t* dst, size_t ns)
{
    for (size_t i = 0; i < ns; i++)
    {
        uint32_t offset = (512 * i);
        if (!readSector(sector + i, dst + offset)) return false;
    }

    return true;
}

uint32_t VirtualBlockDevice::sectorCount()
{
    return _sectorCount;
}

bool VirtualBlockDevice::syncDevice()
{
    _flash->commit();
    return true;
}

bool VirtualBlockDevice::writeSector(uint32_t sector, const uint8_t* src)
{
    return write(sector, 0, 512, src);
}

bool VirtualBlockDevice::writeSectors(uint32_t sector, const uint8_t* src, size_t ns)
{
    for (size_t i = 0; i < ns; i++)
    {
        uint32_t offset = (512 * i);
        if (!writeSector(sector + i, src + offset)) return false;
    }
    return true;
}

bool VirtualBlockDevice::read(uint32_t sector, uint32_t offset, uint32_t size, uint8_t* dst)
{
    auto it = _sectorMap.find(sector);
    if (it != _sectorMap.end())
    {
        uint32_t pos = (it->second * 512) + offset;
        _flash->read(pos, dst, size);
    }
    else
    {
        memset(dst, 0xFF, size);
    }
    return true;
}

bool VirtualBlockDevice::write(uint32_t sector, uint32_t offset, uint32_t size, const uint8_t* src)
{
    uint32_t index = 0;

    auto it = _sectorMap.find(sector);
    if (it != _sectorMap.end())
    {
        index = it->second;
    }
    else
    {
        if (_sectorMap.size() >= flashSectorCount()) return false;
        index = _sectorCurrent;
        _sectorCurrent++;
        if (_sectorCurrent >= flashSectorCount()) _sectorCurrent = 0;
        _sectorMap.insert(std::make_pair(sector, index));
    }

    uint32_t pos = (index * 512) + offset;
    _flash->write(pos, (uint8_t*)src, size);
    return true;
}
