Backend — program88.py (FastAPI + Google Earth Engine)
======================================================

Plik główny: program88.py

1. Skopiuj env.txt.example → env.txt i wpisz nazwę projektu GEE
   (https://code.earthengine.google.com/).

2. Z katalogu głównego projektu uruchom:

      start-backend.bat

   Serwer:     http://127.0.0.1:8000
   Endpoint:   POST /analiza
   Dokumentacja: http://127.0.0.1:8000/docs

Kontrakt żądania (JSON):
  POCZATEK, KONIEC, X, Y, XX, YY

Kontrakt odpowiedzi:
  DANE: { "{rok}_BEZ": {B2,B3,B4,B8}, "{rok}_CHM": {...}, ... }
       lata z zakresu [POCZATEK, KONIEC)
  POWIERZCHNIA: macierz m² na piksel

Uwaga: frontend liczy NDVI, kompozycje i maskę wylesień lokalnie (C++).
