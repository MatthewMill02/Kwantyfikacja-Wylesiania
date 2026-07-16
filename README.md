# Wylesianie

Aplikacja desktopowa do **automatycznej detekcji i kwantyfikacji wylesiania** na podstawie danych satelitarnych. Projekt powstaje w **Qt Creator** z wykorzystaniem **Qt Designer** (Widgets, C++17, CMake).

## Cel projektu

Narzędzie ma wskazać obszary utraty lasu w zadanym przedziale czasu i obliczyć łączną powierzchnię tej utraty wyrażoną w hektarach.

## Źródła danych

Analiza opiera się na obrazach satelitarnych z jednego z poniższych zbiorów:

- **Sentinel-2 MSI (Level-2A)**
- **Landsat 8 Surface Reflectance**

## Wizualizacja początkowa

Interfejs użytkownika powinien umożliwiać:

- wyświetlenie wybranego obszaru leśnego (np. Puszcza Białowieska, fragment Amazonii),
- porównanie dwóch lat (np. 2018 i 2023),
- przełączanie między kompozycjami **True Color** i **False Color** (podkreślającą roślinność),
- interaktywne porównanie obrazów za pomocą **split-panel slidera**.

## Algorytm detekcji

Pipeline przetwarzania obejmuje następujące kroki:

| Krok | Opis |
|------|------|
| 1. Maskowanie chmur | Odfiltrowanie chmur na podstawie pasma QA (Quality Assessment). |
| 2. Obliczenie NDVI | Wskaźnik znormalizowanej różnicy wegetacji dla obu okresów: |
| 3. Różnica NDVI | Obliczenie zmiany między okresami: |
| 4. Progowanie | Wyodrębnienie pikseli, w których spadek NDVI przekracza ustalony próg (np. > 0,3 → wylesienie). |
| 5. Powierzchnia | Przeliczenie liczby pikseli na powierzchnię w **hektarach**. |

**NDVI:**

```
NDVI = (NIR - Red) / (NIR + Red)
```

**Zmiana NDVI w czasie:**

```
ΔNDVI = NDVI_T2 - NDVI_T1
```

## Wizualizacja wyników

Po zakończeniu analizy aplikacja powinna prezentować:

- **nakładkę mapową** — czerwone piksele lub poligony oznaczające utratę lasu na mapie bazowej,
- **wykres słupkowy** — łączną utratę lasu w hektarach w podziale na analizowane lata.

## Wymagania

- **Qt** 6.x (lub 5.x — projekt wspiera obie wersje przez CMake)
- **Kompilator C++17** (np. MinGW 64-bit w konfiguracji Qt Creator)
- **CMake** ≥ 3.16
- **Qt Creator** z modułem Qt Widgets

## Uruchomienie w Qt Creator

1. Otwórz plik `CMakeLists.txt` w Qt Creator (*File → Open File or Project…*).
2. Wybierz kit (np. *Desktop Qt 6.x MinGW 64-bit*).
3. Skonfiguruj projekt (*Configure Project*).
4. Zbuduj (*Build → Build Project „Wylesianie”*) i uruchom (*Run*).

Katalog `build/` jest generowany lokalnie przez Qt Creator i nie trafia do repozytorium (patrz `.gitignore`).

## Architektura (frontend / backend)

| Warstwa | Odpowiedzialność |
|---------|------------------|
| **Frontend (ten projekt)** | Formularz, progress, renderowanie kompozycji pixel-by-pixel w C++, suwaki progów, toggle chmur |
| **Backend (FastAPI + GEE)** | Pobranie Sentinel-2, maskowanie chmur, surowe pasma / PPM — kontrakt `POST /analiza` |

Na razie frontend działa w **trybie mock** (`BackendClient::setUseMock(true)`): symuluje postęp i generuje syntetyczne pasma B2/B3/B4/B8. Po podłączeniu backendu wystarczy wyłączyć mock.

## Struktura projektu

```
Wylesianie/
├── CMakeLists.txt
├── main.cpp / mainwindow.* / mainwindow.ui   # QStackedWidget: formularz → progress → wyniki
├── models/          # AnalysisRequest, AnalysisResult, BandBuffer
├── network/         # BackendClient (HTTP + mock)
├── processing/      # ImageRenderer, MockDataGenerator
├── widgets/         # ImagePanelWidget, ResultsViewWidget
├── README.md
└── .gitignore
```

## Status

Działa pełny przepływ UI z lokalnym renderowaniem True/False Color, własnej zieleni, maski wylesień, pasm B4/B8 oraz ich średniej. Integracja z prawdziwym backendem GEE — kolejny etap.
