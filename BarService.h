#include "ble/BLE.h"

class BarService{
public:
    BarService(BLEDevice &_ble, int br):
    ble(_ble),
    valueBytes(br),
    bar(0x1001, valueBytes.getPointer(),
         sizeof(float), sizeof(float), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY){
             GattCharacteristic *charTable[] = {&bar};
             GattService         barService(0xD000, charTable, 1);
             ble.addService(barService);
    }

    void updateBar(int br){
        valueBytes.updateBar(br);
        ble.gattServer().write(bar.getValueHandle(), valueBytes.getPointer(), sizeof(BarValueBytes));
    }

    struct BarValueBytes {
        static const unsigned MAX_VALUE_BYTES  = sizeof(int);

        BarValueBytes(int br) : valueBytes() {
            updateBar(br);
        }

        void updateBar(int br) {
            memcpy(&valueBytes, &br, sizeof(int));
        }

        uint8_t *getPointer(void) {
            return valueBytes;
        }

    private:
       uint8_t valueBytes[MAX_VALUE_BYTES];
    };

    BLEDevice &ble;
    BarValueBytes valueBytes;
    GattCharacteristic bar;
};
