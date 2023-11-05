# OFM-UsbExchange

> **_Hinweis:_** Das Modul befindet sich nicht in der Entwicklungsphase und ist noch nicht fertig!

Dieses Modul stellt ein virtuelles Laufwerk bereit, ähnlich einem USB-Stick, über das Dateien auf den internen Speicher übertragen und auch (eingeschränkt) Dateien bereitgestellt werden können. Beachte jedoch, dass die Daten vorübergehend in einem separaten Flash-Bereich zwischengespeichert werden müssen und der verfügbare Speicher begrenzt ist.

Die Firmware kann vor dem Einbinden zusätzliche Dateien auf den Speicher bereitstellen. Sobald der Benutzer seine eigenen Daten auf den Speicher kopiert hat, kann er das Laufwerk auswerfen. Anschließend werden die neuen Dateien auf den internen Speicher rüber kopiert.

## Verwendung

Soabld der Benutzer die Prog-Taste lange gedrückt hällt, wird das Laufwerk vorbereitet (Dateisystem erstellt + Standarddateien kopiert) und als Laufwerk bereit gestellt. Durch dass erneute lange drücken der Prog-Taste,

## Integration
In der Regel haben unsere Geräte 2MB bzw. 16MB. Der erste Megabyte ist standardmäßig bereits für das System reserviert (+4096 Bytes am Flashende), wodurch der Speicher gerade bei 2MB sehr klein ist. Dem Modul liegt eine platform.exchange.ini als Vorlage bereit. 

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

In der eigenen `platformio.custom.ini` bzw `platformio.ini` müssen dann die Gerätesektionen passend zum Speicher erweitert werden.

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


### Parameter

Eine Anpassung ist bei Verwendung der Vorlage in der Regel nicht notwendig.

| Parameter             | Beschreibung                                                                                                          |
| --------------------- | --------------------------------------------------------------------------------------------------------------------- |
| EXCHANGE_FLASH_OFFSET | Wo im Flash sollen die Daten zwischen gespeichert werden                                                              |
| EXCHANGE_FLASH_SIZE   | Wie groß soll der Bereich werden (Limit wieviele Daten tatsächlich auf dem Laufwerk gepseicher werden könen)          |
| EXCHANGE_FS_SIZE      | Gibt die Größe des virtuelle Speichers an. Muss min. größer als 6MB sein. Dies ist unabhängig von EXCHANGE_FLASH_SIZE |

### Layouts

| Variante             | System | USB Exchnage | Internes Datisystem |
| -------------------- | -----: | -----------: | ------------------: |
| RP2040_EXCHANGE_2MB  |    1MB |        256KB |          ca. 768 KB |
| RP2040_EXCHANGE_16MB |    1MB |          1MB |            ca. 14MB |

