# OFM-UsbExchange

> **_Hinweis:_** Das Modul befindet sich noch in der Entwicklungsphase!

Dieses Modul stellt ein virtuelles Laufwerk bereit, ähnlich einem USB-Stick, über welches Dateien auf den internen Speicher übertragen und auch (eingeschränkt) Dateien bereitgestellt werden können. Beachte jedoch, dass die Daten vorübergehend in einem abgetrennten Flash-Bereich zwischengespeichert werden müssen und der damit verfügbare LittleFS-Speicher begrenzt ist.

Um den virtuellen Speicher zu startet, muss der Benutzer die Prog-Taste doppelt drücken. Daraufhin  wird der Speicher eingerichtet, als FAT16 formatiert und das Dateisystem wie folgt vorbefüllt:

- **Inbox**  *Hier können Dateien abgelegen, welche im Anschluss auf den internen Speicher umkopiert werden.*
- **Readme.txt**  *Paar Informationen zur Nutzung*
- **Support.txt**  *Informationen über das System (Versionen, Zustände etc.)*
- **Flash.txt**  *Inhaltsverzeichnis des internen LittleFS Speicher.*

Die Vorbefüllung kann darüber hinaus von Modulen mittels eines Callbacks um eigene Dateien ergänzt werden.

Beim Auswerfen bzw. erneuten Doppelklick auf die Prog-Taste, wird das Laufwerk ausgeworfen. Im Anschluss werden die Dateien aus dem Inbox-Ordner auf das interne Dateisystem umkopiert. Darüber hinaus können auch Callbacks für bestimmte Dateien registriert werden.

## Integration
In der Regel haben unsere Geräte 2MB bzw. 16MB. Das erste Megabyte ist standardmäßig bereits für das System reserviert (+4096 Bytes am Flashende), wodurch der Speicher gerade bei 2MB sehr klein ist. Dem Modul liegt eine platform.exchange.ini als Vorlage bereit. 

Dazu muss in der eigenen platformio.ini die Vorlage per `extra_configs` eingebunden werden.

*Hier ein Beispiel*
```
extra_configs =
  lib/OGM-Common/platformio.base.ini
  lib/OGM-Common/platformio.rp2040.ini
  lib/OGM-Common/platformio.samd.ini
  lib/OGM-Common/platformio.esp32.ini
  lib/OFM-UsbExchange/platformio.exchange.ini
  platformio.custom.ini
```

Das Ganze klappt aber nur, wenn man die eigenen Einstellungen in eine eigene platformio.custom.ini ausgelagert hat, wodurch man dann auf die Parameter der `platformio.exchange.ini` entsprechend zugreifen kann. Alternativ muss der Inhalt in die eigene `platformio.ini` übernommen werden.

In der eigenen `platformio.custom.ini` bzw. `platformio.ini` müssen dann die Gerätesektionen passend zum Speicher erweitert werden.

*Hier ein Beispiel*
```
[env:develop_RP2040]
extends = RP2040_develop, RP2040_custom, custom_develop, RP2040_EXCHANGE_16MB
build_flags =
  ${RP2040_develop.build_flags}
  ${RP2040_custom.build_flags}
  ${custom_develop.build_flags}
  ${RP2040_EXCHANGE_16MB.build_flags}
  -D OKNXHW_REG1_CONTROLLER2040
```

Zum Schluss muss das Modul nich über die main.c eingebuden werden

```
#include "UsbExchangeModule.h"
...
void setup()
{
  ...
  openknx.addModule(8, openknxUsbExchangeModule);
  ...
}
```

Darüber hinaus können Callbacks für bestimme Dateien hinterlegt werden:

```
openknxUsbExchangeModule.onLoad("Dummy.txt", [](UsbExchangeFile* file) -> void {
    file->write("Demo");
});

openknxUsbExchangeModule.onEject("Dummy.txt", [](UsbExchangeFile* file) -> bool {
    // File is required
    if (file == nullptr)
    {
        logError("DummyModule", "File Dummy.txt was deleted but is mandatory");
        return false;
    }
    return true;
});
```

Beim onEject muss beachtet werden, dass der File-Pointer bei nichtvorhandener Datei ein `nullptr` ist. 
Sollte es beim Verarbeiten der Datei ein Problem geben, kann dies mitels `return bool` zurückgegeben werden.

### Parameter

Eine Anpassung ist bei Verwendung der Vorlage in der Regel nicht notwendig.

| Parameter             | Beschreibung                                                                                                          |
| --------------------- | --------------------------------------------------------------------------------------------------------------------- |
| EXCHANGE_FLASH_OFFSET | Wo im Flash sollen die Daten zwischen gespeichert werden                                                              |
| EXCHANGE_FLASH_SIZE   | Wie groß soll der Bereich werden (Limit wieviele Daten tatsächlich auf dem Laufwerk gepseicher werden könen)          |
| EXCHANGE_FS_SIZE      | Gibt die Größe des virtuelle Speichers an. Muss min. größer als 6MB sein. Dies ist unabhängig von EXCHANGE_FLASH_SIZE |

### Layouts

| Variante             | System | USB Exchange | Internes Dateisystem |
| -------------------- | -----: | -----------: | -------------------: |
| RP2040_EXCHANGE_2MB  |  1 MiB |      256 KiB |          ca. 768 KiB |
| RP2040_EXCHANGE_16MB |  1 MiB |        1 MiB |           ca. 14 MiB |

