# Vapur Geçişi Senkronizasyon Problemi (Ferry Crossing) ⛴️🚗

Bu proje, İşletim Sistemleri dersi kapsamında C dili kullanılarak geliştirilmiş, klasik **Vapur Geçişi Senkronizasyon Problemi**'nin çoklu iş parçacığı (multi-threaded) çözümünü uygulamaktadır. Proje, eş zamanlı çalışan **Thread'ler**, **Semaforlar** ve **Mutex'ler** kullanarak bir kaynağın (vapur) sınırlı kapasiteyle güvenli bir şekilde yönetilmesini ve olayların doğru sıralanmasını (Barrier Synchronization) amaçlamaktadır.

## 📋 Proje Detayları ve Kısıtlamalar

* **Ders:** İşletim Sistemleri (Operating Systems)
* [cite_start]**Dil:** C (C99 Standardı) [cite: 51]
* [cite_start]**Kütüphaneler:** `pthread`, `semaphore`, `sys/time` ve diğer izin verilen kütüphaneler kullanılmıştır[cite: 52].
* [cite_start]**Süre Kısıtı:** Simülasyon, tam olarak **60 saniye** (1 dakika) sonra sonlanır[cite: 26].

***

## ⚙️ Senkronizasyon Mekanizması

Sistem, **Vapur** ve **Araba** thread'lerini koordine etmek için 4 ana Semafor ve 2 Mutex kullanır.

### Semaforların Görevleri

| Mekanizma | Tipi | Kontrol Ettiği Durum | İlişkili Kural |
| :--- | :--- | :--- | :--- |
| **`sem_board`** | Counting | Arabaların vapura binme izni. | [cite_start]Vapur sinyal verir, Araba bekler[cite: 10]. |
| **`sem_full`** | Binary | Vapurun dolduğu sinyali. | [cite_start]Son Araba sinyal verir, Vapur kalkış için bekler[cite: 11, 12]. |
| **`sem_unboard`** | Counting | Arabaların vapurdan inme izni. | [cite_start]Vapur sinyal verir, Araba bekler[cite: 13]. |
| **`sem_empty`** | Binary | Vapurun tamamen boşaldığı sinyali. | [cite_start]Son inen Araba sinyal verir, Vapur reset için bekler[cite: 14]. |

### Mutex Kullanımı
* [cite_start]**`mutex`:** `cars_on_board` sayacının değerini, birden fazla araba aynı anda değiştirirken oluşabilecek **yarış durumlarını (race condition)** önlemek için kullanılır[cite: 15].
* **`car_production_flag_mutex`:** Vapur seyahat halindeyken yeni araba üretimini durduran bayrağı korur.

***

## 🚗 Senaryo Akışı

1.  [cite_start]**Araba Varışı:** Arabalar rıhtıma 0 ile 1 saniye arasında rastgele (uniform) aralıklarla varır[cite: 20].
2.  [cite_start]**Biniş Kontrolü:** Vapur, kapasitesi (5) kadar biniş izni verir[cite: 16].
3.  [cite_start]**Kalkış Kontrolü:** 5 araba bindiğinde, son araba Vapur'un kalkması için sinyal verir[cite: 17].
4.  [cite_start]**Seyahat:** Vapur 3 saniye seyahat eder[cite: 22]. [cite_start]Bu süre zarfında yeni araba üretimi durur[cite: 23].
5.  [cite_start]**İniş Kontrolü:** Vapur varışta inme izni verir[cite: 18].
6.  [cite_start]**Reset:** Tüm arabalar indiğinde, son inen araba Vapur'un sıfırlanması için sinyal verir[cite: 19].
7.  **Döngü:** Yeni tur başlar ve program 60 saniye dolana kadar bu süreç devam eder.

***

## 🚀 Derleme ve Çalıştırma

Proje, standart GCC derleyicisi ve `Makefile` ile kolayca derlenir.

### Önkoşullar
* GCC Compiler (`-pthread` bayrağı ile)
* POSIX uyumlu sistem (Linux/Unix)

### Derleme (`Makefile` Kullanımı)
Proje dizininde `make` komutunu kullanın:

```bash
make
Çalıştırma
Oluşan ferry_cross dosyasını çalıştırın:

Bash

./ferry_cross
Temizleme
Derlenen dosyaları kaldırmak için:

Bash

make clean
📊 Örnek Çıktı Formatı
Program çıktısı, zaman damgalı ([Clock: %.4f]) olarak ödev formatına uygun şekilde görüntülenir.

Plaintext

[Clock: 0.4405] Car 0 entered the ferry [cite: 31]
[Clock: 2.4423] Car 4 entered the ferry [cite: 34]
[Clock: 2.4424] Ferry leaves the dock [cite: 36]
[Clock: 5.4430] Ferry arrives to new dock [cite: 37]
[Clock: 5.4433] Car 0 left the ferry [cite: 38]
[Clock: 5.4440] Car 2 left the ferry [cite: 40]