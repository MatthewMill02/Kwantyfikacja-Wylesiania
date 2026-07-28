Backend — backend.py (FastAPI + GEE + streaming progresu)
=======================================================

Plik główny: backend.py
Archiwum:   program88.py (bez streamingu — nieużywany przez start-backend.bat)

1. Skopiuj env.txt.example → env.txt i wpisz nazwę projektu GEE.

2. Uruchom z katalogu głównego: start-backend.bat

Endpoint: POST /analiza  (StreamingResponse, text/plain)

Protokół streamu (linie oddzielone \n):
  1) Liczba lat: KONIEC - POCZATEK + 1
  2) N × 2 komunikaty postępu (rok z chmurami / bez chmur)
  3) „Pobrano dane o powierzchni.”
  4) JSON wyniku: {"DANE": {...}, "POWIERZCHNIA": [...]}

Frontend liczy postęp: (lata × 2 + 1) kroków informacyjnych,
ostatnia linia to wynik końcowy (nie wlicza się do procentów).
