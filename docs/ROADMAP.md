# NoyaPLANET 11to10 — geliştirme yol haritası

Amaç: Windows 11 üzerinde hafif, geri alınabilir ve Windows 10 22H2'yi
anımsatan bir masaüstü deneyimi. "Tam kabuk değişimi" ile "bağımsız
önizleme/ek araç" birbirine karıştırılmamalı.

## Tamamlanan geliştirme kaynak kodu

- **v0.1:** C++20 performans CLI; kullanıcı görsel efektleri yedeği/geri alma;
  font smoothing korunur; otomatik Windows derleme testleri.
- **v0.2:** Türkçe native Win32 panel, Başlat ve görev çubuğu önizlemeleri.
- **v0.3:** Geri alınabilir HKCU Run yönetimi, kurulu klasik Başlat kısayolları,
  ilk açılış olay ölçümleri ve uygulamanın kendi bildirim simgesi.
- **v0.4:** Açılış geçmişi, başlatma envanteri, takvim ve dört kalıcı kutucuk.
- **v0.5:** Kullanıcı onaylı ilave masaüstü AppBar, açık pencere yönetimi,
  salt okunur Windows servis incelemesi.
- **v0.6:** Ayrı klasik dosya tarayıcısı, kayıt sonrası en az üç yeni açılışla
  karşılaştırma, Windows 11 ARM64 native CI ve GUI smoke.
- **v0.7:** AppsFolder içindeki modern Microsoft Store uygulamalarını Başlat
  menüsüne katma; açılış hedeflerini yeniden çözümleme; uygulama listesi
  önbelleği ve yenileme, klavye gezinmesi, Türkçe büyük/küçük harf araması,
  altı otomatik test ve genişletilmiş arayüz bileşeni smoke testi.

- **v0.7 geliştirme güncellemesi:** HKCU/HKLM StartupApproved Run kayıtlarının salt okunur durumları ve bilinmeyen kayıt türleri için güvenli sınıflandırma testleri. Bu durum, gerçek açılışta çalışmayı garanti etmez.

## Ürün tamamlandı denmeden önce

1. **Gerçek kabuk entegrasyonu:** Windows'un orijinal Başlat/görev
   çubuğu/tepsi/Explorer davranışlarının Windows 10'a benzer şekilde
   desteklenen Windows 11 sürümlerinde çalışması; geri alma ve hata kurtarma.
   Mevcut AppBar ve Başlat önizlemesi bunların yerine geçmez.
2. **Windows 11 x64 QA:** Farklı DPI, çoklu monitör, ekran okuyucu,
   Explorer yeniden başlatma, tam ekran uygulamaları, gerçek mağaza kurulumu.
3. **Performansın kanıtlanması:** Aynı cihaz, aynı başlangıç koşulları ve
   tekrarlı açılışlarda karşılaştırılabilir sonuçlar.
4. **Store yayımlaması:** Son marka varlıkları, gerçek ekran görüntüleri,
   gizlilik/destek alanları, hesap doğrulaması, Microsoft sertifikasyonu ve
   güvenilir MSIX imzası.

Açık GitHub işleri:
- https://github.com/ufukunal/win11to10/issues/2 (kabuk entegrasyonu)
- https://github.com/ufukunal/win11to10/issues/3 (performans)
- https://github.com/ufukunal/win11to10/issues/4 (Windows 11 QA ve Store)

## Değişmeyen güvenlik ilkeleri

Defender, Smart App Control ve Windows Update kapatılmaz; sistem ikili
dosyaları yamalanmaz; belirsiz Explorer API'lerine kod enjeksiyonu yapılmaz.
Otomatik servis kapatma, imzasız kurulumun "güvenilir" gösterilmesi veya
ölçülmemiş hız artışı iddiaları yoktur.
