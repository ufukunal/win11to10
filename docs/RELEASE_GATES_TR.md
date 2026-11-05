# Yayımlama kapıları — kaynak kodu ile ürün arasındaki fark

## Otomatik doğrulanabilenler

- [x] Windows x64 MSVC/CMake derlemesi, CTest, EXE meta verileri.
- [x] Windows 11 ARM64 üzerinde yerel C++ derlemesi ve temel GUI oluşturma/kapatma.
- [x] Windows AppsFolder'dan modern uygulama tarama/hedef çözümleme testi.
- [x] Başlat, bağımsız dosya tarayıcısı, takvim ve görev çubuğu kontrollerinin
      açılıp kapanmasını sınayan zararsız GUI testi.
- [x] Geri alınabilir HKCU başlangıç kayıtları testi (geçici CI hesabı).
- [x] Microsoft Store paket adı/yayıncı kimliği, EXE/MSIX içerik doğrulaması.
- [x] İmzasız EXE ve MSIX'in güvenilir sürüm olarak sunulmasının engellenmesi.

## Ürün tamamlandı demek için gerekenler

- [ ] Gerçek Windows 11 **x64** masaüstünde çoklu monitör, DPI, erişilebilirlik,
      Explorer yeniden başlatma ve tam ekran uygulamaları test edildi.
- [ ] Windows 11'in **gerçek** Başlat menüsü, görev çubuğu, üçüncü taraf sistem
      tepsisi ve Explorer için istenen Windows 10 22H2 davranışları elde edildi
      veya işletim sistemi sürümündeki desteklenmeyen farklar açıkça anlatıldı.
      Mevcut AppBar ve pencere önizlemeleri bunların yerini tutmaz.
- [ ] Açılış hızlanması aynı bilgisayarda yeterli sayıda önce/sonra ölçümle
      doğrulandı. Yalnızca olay günlüğünden medyan okumak hız kazancı kanıtı değildir.
- [ ] Final uygulama görseli, gerçek ürün ekran görüntüleri, yayımlanmış gizlilik
      ilkesi ve yayıncı iletişim/destek yolu onaylandı.
- [ ] Partner Center başvurusu, Microsoft Store sertifikasyonu ve paket imzası
      başarıyla tamamlandı.
- [ ] Smart App Control açıkken Store kurulum/güncelleme/kaldırma testi geçti.

## Güvenlik kuralları

Güvenilir imza elde etmek için Windows Defender, Smart App Control veya
Windows Update kapatılmaz. İmzasız dosyalar kullanıcılara kurulum sürümü
gibi sunulmaz. Windows Explorer dosyaları yamalanmaz, bilinmeyen kabuk
API'lerine kod enjeksiyonu yapılmaz. Yayım veya tam uyumluluk onayı verilmeden
main dalındaki kaynak kodu yalnızca geliştirme sürümü kabul edilir.
