from pydantic import BaseModel
from typing import List, Optional
from datetime import datetime

class LEDStateSchema(BaseModel):
    Id: int
    Brightness: int

class ButtonPressedEventSchemaInput(BaseModel):
    Id: int
    PressType: str
    IP: str
    


class ButtonPressedEventSchemaOutput(BaseModel):
    Id: int
    PressType: str
    IP: str
    LEDState: List[LEDStateSchema]
    Status: str


