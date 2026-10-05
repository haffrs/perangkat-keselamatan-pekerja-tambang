# Perangkat Keselamatan Pekerja Tambang (Wearable)

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Board](https://img.shields.io/badge/Board-Arduino%20Mega%202560-00979D?logo=arduino&logoColor=white)
![Simulator](https://img.shields.io/badge/Simulator-Wokwi-blue)

Prototipe perangkat keselamatan yang dikenakan pekerja tambang, berbasis **Arduino Mega 2560** dan disimulasikan di **Wokwi**. Perangkat memantau gas, suhu, kelembapan, dan cahaya di sekitar pekerja, lalu memberi peringatan lewat LCD, LED, dan buzzer, serta menyalakan lampu penerang secara otomatis saat gelap.

> **English summary:** An educational IoT prototype of a wearable safety unit for mine workers. An Arduino Mega 2560 reads a gas sensor (MQ-2), a temperature/humidity sensor (DHT22) and a light sensor (LDR), and drives a 20x4 I2C LCD, status LEDs, a buzzer and an automatic work light. Simulated in Wokwi.

> **Peringatan:** proyek ini dibuat untuk **tujuan pembelajaran** (tugas mata kuliah Internet of Things). Perangkat ini bukan alat keselamatan bersertifikasi dan **tidak boleh dijadikan satu-satunya pelindung** di lingkungan kerja sungguhan.

[Buka simulasi di Wokwi](https://wokwi.com/projects/476661247482973185)

## Fitur

- Deteksi gas dengan tiga level: **AMAN**, **WASPADA**, **BAHAYA**.
- Peringatan **PANAS** berdasarkan gabungan suhu dan kelembapan (LED kuning dan buzzer jarang).
- **Lampu penerang otomatis** dari sensor cahaya, memakai histeresis dan waktu minimal menyala agar tidak berkedip.
- Layar LCD 20x4 menampilkan nama dan NIM saat penyalaan, lalu Status, Gas, Suhu, dan Kelembapan.
- Pewaktuan tanpa `delay()` (memakai `millis()`), jadi sensor, LCD, dan buzzer berjalan bersamaan.
- Seluruh pin dikumpulkan sebagai konstanta di awal program sehingga mudah diubah.

## Komponen

| Komponen | Jumlah | Fungsi |
|---|---|---|
| Arduino Mega 2560 | 1 | Mikrokontroler |
| Sensor gas MQ-2 | 1 | Sensor 1: kadar gas (nilai ADC) |
| Sensor DHT22 | 1 | Sensor 2: suhu dan kelembapan |
| Modul LDR (photoresistor) | 1 | Sensor 3: intensitas cahaya |
| LCD 20x4 I2C | 1 | Tampilan nama, NIM, dan data |
| LED merah, oranye, kuning, putih | 1 tiap warna | Indikator dan lampu penerang |
| Buzzer | 1 | Alarm suara |
| Resistor 220 Ω | 4 | Pembatas arus LED |
| Breadboard dan kabel jumper | 1 set | Distribusi 5 V dan GND |

## Pin

| Komponen | Pin Mega | Jenis sinyal |
|---|---|---|
| MQ-2 (AOUT) | A0 | Analog |
| LDR (AO) | A1 | Analog |
| DHT22 (data) | 19 | Digital (satu kabel) |
| LED merah (gas BAHAYA) | 27 | Digital |
| LED oranye (gas WASPADA) | 35 | Digital |
| LED kuning (PANAS) | 43 | Digital |
| LED putih (lampu penerang) | 49 | Digital |
| Buzzer | 12 | Digital |
| LCD SDA / SCL | 20 / 21 | I2C (pin tetap) |

## Logika

| Kondisi | Aturan | Keluaran |
|---|---|---|
| Gas AMAN | ADC MQ-2 < 400 | LED gas mati |
| Gas WASPADA | 400 ≤ ADC < 700 | LED oranye, beep pelan |
| Gas BAHAYA | ADC ≥ 700 | LED merah, buzzer cepat |
| PANAS | suhu ≥ 37 °C **atau** (suhu ≥ 30 °C **dan** kelembapan ≥ 80 %) | LED kuning, buzzer beep jarang |
| GELAP | cahaya < 300 (lampu boleh padam saat > 700 dan sudah menyala ≥ 5 detik) | LED putih menyala |

Catatan perilaku:

- Status di LCD hanya mengikuti gas. Kondisi PANAS ditandai oleh LED kuning dan buzzer.
- Jika gas dan panas terjadi bersamaan, kedua LED menyala dan buzzer mengikuti ritme gas.
- Kondisi PANAS tanpa histeresis supaya LED langsung bereaksi. Kecepatannya dibatasi pembacaan DHT22 (sekitar 2 detik).
- Semua ambang adalah **nilai contoh**. Sesuaikan dengan hasil pengamatan dan regulasi keselamatan yang berlaku.

Contoh tampilan LCD:

```
Status : AMAN
Gas    : 120 ADC
Suhu   : 28.0 °C
Lembap : 65 %
```

## Cara menjalankan di Wokwi

1. Buka [wokwi.com](https://wokwi.com), buat proyek baru dengan board **Arduino Mega**.
2. Tempel isi `firmware/perangkat_keselamatan_tambang/perangkat_keselamatan_tambang.ino` ke tab `sketch.ino`.
3. Tempel isi `wokwi/diagram.json` ke tab `diagram.json` (lihat [wokwi/README.md](wokwi/README.md)).
4. Tempel isi `wokwi/libraries.txt` ke tab `libraries.txt`.
5. Klik **Start Simulation**.

Untuk menguji, klik komponen saat simulasi berjalan:

- **DHT22:** atur suhu dan kelembapan.
- **MQ-2:** atur konsentrasi gas, lalu baca nilai `Gas(ADC)` di Serial Monitor untuk menyesuaikan ambang.
- **LDR:** atur tingkat cahaya.

Jika lampu menyala saat terang (atau sebaliknya), ubah `LDR_TERBALIK` di bagian parameter.

## Cara menjalankan di Arduino IDE (opsional)

1. Buka folder `firmware/perangkat_keselamatan_tambang/` di Arduino IDE.
2. Pasang library lewat Library Manager: **LiquidCrystal I2C**, **DHT sensor library**, **Adafruit Unified Sensor**.
3. Pilih board **Arduino Mega or Mega 2560**, lalu unggah.

Kode ini ditulis dan diuji untuk simulasi di Wokwi. Pada perangkat keras fisik, alamat I2C LCD dan perilaku sensor mungkin berbeda dan perlu diperiksa.

## Mengubah posisi pin

Semua pin ada di bagian `KONFIGURASI PIN` pada awal program. Contoh memindah buzzer dari pin 12 ke pin 8:

```cpp
const int pinBuzzer = 8;   // sebelumnya 12
```

Lalu pindahkan kabel sinyal buzzer di rangkaian. Batasan: sensor analog harus tetap di A0 sampai A15, jalur I2C LCD tetap di pin 20 dan 21, dan hindari pin 0 dan 1 (serial).

## Struktur repositori

```
.
├── firmware/
│   └── perangkat_keselamatan_tambang/
│       └── perangkat_keselamatan_tambang.ino   # kode program
├── wokwi/
│   ├── diagram.json                            # rangkaian Wokwi (salin dari editor)
│   ├── libraries.txt                           # library Wokwi
│   └── README.md
├── docs/
│   ├── Laporan_Praktikum_IoT_Perangkat_Keselamatan_Pekerja_Tambang.docx
│   └── images/                                 # tangkapan layar
├── LICENSE
└── README.md
```

## Tangkapan layar

| Rangkaian | Kondisi AMAN |
|---|---|
| ![Rangkaian](/images/rangkaian.png) | ![Aman](/images/kondisi-aman.png) |

| Gas BAHAYA | PANAS |
|---|---|
| ![Bahaya](/images/kondisi-bahaya.png) | ![Panas](/images/kondisi-panas.png) |

## Keterbatasan

- Disimulasikan di Wokwi; belum diuji pada perangkat keras fisik.
- Gas ditampilkan sebagai **nilai ADC** (indeks relatif), bukan ppm terkalibrasi. MQ-2 tidak membedakan jenis gas.
- Perangkat **tidak mengukur oksigen**.
- Arduino Mega terlalu besar untuk dikenakan sungguhan, sehingga ini berstatus prototipe.
- Peringatan hanya bersifat lokal, belum dikirim ke pos kendali.

## Rencana pengembangan

- [ ] Sensor oksigen elektrokimia dan sensor metana khusus.
- [ ] Kalibrasi MQ-2 untuk konsentrasi dalam ppm.
- [ ] Deteksi pekerja terjatuh dengan MPU6050.
- [ ] Pengiriman peringatan ke pos kendali (misalnya LoRa).
- [ ] Papan yang lebih kecil dan catu daya baterai.

## Dokumentasi

Laporan praktikum lengkap tersedia di folder [`docs/`](docs/).

## Lisensi

Dirilis di bawah lisensi [MIT](LICENSE).

## Penulis

**Mohamad Hafiz Sabar**, S1 Teknik Informatika, mata kuliah Internet of Things.
