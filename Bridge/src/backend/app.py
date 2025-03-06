from typing import Union
from fastapi.responses import HTMLResponse, FileResponse

import random

from fastapi import FastAPI
from pydantic import BaseModel

from BridgeBackend.systemSetup import hue_api

from ButtonPressedEventSchema import ButtonPressedEventSchemaInput, ButtonPressedEventSchemaOutput


app = FastAPI()


@app.post("/button_pressed_event")
def create_event(input_value: ButtonPressedEventSchemaInput):
    print(input_value)
    return ButtonPressedEventSchemaOutput(
        Id=input_value.Id,
        PressType=input_value.PressType,
        IP=input_value.IP,
        LEDState=[{
            "Id": 1,
            "Brightness": 100
        }],
        Status="OK"
    )

@app.get("/button_pressed_event")
def get_event(id: int, PressType: str, IP: str):
    print(f"Id: {id}, PressType: {PressType}, IP: {IP}")

    if PressType == "SHORT_PRESS":
        hue_api.toggle_light(2)
    elif PressType == "LONG_PRESS":
        if hue_api.get_light_state(2)["on"]:
            hue_api.change_light_state(2, on=True,transitiontime=0.0,bri=1.0,ct=0.6)
        else:
            hue_api.change_light_state(2, on=True,transitiontime=1.0,bri=0.5,ct=0.6)

    brightness = 2
    print(f"Random brightness: {brightness}") 
    return ButtonPressedEventSchemaOutput(
        Id=id,
        PressType=PressType,
        IP=IP,
        LEDState=[{
            "Id": 0,
            "Brightness": brightness
        }],
        Status="OK"
    )


# Ensure the OpenAPI documentation is available
@app.get("/openapi.json")
async def get_openapi():
    print(app.openapi())
    return app.openapi()

@app.get("/docs")
async def get_docs():
    print(app.docs_url)
    return app.docs_url

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)