# https://code.earthengine.google.com/ tutaj stwórz projekt a jego nazwę zapisz w pliku env.txt obok tego pliku py.
# python -m venv .venv
# venv\Scripts\activate
# pip install fastapi
# pip install uvicorn[standard]
# pip install earthengine-api
# Uruchom mnie za pomocą: uvicorn NAZWAPLIKUBEZKROPKAPY:app --reload

import ee
from fastapi import FastAPI
from pydantic import BaseModel
import time
from fastapi.middleware.cors import CORSMiddleware


app = FastAPI(title = "API")

app.add_middleware(CORSMiddleware, allow_origins = ["*"], allow_credentials = True, allow_methods = ["*"], allow_headers = ["*"])

with open("env.txt", "r", encoding = "utf-8") as file:
    PROJEKT = file.read().strip()

class ANALIZA(BaseModel):
    POCZATEK: int = 2018
    KONIEC: int = 2023
    X: float = -55.86
    Y: float = -7.58
    XX: float = -55.81
    YY: float = -7.555

def usuncienieichmury(OBRAZ):
    SCL = OBRAZ.select("SCL")
    MASKA = SCL.neq(3).And(SCL.neq(8)).And(SCL.neq(9)).And(SCL.neq(10))
    return OBRAZ.updateMask(MASKA)

def pobierzbezchmur(ROK, OBSZAR):
    return (ee.ImageCollection("COPERNICUS/S2_SR_HARMONIZED").filterBounds(OBSZAR).filterDate(str(ROK) + "-06-01", str(ROK) + "-08-31").map(usuncienieichmury).median())

def pobierzzchmurami(ROK, OBSZAR):
    return (ee.ImageCollection("COPERNICUS/S2_SR_HARMONIZED").filterBounds(OBSZAR).filterDate(str(ROK) + "-06-01", str(ROK) + "-08-31").median())

def pobierzdane(ROK, OBSZAR, CHMURY):
    if CHMURY == True:
        OBRAZ = pobierzzchmurami(ROK, OBSZAR)
        NAZWA = "CHM"
    else:
        OBRAZ = pobierzbezchmur(ROK, OBSZAR)
        NAZWA = "BEZ"

    PASMO = OBRAZ.select(["B8", "B4", "B3", "B2"])
    PASMA = PASMO.reproject(crs = "EPSG:3857", scale = 10)
    DANE = PASMA.sampleRectangle(region = OBSZAR)

    wyniki = {
        str(ROK) + "_" + NAZWA: {
            "B8": DANE.get("B8").getInfo(),
            "B4": DANE.get("B4").getInfo(),
            "B3": DANE.get("B3").getInfo(),
            "B2": DANE.get("B2").getInfo()
        }
    }

    return wyniki

ee.Initialize(project = PROJEKT)

@app.post("/analiza")
def analiza(req: ANALIZA):

    START = int(time.time())

    OBSZAR = ee.Geometry.Rectangle([req.X, req.Y, req.XX, req.YY])
    DANE = {}

    ROK = req.POCZATEK
    while ROK < req.KONIEC:
        DANE.update(pobierzdane(ROK, OBSZAR, CHMURY = True))
        DANE.update(pobierzdane(ROK, OBSZAR, CHMURY = False))

        ROK = ROK + 1

    POWIERZCHNIA = ee.Image.pixelArea().reproject(crs = "EPSG:3857", scale = 10)
    POWIERZCHNIE = POWIERZCHNIA.sampleRectangle(region = OBSZAR)
    MACIERZ = POWIERZCHNIE.get("area").getInfo()

    META = int(time.time())

    print(META - START)

    return {"DANE": DANE, "POWIERZCHNIA": MACIERZ}
