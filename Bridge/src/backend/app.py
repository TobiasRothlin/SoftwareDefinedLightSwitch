from typing import Union
from fastapi.responses import HTMLResponse, FileResponse

from fastapi import FastAPI
from pydantic import BaseModel

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