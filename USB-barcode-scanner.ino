/*
 * Barcode Scanner Arduino
 * Original code by: RadhiFadhillah (github) https://github.com/felis/USB_Host_Shield_2.0/issues/323
 * Modified by: Dirga Brajamusti (dirgabrajamusti::github)
 * Modified by: Martin C. on 30/10/21
 */
#include <usbhid.h>
#include <usbhub.h>
#include <hiduniversal.h>
#include <hidboot.h>
#include <SPI.h>

#define DEBUG         // Décommenter si besoin d'afficher des infos au moniteur série

const uint8_t numBarcodes = 4;          /* Nombre de codes barres */
const uint8_t ledGreenPin = A2;         /* Pin de la led verte */
const uint8_t ledRedPin = A3;           /* Pin de la led rouge */
const uint8_t winPin = 3;              /* Pin de sortie a activer/desactiver si le puzzle est resolu*/

const String  correctIDs[] = {"xwcl02bxjl", "xwcl04bxjl", "xwcl10bxjl", "xwcl05bxjl"}; /* Suite d'identifiant de readers à realiser pour resoudre le puzzle */
String hasil;

class MyParser : public HIDReportParser {
  public:
    MyParser();
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
  protected:
    uint8_t KeyToAscii(bool upper, uint8_t mod, uint8_t key);
    virtual void OnKeyScanned(bool upper, uint8_t mod, uint8_t key);
    virtual void OnScanFinished();
};

MyParser::MyParser() {}

void MyParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  // If error or empty, return
  if (buf[2] == 1 || buf[2] == 0) return;

  for (uint8_t i = 7; i >= 2; i--) {
    // If empty, skip
    if (buf[i] == 0) continue;

    // If enter signal emitted, scan finished
    if (buf[i] == UHS_HID_BOOT_KEY_ENTER) {
      OnScanFinished();
    }

    // If not, continue normally
    else {
      // If bit position not in 2, it's uppercase words
      OnKeyScanned(i > 2, buf, buf[i]);
    }

    return;
  }
}

uint8_t MyParser::KeyToAscii(bool upper, uint8_t mod, uint8_t key) {
  // Letters
  if (VALUE_WITHIN(key, 0x04, 0x1d)) {
    if (upper) return (key - 4 + 'A');
    else return (key - 4 + 'a');
  }

  // Numbers
  else if (VALUE_WITHIN(key, 0x1e, 0x27)) {
    return ((key == UHS_HID_BOOT_KEY_ZERO) ? '0' : key - 0x1e + '1');
  }

  return 0;
}

void MyParser::OnKeyScanned(bool upper, uint8_t mod, uint8_t key) {
  uint8_t ascii = KeyToAscii(upper, mod, key);
  hasil = hasil + (char)ascii;
}

void MyParser::OnScanFinished() {
  static uint8_t l_u8Cnt = 0u;

#ifdef DEBUG
  Serial.println(hasil);
  Serial.println(l_u8Cnt);
#endif

  if (correctIDs[l_u8Cnt] == hasil){
    l_u8Cnt++;

    if (l_u8Cnt == numBarcodes) {

    #ifdef DEBUG
      Serial.println(F("Puzzle solved!"));
    #endif
      digitalWrite(winPin, HIGH);       // Turn laser on
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledRedPin, LOW);
      
      while(1){}  //exit program
    }
  }else{
    l_u8Cnt = 0u;

    digitalWrite(ledRedPin, HIGH);
    digitalWrite(ledGreenPin, LOW);
  }

  hasil = ""; // clear buffer
}

USB          Usb;
//USBHub       Hub(&Usb);
HIDUniversal Hid(&Usb);
MyParser     Parser;

void setup() {

  pinMode(winPin, OUTPUT);
  digitalWrite(winPin, LOW);
  pinMode(ledGreenPin, OUTPUT);
  digitalWrite(ledGreenPin, LOW);
  pinMode(ledRedPin, OUTPUT);
  digitalWrite(ledRedPin, HIGH);

#ifdef DEBUG  
  Serial.begin(115200);
  Serial.println("Start");
#endif
  
  if (Usb.Init() == -1) {
  #ifdef DEBUG  
    Serial.println("OSC did not start.");
  #endif
  }

  delay(200);

  Hid.SetReportParser(0, &Parser);
}

void loop() {
  Usb.Task();
}
