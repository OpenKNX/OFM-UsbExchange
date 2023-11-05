#include "UsbExchangeModule.h"

#include "class/msc/msc.h"
#include "class/msc/msc_device.h"

void writeLineToFile(FatFile file, const char* line, ...)
{
    char buf[100];
    memset(buf, 0x0, 100);
    va_list values;
    va_start(values, line);
    vsnprintf(buf, 100, line, values);
    file.write(buf);
    file.write("\n\r");
    va_end(values);
}

// Activate
void __USBInstallMassStorage() {}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    (void)lun;

    const char vid[] = "OpenKNX";
    const char pid[] = "USB Exchange";
    const char rev[] = "1.0";

    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    if (lun == 0) return openknxUsbExchangeModule.mscReady();
    return false;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
    if (lun == 0)
    {
        *block_count = EXCHANGE_FS_SIZE / 512;
        *block_size = 512;
    }
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    if (lun == 0) return openknxUsbExchangeModule.mscStartStop(power_condition, start, load_eject);
    return false;
}
bool tud_msc_is_writable_cb(uint8_t lun)
{
    if (lun == 0) return true;
    return false;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t size)
{
    if (lun == 0)
    {
        return openknxUsbExchangeModule.mscRead(lba, offset, (uint8_t*)buffer, size);
    }

    return -1;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t size)
{
    if (lun == 0)
    {
        return openknxUsbExchangeModule.mscWrite(lba, offset, buffer, size);
    }
    return -1;
}

void tud_msc_write10_complete_cb(uint8_t lun)
{
    if (lun == 0) openknxUsbExchangeModule.mscFlush();
}

int32_t UsbExchangeModule::mscRead(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t size)
{
    if (!_status) return -1;
    activity();
    if (!_blockDevice->read(lba, offset, size, buffer)) return -1;
    return size;
}

int32_t UsbExchangeModule::mscWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t size)
{
    if (!_status) return -1;
    activity();
    if (!_blockDevice->write(lba, offset, size, buffer))
    {
        deactivate();
        return -1;
    };
    return size;
}

void UsbExchangeModule::mscFlush()
{
    activity();
    _blockDevice->syncDevice();
}

bool UsbExchangeModule::mscStartStop(uint8_t power_condition, bool start, bool load_eject)
{
    (void)power_condition;

    activity();
    if (load_eject && !start) deactivate();

    return true;
}

bool UsbExchangeModule::mscReady()
{
    return _status;
}

const std::string UsbExchangeModule::name()
{
    return "UsbExchange";
}

const std::string UsbExchangeModule::version()
{
    return MODULE_UsbExchange_Version;
}

void UsbExchangeModule::setup(bool configured)
{
    logInfoP("Inizialize usb exchange flash");
    logIndentUp();

    _flash.init("Exchange", EXCHANGE_FLASH_OFFSET, EXCHANGE_FLASH_SIZE);
    _loggerFat = new OpenKNX::Log::VirtualSerial("Fat<Exchange>", 100);

    _blockDevice = new VirtualBlockDevice("Exchange", &_flash, EXCHANGE_FS_SIZE);
    logIndentDown();

    openknx.progButton.onDoubleClick([]() -> void {
        openknxUsbExchangeModule.toggleExchangeMode();
    });
}

void UsbExchangeModule::loop(bool configured)
{
}

void UsbExchangeModule::activity()
{
    _activity = millis();
}

bool UsbExchangeModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.substr(0, 8) == "exchange")
    {
        toggleExchangeMode();
        return true;
    }
    if (cmd.substr(0, 11) == "exchange ls")
    {
        deactivate();
        FatVolume vol;
        vol.begin(_blockDevice);
        vol.ls(_loggerFat);
        return true;
    }
    return false;
}

void UsbExchangeModule::toggleExchangeMode()
{
    _status ? deactivate() : activate();
}

void UsbExchangeModule::deactivate()
{
    logInfoP("deactivate");
    _blockDevice->syncDevice();
    logIndentUp();
    logInfoP("copy files");
    FatVolume vol;
    vol.begin(_blockDevice);
    vol.ls(_loggerFat);
    logIndentDown();
    openknx.progLed.off();
    _status = false;
}

void UsbExchangeModule::activate()
{
    logInfoP("activate usb storage");
    logIndentUp();
    logInfoP("format");
    logIndentUp();

    // format
    FatFormatter formatter;
    uint8_t sectorBuffer[512] = {};
    formatter.format(_blockDevice, (uint8_t*)sectorBuffer, _loggerFat);
    logInfoP("format");

    logIndentDown();
    logInfoP("create info files");
    FatVolume vol;
    vol.begin(_blockDevice);
    FatFile file = vol.open("/Readme.txt", O_WRITE | O_TRUNC | O_CREAT);
    if (file)
    {
        file.write("Test bla bla");
        file.close();
    }
    else
    {
        logErrorP("create file failed");
    }
    writeSupportFile(vol);
    _blockDevice->syncDevice();

    logIndentDown();
    _status = true;
    
    openknx.progLed.activity(_activity);
}

void UsbExchangeModule::writeSupportFile(FatVolume& vol)
{
    FatFile file = vol.open("/Support.txt", O_WRITE | O_TRUNC | O_CREAT);
    writeLineToFile(file, "= Versions =");
    writeLineToFile(file, "Firmware: %s", openknx.info.humanFirmwareVersion(true));
    writeLineToFile(file, "KNX: %s", KNX_Version);
    writeLineToFile(file, "%s: %s", openknx.common.logPrefix(), MODULE_Common_Version);
    for (uint8_t i = 0; i < openknx.modules.count; i++)
    {
        if (openknx.modules.list[i]->version().empty()) continue;
        writeLineToFile(file, "%s: %s", openknx.modules.list[i]->name().c_str(), openknx.modules.list[i]->version().c_str());
    }
    file.close();
}

UsbExchangeModule openknxUsbExchangeModule;