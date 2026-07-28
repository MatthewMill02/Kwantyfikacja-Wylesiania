import ee
import json
from fastapi import FastAPI
from pydantic import BaseModel
import time
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import StreamingResponse

ee.Authenticate()

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

    def generuj():
        START = int(time.time())

        OBSZAR = ee.Geometry.Rectangle([req.X, req.Y, req.XX, req.YY])
        DANE = {}

        print(req.KONIEC - req.POCZATEK + 1)
        yield str(req.KONIEC - req.POCZATEK + 1) + "\n"

        ROK = req.POCZATEK
        while ROK <= req.KONIEC:
            DANE.update(pobierzdane(ROK, OBSZAR, CHMURY = True))
            INFO = "Pobrano dane dla roku " + str(ROK) + " z chmurami."
            print(INFO)
            yield INFO + "\n"

            DANE.update(pobierzdane(ROK, OBSZAR, CHMURY = False))
            INFO = "Pobrano dane dla roku " + str(ROK) + " bez chmur."
            print(INFO)
            yield INFO + "\n"

            ROK = ROK + 1

        POWIERZCHNIA = ee.Image.pixelArea().reproject(crs = "EPSG:3857", scale = 10)
        POWIERZCHNIE = POWIERZCHNIA.sampleRectangle(region = OBSZAR)
        MACIERZ = POWIERZCHNIE.get("area").getInfo()
        
        INFO = "Pobrano dane o powierzchni."
        print(INFO)
        yield INFO + "\n"

        META = int(time.time())
        print(META - START)

        WYNIK = {"DANE": DANE, "POWIERZCHNIA": MACIERZ}
        yield json.dumps(WYNIK) + "\n"

    return StreamingResponse(generuj(), media_type="text/plain")