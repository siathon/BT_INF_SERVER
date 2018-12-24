#include "main.h"

int main() {
    scanner.start(scanPin);
    // pc.baud(9600);
    // scanner.attach(scanPin, 0.02);
    tft.begin();
    tft.setBitrate(500000000);
    tft.setRotation(2);
    LCDTextInit();
    lcdReady = 1;
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    ble.init(bleInitComplete);

    eventQueue.call_every(100, update);
    eventQueue.dispatch_forever();
    return 0;
}
