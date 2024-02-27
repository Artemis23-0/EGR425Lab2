///////////////////////////////////////////////////////////////
// TODO 1: Insert this class into your BLE server program
// and modify as needed
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// BLECharacteristic Callback Methods
///////////////////////////////////////////////////////////////
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    // Callback function to support a read request.
    void onRead(BLECharacteristic* pCharacteristic) {
        String characteristicUUID = pCharacteristic->getUUID().toString().c_str();
        String chraracteristicValue = pCharacteristic->getValue().c_str();
        Serial.printf("Client JUST read from %s: %s", characteristicUUID.c_str(), chraracteristicValue.c_str());
    }

    // Callback function to support a write request.
    void onWrite(BLECharacteristic* pCharacteristic) {
        // Get the characteristic enum and print for logging
        String characteristicUUID = pCharacteristic->getUUID().toString().c_str();
        String chraracteristicValue = pCharacteristic->getValue().c_str();
        Serial.printf("Client JUST wrote to %s: %s", characteristicUUID.c_str(), chraracteristicValue.c_str());

        // TODO: Take action by checking if the characteristicUUID matches a known UUID
		if (characteristicUUID.equalsIgnoreCase(CHARACTERISTIC_UUID)) {
			// The characteristicUUID that just got written to by a client matches the known
			// CHARACTERISTIC_UUID (assuming this constant is defined somewhere in our code)
		}
    }

    // Callback function to support a Notify request.
    void onNotify(BLECharacteristic* pCharacteristic) {
        String characteristicUUID = pCharacteristic->getUUID().toString().c_str();
        Serial.printf("Client JUST notified about change to %s: %s", characteristicUUID.c_str(), pCharacteristic->getValue().c_str());
    }

    // Callback function to support when a client subscribes to notifications/indications.
    void onSubscribe(BLECharacteristic* pCharacteristic, uint16_t subValue) {
    }

    // Callback function to support a Notify/Indicate Status report.
    void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) {
        // Print appropriate response
		String characteristicUUID = pCharacteristic->getUUID().toString().c_str();
        switch (s) {
            case SUCCESS_INDICATE:
                // Serial.printf("Status for %s: Successful Indication", characteristicUUID.c_str());
                break;
            case SUCCESS_NOTIFY:
                Serial.printf("Status for %s: Successful Notification", characteristicUUID.c_str());
                break;
            case ERROR_INDICATE_DISABLED:
                Serial.printf("Status for %s: Failure; Indication Disabled on Client", characteristicUUID.c_str());
                break;
            case ERROR_NOTIFY_DISABLED:
                Serial.printf("Status for %s: Failure; Notification Disabled on Client", characteristicUUID.c_str());
                break;
            case ERROR_GATT:
                Serial.printf("Status for %s: Failure; GATT Issue", characteristicUUID.c_str());
                break;
            case ERROR_NO_CLIENT:
                Serial.printf("Status for %s: Failure; No BLE Client", characteristicUUID.c_str());
                break;
            case ERROR_INDICATE_TIMEOUT:
                Serial.printf("Status for %s: Failure; Indication Timeout", characteristicUUID.c_str());
                break;
            case ERROR_INDICATE_FAILURE:
                Serial.printf("Status for %s: Failure; Indication Failure", characteristicUUID.c_str());
                break;
        }
    }
};


///////////////////////////////////////////////////////////////
// TODO 2: Insert a call to the setCallbacks method on any
// BleCharacteristic instances you make (in the BLE server
// program) to ensure they all execute the callback methods
// defined above
/////////////////////////////////////////////////////////////////////////
// Set callbacks (what happens when connecting, disconnecting, reading, writing, etc.)
bleCharacteristic->setCallbacks(new MyCharacteristicCallbacks());