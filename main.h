#include <events/mbed_events.h>
#include <mbed.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "Adafruit_GFX.h"
#include "TFT_ILI9163C.h"
#include "Org_01.h"
// #include "menu_BGR.h"
// #include "menu_RGB.h"
#include "menu128.h"

#include "CounterService.h"
#include "ParameterService.h"
#include "BarService.h"

#define __MOSI p19
#define __MISO p13
#define __SCLK p16
#define __CS   p14
#define __DC   p12
#define __RST  p6

static EventQueue eventQueue(16 * EVENTS_EVENT_SIZE);

TFT_ILI9163C tft(__MOSI, __MISO, __SCLK, __CS, __DC, __RST);

DigitalOut lcdEn(p21);

AnalogIn sensor(p1);
DigitalOut led(p20);
DigitalOut lcdReady(p22, 0);
DigitalOut sensorEn(p3);
Serial pc(USBTX, USBRX);

const static char     DEVICE_NAME[] = "1.5Inch_3";
static const uint16_t uuid16_list[] = {0xA000, 0xF000};

CounterService *counterService;
ParameterService *parameterService;
// BarService *barService;

int f1 = 0, f2 = 0, flow = 0, flowCC = 0, oldFlowCC = -1;
int scanCnt = 0, sampleCount = 10, sampleTime = 10, meanValue = 0, battValue = 100, oldBattValue = -1;
int pin = 0, ready = 0;
bool connected = false, setLowTh = false, setHighTh = false, setSampleTime = false, setSampleCount = false, setCounterParameter = false;
int counter = 0, oldCounter = -1, counterLitr = 999999999, oldCounterLitr = -1, sensorValue = 0, oldSensorValue = -1;
char str[10];
float counterParameter = 3.46;
int16_t  x1, y1;
uint16_t w, h;

int highTh = 190, lowTh = 110;

void resetParams(){
    counter = 0;
    counterLitr = 0;
    oldCounterLitr = -9999;
    flow = 0;
    flowCC = 0;
    oldFlowCC = -9999;
    scanCnt = 0;
}

// extern const unsigned short menu11[20480];

// Ticker scanner;
Thread scanner;

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params){
    (void) params;
    BLE::Instance().gap().startAdvertising();
    connected = false;
}

void connectionCallback(const Gap::ConnectionCallbackParams_t *params) {
    connected = true;
}

void onDataWrittenCallback(const GattWriteCallbackParams *params) {
    if ((params->handle == parameterService->getValueHandle()) && (params->len == 4)) {
        int temp = 0;
        for (size_t i = 0; i < 4; i++) {
            temp += (params->data[i] << (8*i));
        }
        if (temp == 9999) {
            NVIC_SystemReset();
        }

        if (temp == 1000) {
            resetParams();
            return;
        }

        if (temp == 1001) {
            setLowTh = true;
            return;
        }

        if (temp == 1002) {
            setHighTh = true;
            return;
        }

        if (temp == 1003) {
            setSampleCount = true;
            return;
        }

        if (temp == 1004) {
            setSampleTime = true;
            return;
        }

        if (temp == 1005) {
            setCounterParameter = true;
            return;
        }

        if (setLowTh) {
            lowTh = temp;
            setLowTh = false;
            return;
        }

        if (setHighTh) {
            highTh = temp;
            setHighTh = false;
            return;
        }

        if (setSampleCount) {
            sampleCount = temp;
            setSampleCount = false;
            return;
        }

        if (setSampleTime) {
            sampleTime = temp;
            setSampleTime = false;
            return;
        }
        if(setCounterParameter){
            counterParameter = (float)(temp / 100.0);
        }
    }
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params){
    BLE&        ble   = params->ble;
    // ble_error_t error = params->error;

    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);
    ble.gap().onConnection(connectionCallback);

    counterService = new CounterService(ble, counter);
    parameterService = new ParameterService(ble, counterParameter);
    // barService = new BarService(ble, 0);

    ble.gattServer().onDataWritten(onDataWrittenCallback);

    /* setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms. */
    ble.gap().startAdvertising();
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.call(Callback<void()>(&ble, &BLE::processEvents));
}

void LCDTextInit(){
     tft.drawRGBBitmap(0, 0, menu128, 128, 128);
     tft.setFont(&Org_01);
     tft.setTextSize(1);
     tft.setTextColor(WHITE);

     tft.setCursor(5, 98);
     sprintf(str, "B:");
     tft.print(str);

     tft.setCursor(60, 98);
     sprintf(str, "| IR:");
     tft.print(str);

     tft.setCursor(5, 90);
     sprintf(str, "P:");
     tft.print(str);
}

void showCounter(){
    tft.setTextSize(1);
    sprintf(str, "%d", oldCounter);
    tft.getTextBounds(str, 15, 90, &x1, &y1, &w, &h);
    tft.fillRect(x1, y1, w, h, BLACK);
    sprintf(str, "%d", counter);
    tft.setCursor(15, 90);
    tft.setTextColor(WHITE);
    tft.print(str);
}

void showFlow(){
    tft.setTextSize(2);
    sprintf(str, "%d", oldFlowCC);
    tft.getTextBounds(str, 40, 78, &x1, &y1, &w, &h);
    tft.fillRect(x1, y1, w, h, BLACK);
    sprintf(str, "%d", flowCC);
    tft.setCursor(40, 78);
    tft.setTextColor(YELLOW);
    tft.print(str);
}

void showBatt() {
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    sprintf(str, "%3d%%", oldBattValue);
    tft.getTextBounds(str, 15, 98, &x1, &y1, &w, &h);
    tft.fillRect(x1, y1, w, h, BLACK);
    sprintf(str, "%3d%%", battValue);
    tft.setCursor(15, 98);
    tft.print(str);
}

void showSensorValue(){
    tft.setTextSize(1);
    sprintf(str, "%3d", oldSensorValue);
    tft.getTextBounds(str, 90, 98, &x1, &y1, &w, &h);
    tft.fillRect(x1, y1, w, h, BLACK);
    sprintf(str, "%3d", sensorValue);
    tft.setCursor(90, 98);
    tft.setTextColor(WHITE);
    tft.print(str);
}

void showLitr(){
    tft.setTextSize(2);
    sprintf(str, "%d", oldCounterLitr);
    tft.getTextBounds(str, 10, 36, &x1, &y1, &w, &h);
    tft.fillRect(x1, y1, w, h, BLACK);
    sprintf(str, "%d", counterLitr);
    tft.setCursor(10, 36);
    tft.setTextColor(YELLOW);
    tft.print(str);
}

void scanPin(){
    while (1) {
        scanCnt++;
        sensorEn = 1;
        uint16_t sum = 0;
        for (size_t i = 0; i < sampleCount; i++) {
            sum += sensor.read_u16();
        }
        sensorEn = 0;
        meanValue = sum / sampleCount;

        // pc.printf("Mean Value = %d\n", meanValue);
        // if (connected) {
        //     barService->updateBar(meanValue);
        // }
        if (pin == 1){
            ready = 1;
        }
        if (meanValue < lowTh) {
            pin = 0;
        }
        else if(meanValue > highTh){
            pin = 1;
        }
        // pc.printf("%d\n", pin);
        if((!pin) && ready){
            counter++;
            ready = 0;
        }
        if (scanCnt == 1500) {
            f1 = f2;
            f2 = counter;
            flow = (f2 - f1) * 4;
            scanCnt = 0;
        }
        if (scanCnt % 50 == 0) {
            sensorValue = meanValue;
        }
        wait_ms(sampleTime);
    }
}

void update(){
    if(connected){
        lcdEn = 1;

        if(sensorValue != oldSensorValue){
            showSensorValue();
            oldSensorValue = sensorValue;
        }

        if (counter != oldCounter) {
            showCounter();
            oldCounter = counter;
        }

        counterLitr = (int)((counter / counterParameter) + 0.5);
        if(counterLitr != oldCounterLitr){
            showLitr();
            oldCounterLitr = counterLitr;
            // pc.printf("%d\n", counter);
            counterService->updateCounter(counterLitr);
        }

        flowCC = (int)((flow / counterParameter));
        if(flowCC != oldFlowCC && flowCC >= 0){
            showFlow();
            oldFlowCC = flowCC;
        }

        if (battValue != oldBattValue) {
            showBatt();
            oldBattValue = battValue;
        }
    }
    else{
        lcdEn = 0;
    }
}
