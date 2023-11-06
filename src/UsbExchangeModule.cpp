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
    openknx.common.skipLooptimeWarning();
    if (!_blockDevice->write(lba, offset, size, buffer))
    {
        eject();
        return -1;
    };
    return size;
}

void UsbExchangeModule::mscFlush()
{
    activity();
    openknx.common.skipLooptimeWarning();
    _blockDevice->syncDevice();
}

bool UsbExchangeModule::mscStartStop(uint8_t power_condition, bool start, bool load_eject)
{
    (void)power_condition;

    activity();
    if (load_eject && !start) eject();

    return true;
}

bool UsbExchangeModule::mscReady()
{
    return _ready;
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
    _logger = new OpenKNX::Log::VirtualSerial("UsbExchange", 100);

    _blockDevice = new VirtualBlockDevice("Exchange", &_flash, EXCHANGE_FS_SIZE);
    logIndentDown();

    openknx.progButton.onDoubleClick([]() -> void {
        openknxUsbExchangeModule.toggle();
    });

    onLoad("Readme.txt", openknxUsbExchangeModule.fillReadmeFile);
    onLoad("Support.txt", openknxUsbExchangeModule.fillSupportFile);
    onLoad("Flash.txt", openknxUsbExchangeModule.fillFlashFile);
}

void UsbExchangeModule::fillReadmeFile(UsbExchangeFile* file)
{
    file->write("Bla");
}

void UsbExchangeModule::fillSupportFile(UsbExchangeFile* file)
{
    file->write("Bla");
}

void UsbExchangeModule::fillFlashFile(UsbExchangeFile* file)
{
    file->write("Bla");
}

void UsbExchangeModule::loop(bool configured)
{
    processLoading();
    processEjecting();
}

void UsbExchangeModule::activity()
{
    _activity = millis();
}

bool UsbExchangeModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.substr(0, 8) == "exchange")
    {
        toggle();
        return true;
    }
    return false;
}

void UsbExchangeModule::toggle()
{
    if (_loading) return;
    if (_ejecting) return;

    _status ? eject() : load();
}

void UsbExchangeModule::eject()
{
    if (!_status) return;
    if (_loading) return;
    if (_ejecting) return;

    _ejecting = 1;
    _status = false;
}

void UsbExchangeModule::load()
{
    if (_status) return;
    if (_loading) return;
    if (_ejecting) return;
    _loading = 1;
    _status = true;
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

void UsbExchangeModule::onLoad(std::string filename, FileOnLoadCallback callback)
{
    _filesOnLoad.push_back(std::make_pair(filename, callback));
}

void UsbExchangeModule::onEject(std::string filename, FileOnEjectCallback callback)
{
    _filesOnEject.push_back(std::make_pair(filename, callback));
}

void UsbExchangeModule::processEjecting()
{
    if (!_ejecting) return;
    if (_ejecting == 1) logInfoP("Ejecting usb storage");
    if (_ejecting > 0) openknx.common.skipLooptimeWarning();
    logIndentUp();

    const uint8_t shiftCallback = 3;

    if (_ejecting == 1)
    {
        openknx.progLed.pulsing();

        _vol.begin(_blockDevice);
        logInfoP("Show files");
        logIndentUp();
        _vol.ls(_logger, LS_SIZE | LS_R);
        logIndentDown();
        _ready = false;
        _ejectingError = false;
    }
    else if (_ejecting == 2)
    {
        // TODO Optimize over multple loops
        // TODO Directory support
        FatFile dir = _vol.open("/Inbox");
        if (dir && dir.isDir())
        {
            logInfoP("Copy inbox to internal flash");
            FatFile source;
            while (source.openNext(&dir, O_RDONLY))
            {
                if (source.isFile())
                {
                    logIndentUp();
                    size_t len = 0;
                    char buf[512] = {'/'};

                    source.getName(buf + 1, 50);
                    File target = LittleFS.open(buf, "w");
                    if (target)
                    {
                        logInfoP("copy %s", buf);
                        while (len = source.read(buf, 512))
                            target.write(buf, len);

                        target.close();
                    }
                    else
                    {
                        logErrorP("%s was not copied", buf);
                        _ejectingError = true;
                    }

                    logIndentDown();
                    source.close();
                }
            }
        }
    }
    else if (_ejecting >= shiftCallback && _ejecting < shiftCallback + _filesOnEject.size())
    {
        auto entry = _filesOnEject[_ejecting - shiftCallback];

        UsbExchangeFile file = _vol.open(entry.first.c_str(), O_READ);
        if (file)
        {
            logInfoP("Read file: %s", entry.first.c_str());
            logIndentUp();
            _ejectingError |= !entry.second(&file);
            logIndentDown();
            file.close();
        }
        else
        {
            logInfoP("File not found: %s", entry.first.c_str());
            logIndentUp();
            _ejectingError |= !entry.second(nullptr);
            logIndentDown();
        }
    }
    else
    {
        if (_ejectingError)
            logErrorP("Ejecting completed with errors!");
        else
            logInfoP("Ejecting completed");

        openknx.progLed.off();
        _ejecting = 0;
        goto Done;
    }

    _ejecting++;
Done:
    logIndentDown();
}

void UsbExchangeModule::processLoading()
{
    if (!_loading) return;
    if (_loading == 1) logInfoP("Load usb storage");
    if (_loading > 0) openknx.common.skipLooptimeWarning();
    logIndentUp();

    const uint8_t shiftCallback = 2;

    if (_loading == 1)
    {
        openknx.progLed.pulsing();
        logInfoP("Start formatting");
        logIndentUp();
        doFormat();
        _vol.begin(_blockDevice);
        _vol.mkdir("/Inbox");
        logIndentDown();
    }
    else if (_loading >= shiftCallback && _loading < shiftCallback + _filesOnLoad.size())
    {
        auto entry = _filesOnLoad[_loading - shiftCallback];
        logInfoP("Create file: %s", entry.first.c_str());

        logIndentUp();
        UsbExchangeFile file = _vol.open(entry.first.c_str(), O_WRITE | O_TRUNC | O_CREAT);
        if (file)
        {
            entry.second(&file);
            file.close();
        }
        else
        {
            entry.second(nullptr);
        }
        logIndentDown();
    }
    else
    {
        _loading = 0;
        _ready = true;
        _blockDevice->syncDevice();
        logInfoP("Loading completed");
        openknx.progLed.activity(_activity);
        goto Done;
    }

    _loading++;
Done:
    logIndentDown();
}

void UsbExchangeModule::doFormat()
{
    FatFormatter formatter;
    uint8_t sectorBuffer[512] = {};
    _blockDevice->clearSectorMap();
    formatter.format(_blockDevice, (uint8_t*)sectorBuffer, _logger);
}

UsbExchangeModule openknxUsbExchangeModule;