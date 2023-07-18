#include <Arduino.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include "HX711.h"

const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;
const int ESC_PIN = 15;

float calibration_factor = 0;
HX711 loadcell;
Servo esc;
BluetoothSerial SerialBT;
const String tare = "tare";
const String calibrarBalanca = "calibrarBalanca";
String throttleInput;
int throttleValue = 0;
void setup()
{
    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    loadcell.set_scale();
    loadcell.tare();

    SerialBT.begin("BANCADA-EDRA");
    Serial.begin(115200);
    delay(1000);
    esc.attach(ESC_PIN, 1000, 2000);
    Serial.print("Calibrando ESC...");
    esc.writeMicroseconds(2000);
    delay(1000);
    esc.writeMicroseconds(1000);
    delay(1000);
    Serial.print("ESC calibrado com sucesso!");
}

String readSerialInput()
{
    if (Serial.available())
    {
        return Serial.readStringUntil('\n');
    }
    else if (SerialBT.available())
    {
        return SerialBT.readStringUntil('\n');
    }

    return "";
}

String waitForSerialInput()
{
    String input = "";

    static unsigned long timeout = 0;
    const unsigned long inputTimeout = 1000; // Adjust timeout duration as needed

    // Check for available data on Serial
    if (Serial.available())
    {
        input = Serial.readStringUntil('\n');
        timeout = millis() + inputTimeout;
    }

    // Check for available data on SerialBT
    if (SerialBT.available())
    {
        input = SerialBT.readStringUntil('\n');
        timeout = millis() + inputTimeout;
    }

    // Check if the timeout period has elapsed
    if (timeout > 0 && millis() > timeout)
    {
        timeout = 0; // Reset the timeout value
    }

    return input;
}

void middleware()
{
    if (throttleInput == tare)
    {
        Serial.println("Tarando a balanca...");
        loadcell.set_scale();
        loadcell.tare();
        Serial.println("Balanca tarada com sucesso.");
        throttleInput = "";
    }
    if (throttleInput == calibrarBalanca)
    {
        Serial.println("Insira o fator de calibracao: ");
        String input = waitForSerialInput();
        while (input.length() == 0)
        {
            input = waitForSerialInput();
        }
        calibration_factor = input.toFloat();
        Serial.println("Fator de calibracao atualizado para: ");
        Serial.println(calibration_factor);
        throttleInput = "";
    }
}

void printInfos()
{

    StaticJsonDocument<128> jsonDocument;

    float weight = loadcell.get_units(5);

    jsonDocument["PWM"] = throttleValue;
    jsonDocument["weight"] = weight;

    String jsonString;
    serializeJson(jsonDocument, jsonString);

    Serial.println(jsonString);
}

void loop()
{
    loadcell.set_scale(calibration_factor);

    printInfos();
    middleware();

    int throttlePercentage = throttleInput.toInt();
    throttlePercentage = constrain(throttlePercentage, 0, 100);

    throttleValue = map(throttlePercentage, 0, 100, 1000, 2000);

    esc.writeMicroseconds(throttleValue);

    String input = readSerialInput();

    if (input != "")
    {
        throttleInput = input;
    }
}