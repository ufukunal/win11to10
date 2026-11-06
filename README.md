# NoyaPLANET 11to10

**Windows 11 için yerel C++/Win32 performans aracı ve Windows 10 tarzı
masaüstü bileşenleri.**

- **Sürüm:** v0.7 geliştirme önizlemesi (0.7.0.0)
- **Microsoft Store uygulama adı:** NoyaPLANET 11to10
- **Yayıncı:** NoyaPLANET
- **Güvenilir kurulum:** Henüz **yok**. GitHub imzasız EXE/MSIX'i kullanıcılara
  kurulabilir ürün olarak yayımlamaz.

Bu kaynak kodu Windows 11'i Windows 10 işletim sistemine dönüştürmez.
Gerçek Windows tuşu/Başlat, Windows görev çubuğu, üçüncü taraf bildirim
simgeleri ve sistem Dosya Gezgini Windows tarafından yönetilmeye devam eder.

## Uygulamanın şu an çalışan özellikleri

- **Türkçe kontrol paneli:** Windows derlemesi, RAM ve sistem durumu.
- **Geri alınabilir görsel efekt profili:** Kullanıcının onayıyla belirli
  animasyon, gölge ve geçiş efektlerini azaltır; yazı tipi kenar düzeltmesini
  açık tutar. İlk kullanıcı ayarları saklanır, geri yükleme vardır.
- **Windows 10 tarzı NoyaPLANET Başlat açılır menüsü:** Masaüstünde ayrı,
  çerçevesiz bir açılır pencere olarak çalışır. Orijinal Windows Başlat'ın
  yerini almaz; masaüstünün alt sol köşesine veya ek AppBar'ın üstüne açılır,
  pencere dışına tıklayınca, Esc veya Kapat ile kapanır. Windows uygulamaları,
  klasik Başlat kısayolları ve Microsoft Store'dan yüklenen uygulamalar için
  Windows AppsFolder taraması. Uygulama yalnızca tıklama/Enter ile açılır. Dört
  kalıcı kutucuk, sağ tık menüsü, kullanıcı kontrollü liste yenileme.
  Türkçe büyük-küçük harf araması, Enter, aşağı ok ve Escape desteği.
  Ctrl+Alt+S kısayolu program açıkken kullanılabiliyorsa NoyaPLANET
  menüsünü açıp kapatır.
  Arama dizini yeniden tarama yapılana kadar bellekte önbelleğe alınır.
- **İsteğe bağlı ek görev çubuğu:** Windows Shell AppBar API ile ayrı bir
  ekran alanı ayırır. NoyaPLANET Başlat menüsü, açık pencere düğmeleri, saat
  ve yerel takvime erişim gösterilir. Tümü menüsünden 30'lu sayfalara ayrılan
  açık pencereler, Başlat'taki aynı dört sabitleme ve Windows ses/ağ/güç
  ayarlarına geçişler sunulur. Kullanıcı onayıyla açılır ve kaldırıldığında
  ekran alanını serbest bırakır.
  **Orijinal Windows görev çubuğu gizlenmez veya değiştirilmez.**
  Görünüm menüsündeki kurtarma komutu tüm NoyaPLANET önizlemelerini ve
  ek çubuğu kapatır. Performans ayarlarının geri alınması ayrı onay gerektirir.
- **Sistem tepsisi ve takvim:** NoyaPLANET'in yalnızca kendi simgesi,
  ayrı klasik takvim ve pencere önizlemesi. Diğer uygulamaların tepsi
  simgeleri ele geçirilmez.
- **Klasik dosya tarayıcısı:** Ayrı Win32 penceresinde klasör/ileri/geri/üst
  gezinme, yerel arama, dosya özellikleri, seçilen dosyayı Windows ile açma
  ve dosya yolunu kopyalama. Sistem Explorer'ı değiştirilmez.
- **Başlangıç yönetimi:** Mevcut kullanıcı için HKCU Run girdilerini yalnızca
  onayla ve özgün kayıt değerini saklayarak devre dışı bırakma/geri yükleme.
  Diğer başlangıç kaynakları ve Windows servisleri salt okunur incelenir.
- **İsteğe bağlı oturum açılışında başlatma:** Sistem araçları menüsündeki
  seçenek, yalnızca kullanıcının açık onayıyla NoyaPLANET kontrol paneli
  için HKCU Run kaydı ekler. Aynı adlı başka bir kayıt varsa üzerine yazmaz;
  kaldırırken yalnızca kendi çalıştırılabilir dosya yoluyla eşleşen kaydı
  siler. Windows 10 görünümünü oturum açılışında otomatik etkinleştirmez.
  Uygulamanın konumu değişirse eski kayıt Windows Başlangıç ayarlarından
  kaldırılmalıdır.
- **Açılış ölçümleri:** Windows Diagnostics-Performance Event 100 geçmişi ve
  onaylı bir başlangıç medyanıyla en az üç yeni açılış arasında karşılaştırma.
  Ölçüm sonucu, programın hız kazandırdığının tek başına kanıtı değildir.

Hiçbir sürekli tarama servisi veya Explorer kod enjeksiyonu çalışmaz.
Defender, Smart App Control ve Windows Update kapatılmaz.

## Geliştirme ve test

GitHub Actions, programı Windows Server 2022 **x64** ve gerçek Windows 11
Enterprise **ARM64** ortamında MSVC/CMake ile derler. Otomatik testler C++,
modern AppsFolder hedef çözümlemesi, geçici HKCU kayıtlarının geri yüklenmesi,
servis envanteri, Win32 arayüz pencereleri ve AppBar kaydı uygun bir
masaüstü kabuğunda mümkünse AppBar açma/takvim/kapatma yaşam döngüsünü kapsar.
AppBar kaydı bulunmayan CI oturumunda yalnızca güvenli başarısızlık yolu
doğrulanır; bu test gerçek Windows masaüstü davranışını kanıtlamaz.

Bunlar **Windows 11 x64 masaüstü testi**, çoklu monitör/erişilebilirlik testi,
performans kazanımı kanıtı ya da Microsoft Store onayı değildir.

Geliştiriciler Windows üzerinde:

~~~powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
~~~

İki EXE derlenir:
- NoyaPLANET11to10.exe — grafik arayüz.
- Win11To10.exe — kullanıcı onaylı performans ayarı yardımcı aracı.

Yardımcı aracın komutları:

~~~powershell
.\Win11To10.exe analyze
.\Win11To10.exe status
.\Win11To10.exe apply-performance --confirm
.\Win11To10.exe restore --confirm
~~~

Grafik arayüzde performans profilini uygulamak/geri almak için ayrı onay
alınır. İlk özgün ayarlar şuradadır:

~~~text
%LOCALAPPDATA%\Win11To10\baseline-v1.txt
~~~

## Windows 10 görünümünün sınırları

Güvenli ve desteklenen Win32 API'leriyle **ek bir AppBar ve bağımsız pencereler**
oluşturuluyor. Windows 11'in gerçek Windows tuşu, görev çubuğu, bildirim
alanı veya Explorer şeridi henüz Windows 10 22H2 bileşenlerine dönüşmüyor.
Sürüm bazında desteklenmeyen kabuk işlemlerini değiştirmek için sistem
dosyalarını yamalamıyoruz.

Ayrıntılar: [Kabuk uyumluluğu](docs/SHELL_COMPATIBILITY.md).

## Microsoft Store: NoyaPLANET 11to10

Partner Center paket kimliği depoda sabittir:
- Package Identity Name: NoyaPLANET.NoyaPLANET11to10
- Publisher: CN=B8F538D8-3B22-4726-BFFE-72791FFA184C
- Version: 0.7.0.0

GitHub'daki **Prepare unsigned MSIX for Partner Center (MANUAL ONLY)**
iş akışı bir **imzasız, yalnızca başvuru için** MSIX oluşturur.
Bu dosya Smart App Control bulunan bir bilgisayara yüklenmemelidir.
Microsoft Store, sertifikasyondan geçen MSIX'i yayımlarken kendi
sertifikasıyla imzalar. Microsoft Store sertifikasyonu henüz yapılmadı.

Visual Studio kurmadan başvuru ve imza süreci için
[Türkçe mağaza adımları](docs/STORE_SUBMISSION_STEPS_TR.md) ve
[güvenilir dağıtım](docs/TRUSTED_DISTRIBUTION.md) belgelerine bakın.

## Yayımlamadan önce açık kapılar

- Gerçek Windows 11 **x64** üzerinde ekran, klavye, çoklu monitör,
  Windows Explorer yeniden başlatma, erişilebilirlik ve güvenli geri yükleme.
- Gerçek Windows 10 22H2 kabuk davranışları isteniyorsa ayrıca sürüm bazında
  Windows 11 entegrasyonu ve belgelenmiş uyumluluk.
- Gerçek cihazda açılış hızlanması için kontrol edilmiş önce/sonra ölçüm.
- Ürün simgesi, gerçek Windows 11 ekran görüntüleri, yayıncı iletişim bilgisi.
- Partner Center sertifikasyonu ve **Microsoft imzalı** mağaza dağıtımı.

[Windows 11 test planı](docs/WINDOWS11_TEST_PLAN.md) ·
[Yayınlama kapıları](docs/RELEASE_GATES_TR.md) ·
[Gizlilik](docs/PRIVACY.md) ·
[Destek ve geri bildirim](https://github.com/ufukunal/win11to10/issues)
