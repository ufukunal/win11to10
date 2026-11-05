# NoyaPLANET 11to10 — Microsoft Store gönderim adımları

Bu belge **kullanıcının imzalı sürümü Microsoft Store üzerinden alması**
için gereken adımları anlatır. GitHub'daki geliştirici EXE/MSIX dosyaları
imzasızdır ve Windows Smart App Control kapatılarak kurulmamalıdır.

## Kayıtlı Microsoft Store kimliği

- Uygulama: **NoyaPLANET 11to10**
- Paket adı: `NoyaPLANET.NoyaPLANET11to10`
- Yayıncı: `CN=B8F538D8-3B22-4726-BFFE-72791FFA184C`
- Görünen yayıncı: `NoyaPLANET`
- Aday sürüm: `0.7.0.0`

Bu değerler herkese açık paket tanımlayıcılarıdır; şifre veya özel imzalama
anahtarı değildir. Giriş şifrenizi veya sertifika özel anahtarınızı
GitHub deposuna veya sohbete göndermeyin.

## Yayıncının tamamlaması gereken adımlar

1. [Microsoft Partner Center](https://partner.microsoft.com/dashboard)
   hesabına yayıncı olarak giriş yapın. Hesap doğrulamasını tamamlayın.
2. **NoyaPLANET 11to10** uygulama kaydını açıp yukarıdaki paket adı ve yayıncıyı
   `packaging/store-identity.json` ile karşılaştırın.
3. Gerçek bir **Windows 11 x64** bilgisayarda veya güvenli test ortamında
   [manuel test planını](WINDOWS11_TEST_PLAN.md) uygulayın. Başlat, ek görev
   çubuğu, Windows Gezgini, onaylı ayarlar ve geri yükleme özellikle önemlidir.
   Windows güvenlik özelliklerini devre dışı bırakmayın.
4. Uygulama görselini onaylayıp Windows 11'de gerçekten alınmış Türkçe ekran
   görüntülerini hazırlayın. Yapay/temsili ekran görüntülerini gerçek ürün
   görüntüsü olarak göndermeyin.
5. Microsoft Store açıklamasını [taslak metinle](STORE_LISTING_TR.md)
   karşılaştırın. Uygulamanın **Windows'un orijinal görev çubuğunu/Başlat
   menüsünü değiştirdiğini iddia etmeyin**. Gizlilik metni ve destek
   bağlantılarının geçerliliğini kontrol edin:
   - Gizlilik: https://github.com/ufukunal/win11to10/blob/main/docs/PRIVACY.md
   - Destek/geri bildirim: https://github.com/ufukunal/win11to10/issues
6. Onaylanan kaynak kodunu main'e aldıktan sonra GitHub'da
   **Actions → Prepare unsigned MSIX for Partner Center (MANUAL ONLY) →
   Run workflow** seçeneğine girin. Açık onay alanına şu metni yazın:
   `STORE_SUBMISSION_ONLY_NOT_FOR_INSTALL`.
7. Tamamlanan çalışmanın **PARTNER_CENTER_UPLOAD_ONLY__UNSIGNED__DO_NOT_INSTALL**
   adlı çıktısını yalnızca Partner Center'a yüklemek için indirin. Paket
   imzasızdır; **bu dosyayı çift tıklayıp bilgisayara kurmayın**.
8. Partner Center'da fiyat/erişilebilirlik, gizlilik, açıklama, destek ve
   ekran görüntüsü alanlarını doldurup MSIX paketini gönderin. Microsoft'un
   sertifikasyon sonucunu bekleyin; onay garanti edilmez.
9. **Sertifikasyon ve yayın sonrasında** kullanıcılara yalnızca Microsoft
   Store'da yayımlanan Microsoft imzalı paketi gösterin. Smart App Control
   açıkken kurulum ve güncelleme testlerini gerçekleştirin.

## Durum ve sınırlar

GitHub otomasyonları C++ derlemesini, temel Windows 11 ARM64 doğrulamasını,
MSIX kimliğini, uygulamanın başlamasını ve imzasız dosyaların reddedildiğini
kontrol eder. Bu testler gerçek Windows 11 x64 masaüstü QA, Store başvurusu,
Microsoft sertifikasyonu veya tam Windows 10 22H2 kabuğu uyumluluğu anlamına
gelmez.

Microsoft Store, kabul edilen **MSIX/AppX** paketlerini yayın sırasında
Microsoft sertifikasıyla imzalar. Mağaza dışı doğrudan EXE/MSIX dağıtımı için
ayrı ve güvenilir kod imzalama gereklidir.

Referans: https://learn.microsoft.com/en-us/windows/apps/publish/get-started
