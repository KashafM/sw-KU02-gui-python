import simplepyble

class Ble:
    def __init__(self):
        self.adapter = adapters = simplepyble.Adapter.get_adapters()
        self.adapter = adapters[0]
        self.peripheralDict = {}

    def getPeripherals(self):
        self.peripheralDict = {}
        self.adapter.set_callback_on_scan_start(lambda: {})
        self.adapter.set_callback_on_scan_stop(lambda: {})

        # Scan for 5 seconds
        self.adapter.scan_for(5000)
        peripherals = self.adapter.scan_get_results()

        for peripheral in peripherals:
            if "Itsy" in peripheral.identifier():
                self.peripheralDict[peripheral.identifier() + " " + str(peripheral.address())] = peripheral
        return self.peripheralDict

    def setServices(self, peripheral):
        services = peripheral.services()
        self.service_characteristic_pair = []
        for service in services:
            for characteristic in service.characteristics():
                self.service_characteristic_pair.append((service.uuid(), characteristic.uuid()))

    def connect(self, peripheral):
        peripheral.connect()

    def disconnect(self, peripheral):
        peripheral.disconnect()
        del peripheral
