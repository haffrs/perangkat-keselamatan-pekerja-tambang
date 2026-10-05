/*
 * Perangkat Keselamatan Pekerja Tambang (Wearable)
 * Mikrokontroler : Arduino Mega 2560
 * Sensor         : MQ-2 (gas), DHT22 (suhu dan kelembapan), LDR (cahaya)
 * Aktuator       : LCD 20x4 I2C, LED merah/oranye/kuning/putih, buzzer
 * Simulator      : Wokwi
 * Penulis        : Mohamad Hafiz Sabar
 * Lisensi        : MIT
 *
 * Library: LiquidCrystal I2C, DHT sensor library, Adafruit Unified Sensor
 *
 * Catatan: prototipe edukasi. Bukan alat keselamatan bersertifikasi.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define NAMA  "[Nama]"
#define NIM   "[ID]"

// ===== UKURAN LCD =====
const int LCD_KOLOM = 20;
const int LCD_BARIS = 4;

// inisialisasi alamat I2C LCD (0x27) untuk wokwi
LiquidCrystal_I2C lcd(0x27, LCD_KOLOM, LCD_BARIS);

// ===== KONFIGURASI PIN (ubah di sini untuk maintenance) =====
const int pinGas    = A0;   // MQ-2 AOUT
const int pinLDR    = A1;   // LDR AO
const int pinDHT22  = 19;   // DHT22 SDA (data)
const int pinRed    = 27;   // LED merah   (gas BAHAYA)
const int pinOrange = 35;   // LED oranye  (gas WASPADA)
const int pinYellow = 43;   // LED kuning  (PANAS)
const int pinLampu  = 49;   // LED putih   (lampu)
const int pinBuzzer = 12;   // Buzzer
// LCD I2C di Mega: SDA = 20, SCL = 21 (pin tetap)

DHT dht(pinDHT22, DHT22);   // objek dht, tipe sensor DHT22

// ===== PARAMETER =====
const int GAS_WASPADA    = 400;     // gas >= ini -> WASPADA
const int GAS_BAHAYA     = 700;     // gas >= ini -> BAHAYA

const int CAHAYA_GELAP   = 300;     // cahaya < ini -> lampu penerang ON
const int CAHAYA_TERANG  = 700;     // cahaya > ini -> lampu penerang boleh OFF
const bool LDR_TERBALIK  = true;    // true jika nilai LDR makin BESAR saat makin GELAP

// Aturan PANAS (dua syarat), TANPA histeresis agar LED kuning langsung mengikuti kondisi:
//   PANAS jika  suhu >= SUHU_MUTLAK
//        ATAU   (suhu >= SUHU_LEMBAP DAN kelembapan >= RH_TINGGI)
const float SUHU_MUTLAK  = 37.0;    // suhu kering ekstrem, selalu PANAS
const float SUHU_LEMBAP  = 30.0;    // suhu minimal untuk syarat "panas + lembap"
const float RH_TINGGI    = 80.0;    // kelembapan minimal untuk syarat "panas + lembap"

// Buzzer saat PANAS: nyala-mati bergantian dengan interval panjang (ms)
const unsigned long PANAS_BUZZER_NYALA = 100UL;
const unsigned long PANAS_BUZZER_MATI  = 1500UL;
const int           PANAS_BUZZER_NADA  = 800;     // Hz

const unsigned long MIN_LAMPU_NYALA = 5000UL;  // lampu minimal menyala (ms)
const unsigned long INTERVAL_BACA   = 200UL;
const unsigned long INTERVAL_DHT    = 2000UL;  // batas DHT22: maksimal ~0,5 Hz (jangan diperkecil)
const unsigned long INTERVAL_LCD    = 300UL;

// ===== LEVEL GAS =====
enum LevelGas { G_AMAN, G_WASPADA, G_BAHAYA };
LevelGas levelGas = G_AMAN;

// ===== VARIABEL =====
int nilaiGas = 0;               // nilai ADC dari MQ-2
int nilaiCahaya = 0;
float suhu = 0.0;
float lembap = 0.0;
bool dhtOK = false;             // false jika pembacaan DHT22 gagal (NaN)
bool panas = false;             // langsung mengikuti aturan PANAS (tidak tampil di status LCD)
bool lampuGelap = false;        // hasil histeresis cahaya
int frekuensiAktif = 0;         // frekuensi buzzer yang sedang bunyi, 0 = mati
unsigned long waktuLampuNyala = 0;
unsigned long waktuBaca = 0;
unsigned long waktuDHT = 0;
unsigned long waktuLCD = 0;

// ---------- LCD ----------
void tulisBaris(uint8_t baris, String teks) {
  while (teks.length() < LCD_KOLOM) teks += ' ';
  lcd.setCursor(0, baris);
  lcd.print(teks.substring(0, LCD_KOLOM));
}

// Status hanya mengikuti gas. Panas ditandai oleh LED kuning dan beep, bukan teks.
String kondisiStatus() {
  if (levelGas == G_BAHAYA)  return "BAHAYA";
  if (levelGas == G_WASPADA) return "WASPADA";
  return "AMAN";
}

void tampilLCD() {
  // Baris 1: Status
  tulisBaris(0, "Status : " + kondisiStatus());

  // Baris 2: Gas
  tulisBaris(1, "Gas    : " + String(nilaiGas) + " ADC");

  // Baris 3 dan 4: Suhu dan Kelembapan
  // "\xDF" = simbol derajat bawaan LCD HD44780 (tanda kutip dipisah agar huruf C tidak ikut terbaca)
  if (dhtOK) {
    tulisBaris(2, "Suhu   : " + String(suhu, 1) + " \xDF" "C");
    tulisBaris(3, "Lembap : " + String(lembap, 0) + " %");
  } else {
    tulisBaris(2, "Suhu   : ERROR");
    tulisBaris(3, "Lembap : ERROR");
  }
}

// ---------- Sensor ----------
void bacaSensor() {
  nilaiGas = analogRead(pinGas);

  int mentah = analogRead(pinLDR);
  nilaiCahaya = LDR_TERBALIK ? (1023 - mentah) : mentah;

  // Histeresis + waktu minimal menyala
  if (nilaiCahaya < CAHAYA_GELAP && !lampuGelap) {
    lampuGelap = true;
    waktuLampuNyala = millis();
  } else if (nilaiCahaya > CAHAYA_TERANG && lampuGelap &&
             millis() - waktuLampuNyala > MIN_LAMPU_NYALA) {
    lampuGelap = false;
  }

  // Level gas
  if (nilaiGas >= GAS_BAHAYA)       levelGas = G_BAHAYA;
  else if (nilaiGas >= GAS_WASPADA) levelGas = G_WASPADA;
  else                              levelGas = G_AMAN;
}

void bacaDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    dhtOK = false;              // pembacaan gagal
    return;
  }

  dhtOK = true;
  suhu = t;
  lembap = h;

  // Tanpa histeresis: masuk kondisi panas -> langsung ON, keluar -> langsung OFF
  panas = (suhu >= SUHU_MUTLAK) ||
          (suhu >= SUHU_LEMBAP && lembap >= RH_TINGGI);

  // LED kuning diperbarui saat itu juga, tidak menunggu putaran berikutnya
  digitalWrite(pinYellow, panas ? HIGH : LOW);
}

// ---------- Aktuator ----------
// tone()/noTone() hanya dipanggil saat keadaan berubah, bukan di setiap putaran loop
void setBuzzer(int frekuensi) {
  if (frekuensi == frekuensiAktif) return;
  if (frekuensi == 0) noTone(pinBuzzer);
  else tone(pinBuzzer, frekuensi);
  frekuensiAktif = frekuensi;
}

void kendalikanAktuator() {
  // Tiga LED berdiri sendiri, jadi bisa menyala bersamaan tanpa bentrok
  digitalWrite(pinRed,    levelGas == G_BAHAYA  ? HIGH : LOW);
  digitalWrite(pinOrange, levelGas == G_WASPADA ? HIGH : LOW);
  digitalWrite(pinYellow, panas ? HIGH : LOW);

  // Lampu penerang
  digitalWrite(pinLampu, lampuGelap ? HIGH : LOW);

  // Buzzer: ritme ditentukan gas (lebih mendesak); jika gas aman, panas berkedip pelan
  if (levelGas == G_BAHAYA)       setBuzzer(millis() % 300 < 150 ? 2000 : 0);   // cepat dan keras
  else if (levelGas == G_WASPADA) setBuzzer(millis() % 1000 < 100 ? 1000 : 0);  // beep pelan
  else if (panas)                 setBuzzer(millis() % (PANAS_BUZZER_NYALA + PANAS_BUZZER_MATI)
                                            < PANAS_BUZZER_NYALA ? PANAS_BUZZER_NADA : 0);
  else                            setBuzzer(0);
}

void setup() {
  Serial.begin(9600);

  //konfigurasi input/output
  pinMode(pinRed, OUTPUT);
  pinMode(pinOrange, OUTPUT);
  pinMode(pinYellow, OUTPUT);
  pinMode(pinLampu, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);

  dht.begin();

  // inisialisasi lcd
  lcd.init();
  lcd.backlight();

  // Nama dan NIM di awal
  tulisBaris(0, "Nama:");
  tulisBaris(1, NAMA);
  tulisBaris(2, NIM);
  tone(pinBuzzer, 1000, 200);
  delay(3000);
  lcd.clear();
}

void loop() {
  if (millis() - waktuBaca >= INTERVAL_BACA) {
    waktuBaca = millis();
    bacaSensor();
  }

  if (millis() - waktuDHT >= INTERVAL_DHT) {
    waktuDHT = millis();
    bacaDHT();
  }

  kendalikanAktuator();

  if (millis() - waktuLCD >= INTERVAL_LCD) {
    waktuLCD = millis();
    tampilLCD();

    // Serial Monitor: data lengkap untuk kalibrasi ambang
    Serial.print("Gas(ADC): ");     Serial.print(nilaiGas);
    Serial.print(" | Cahaya: ");    Serial.print(nilaiCahaya);
    if (dhtOK) {
      Serial.print(" | Suhu: ");    Serial.print(suhu, 1);
      Serial.print(" C | RH: ");    Serial.print(lembap, 0);
      Serial.print("%");
    } else {
      Serial.print(" | DHT22: ERROR");
    }
    Serial.print(" | Panas: ");     Serial.print(panas ? "YA" : "TIDAK");
    Serial.print(" | Lampu: ");     Serial.print(lampuGelap ? "ON" : "OFF");
    Serial.print(" | Status: ");    Serial.println(kondisiStatus());
  }
}
