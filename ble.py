# imports
import simplepyble
import logging
import asyncio

# logging
logging.basicConfig(filename='Logs/application.log', format='%(asctime)s %(message)s : %(funcName)s',
                    datefmt='%m/%d/%Y %I:%M:%S %p', encoding='utf-8', level=logging.INFO)

# Press the green button in the gutter to run the script.
class Ble:
    def __init__(self):
        self.adapter = adapters = simplepyble.Adapter.get_adapters()
        self.adapter = adapters[0]
        self.peripheralDict = {}

    def getPeripherals(self):
        self.adapter.set_callback_on_scan_start(lambda: {})
        self.adapter.set_callback_on_scan_stop(lambda: {})

        # Scan for 5 seconds
        self.adapter.scan_for(5000)
        peripherals = self.adapter.scan_get_results()

        for peripheral in peripherals:
            if "Itsy" in peripheral.identifier():
                self.peripheralDict[peripheral.identifier() + " " + str(peripheral.address())] = peripheral
        return self.peripheralDict

    def setPeripheral(self, peripheral):
        self.peripheral = peripheral

    def setServices(self, peripheral):
        services = peripheral.services()
        self.service_characteristic_pair = []
        for service in services:
            for characteristic in service.characteristics():
                self.service_characteristic_pair.append((service.uuid(), characteristic.uuid()))

    def recieveTransmission(self, peripheral):
        service_uuid, characteristic_uuid = self.service_characteristic_pair[0]
        peripheral.notify(service_uuid, characteristic_uuid, lambda data: print(f"Notification: {data}"))

    def connect(self, peripheral):
        peripheral.connect()
        print("Successfully connected to peripheral")

    def disconnect(self, peripheral):
        peripheral.disconnect()
        print("Successfully disconnected to peripheral")