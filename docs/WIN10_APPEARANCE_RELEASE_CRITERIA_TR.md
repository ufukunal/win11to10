# NoyaPLANET 11to10 — Windows 10 görünümü yayın kabul kriterleri

Bu belge **tamamlandı** iddiası değildir. GitHub CI derlemesi, gerçek Windows 11 masaüstü testi veya güvenilir imza yerine geçmez.

## Ürün hedefi
Windows 11'i Windows 10'a **görünüm ve günlük kullanım bakımından** yaklaştırmak. İşletim sistemi sürümünü değiştirmek veya Windows sistem dosyalarını yamalamak kapsam dışıdır.

## Zorunlu kullanıcı deneyimi
- [ ] Başlat düğmesi ile açılan Windows 10 tarzı menü (yalnızca ayrı önizleme penceresi değil)
- [ ] Windows tuşu ile açma/kapatma, erişilebilirlik ve klavye gezinmesi
- [ ] Windows 10 tarzı görev çubuğu; pencere gruplama, sabitleme, çoklu monitör, DPI
- [ ] Saat/tarih, bildirim alanı ve üçüncü taraf tepsi simgeleri için belgelenmiş davranış
- [ ] Klasik sağ tık ve Explorer görünümü için desteklenen sürümlerde gerçek entegrasyon
- [ ] Kurulum, oturum açılışında başlatma, devre dışı bırakma ve eksiksiz kaldırma
- [ ] Explorer yeniden başlatıldığında ve uygulama çöktüğünde orijinal kabuğa güvenli dönüş

## Güvenlik ve uyumluluk
- [ ] Windows 11 x64 gerçek cihaz / sanal makine testleri
- [ ] Windows 11 ARM64 doğrulaması
- [ ] Çoklu ekran, farklı DPI, otomatik gizleme, tam ekran oyun, uyku/uyanma, RDP testleri
- [ ] Defender, Smart App Control ve Windows Update devre dışı bırakılmadan çalışma
- [ ] Lisans ve üçüncü taraf varlıkların denetimi
- [ ] Kod imzalama ve güvenilir dağıtım (Store sertifikasyonu veya uygun imzalı yükleyici)

## GitHub kalite kapıları
- [ ] x64 ve ARM64 CI yeşil
- [ ] Gerçek Windows 11 masaüstü manuel test kanıtları
- [ ] Sürüm notları, SHA-256, geri dönüş adımları
- [ ] İmzalı kurulum paketi ve kurulum/kaldırma testi

## Mevcut sınırlama
v0.7 kaynak ağacındaki Start, AppBar ve Explorer pencereleri Windows'un gerçek kabuğunu değiştirmeyen **önizlemelerdir**. Bu belge tamamlanmadan projeyi Windows 10 arayüzünün birebir yerini alan yayınlanabilir ürün olarak tanıtmayın.
