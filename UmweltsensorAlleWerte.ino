#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <bsec2.h>  // BSEC2 Bibliothek für den BME680-Sensor
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>

#define NUMPIXELS 1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define BME680_I2C_ADDR 0x77



#define I2C_SDA 41
#define I2C_SCL 40

#define SEALEVELPRESSURE_HPA (1013.25)

// Erstelle eine Instanz des Displays
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
// Erstelle eine Instanz des BSEC Sensors
Bsec2 iaqSensor;

unsigned long lastUpdate = 0;  // Letztes Update der Anzeige
int currentPage = 0;           // Aktuelle Seite der Anzeige

// Zeit für den Seitenwechsel (in Millisekunden)
const unsigned long pageDelay = 5000;

// Variablen für Sensordaten
float temperature = 22.5;
float pressure = 1013.25;
float humidity = 45.0;
float co2 = 0.0;
float voc = 0.0;
float iaq = 0.0;

// 'logo', 30x30px                           //Einbinden des Hochschullogos
const unsigned char logo[] PROGMEM = {
  0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xfc,
  0xff, 0xff, 0xff, 0xfc, 0xf8, 0x3f, 0x80, 0x7c, 0xf8, 0x07, 0xc0, 0x7c, 0xf8, 0x01, 0xf0, 0x7c,
  0xf8, 0x00, 0x78, 0x7c, 0xf8, 0x00, 0x3c, 0x7c, 0xf8, 0x00, 0x1e, 0x7c, 0xf8, 0x00, 0x0e, 0x7c,
  0xfc, 0x00, 0x07, 0x7c, 0xff, 0xc0, 0x07, 0xfc, 0xff, 0xf0, 0x03, 0xfc, 0xf9, 0xf8, 0x01, 0xfc,
  0xf8, 0x7c, 0x01, 0xfc, 0xf8, 0x1e, 0x00, 0xfc, 0xf8, 0x0f, 0x00, 0xfc, 0xf8, 0x07, 0x00, 0xfc,
  0xf8, 0x07, 0x80, 0x7c, 0xf8, 0x03, 0x80, 0x7c, 0xf8, 0x03, 0x80, 0x7c, 0xf8, 0x03, 0x80, 0x7c,
  0xf8, 0x01, 0x80, 0x7c, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xfc,
  0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xfc
};

void setup() {
  Serial.begin(9600);
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialisiere das Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Stoppe hier bei Fehler
  }

  pixels.fill(0x000088); //blau
  pixels.show();
 
 #if defined(NEOPIXEL_POWER)
  // If this board has a power control pin, we must set it to output and high
  // in order to enable the NeoPixels. We put this in an #if defined so it can
  // be reused for other boards without compilation errors
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
 #endif

 pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(80); // half so bright


  display.display();
  delay(0);
  display.clearDisplay();

    display.setTextSize(2);                  //Textgröße
  display.setTextColor(WHITE);             //Textfarbe
  display.setCursor(50, 12);               //Cursor setzen auf Punkt 50 / 12
  display.println("HSNR");                 // HSNR Text schreiben
  display.drawBitmap(15, 0, logo, 30, 30, BLACK, WHITE);    //Logo zeichnen an Stelle (15,0) mit Pixelgröße 30x30 
  display.display();
  delay(2500);                              //Zeit die der Text /Logo zusehen ist
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 13);
  display.println("powered by...");
  display.display();
  delay(1500);
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("MakerSpace");
  display.display();
  delay(2500);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 13);
  display.println("Welcome to SmartCity!");
  display.display();
  delay(2500);
  display.clearDisplay();

  display.display();
  delay(100);
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Initialisiere den BME680 Sensor mit der I2C-Adresse 0x76
   iaqSensor.begin(BME680_I2C_ADDR, Wire);
  iaqSensor.attachCallback(newDataCallback);
  
  if (iaqSensor.status != BSEC_OK) {
    Serial.println("BSEC error code : " + String(iaqSensor.status));
    for (;;);
  }
  
  // Lade die BSEC-Konfiguration
  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);

}

void loop() {

  // BME680 Sensor-Daten aktualisieren
  if (iaqSensor.run()) {
    // Verarbeitet die Sensordaten
    } else {
    Serial.println("Failed to run BSEC");
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= pageDelay) {
    lastUpdate = currentMillis;
    currentPage = (currentPage + 1) % 6; // 6 Seiten insgesamt
    displayPage(currentPage);
  }
  delay(1000); // Warte 1 Sekunde vor dem nächsten Durchlauf
}

void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec) {
  if (!outputs.nOutputs) {
    return;
  }
  

  for (uint8_t i = 0; i < outputs.nOutputs; i++) {
    const bsecData output = outputs.output[i];
    switch (output.sensor_id) {
      case BSEC_OUTPUT_RAW_TEMPERATURE:
        temperature = output.signal;
        break;
      case BSEC_OUTPUT_RAW_PRESSURE:
        pressure = output.signal;
        break;
      case BSEC_OUTPUT_RAW_HUMIDITY:
        humidity = output.signal;
        break;
      case BSEC_OUTPUT_CO2_EQUIVALENT:
        co2 = output.signal;
        break;
      case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
        voc = output.signal;
        break;
      case BSEC_OUTPUT_IAQ:
        iaq = output.signal;
        break;
      default:
        break;
    }
  }
}


void displayPage(int page) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 16);

  switch (page) {
    case 0:
      display.print("Temp: ");
      display.print(temperature);
      display.println(" C");
        display.display();

      break;
    case 1:
      display.print("Pressure: ");
      display.print(pressure);
      display.println(" hPa");
        display.display();

      break;
    case 2:
      display.print("Humidity: ");
      display.print(humidity);
      display.println(" %");
        display.display();

      break;
    case 3:
      display.print("CO2: ");
      display.print(co2);
      display.println(" ppm");
        display.display();

      break;
    case 4:
      display.print("VOC: ");
      display.print(voc);
      display.println(" ppb");
        display.display();

      break;
    case 5:
      display.print("IAQ: ");
      display.print(iaq);
      display.println(" ");
        display.display();

      break;
  }

  display.display();
}