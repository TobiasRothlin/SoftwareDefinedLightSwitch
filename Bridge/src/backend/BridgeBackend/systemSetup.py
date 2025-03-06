from .BridgeDatabase import BridgeDatabase
from .HueBridgeApi import HueBridgeApi
import json

CONFIG_FILE = "./config.json"
DB_FILE = "./bridge.json"


with open(CONFIG_FILE, "r") as f:
    config = json.load(f)

hue_api = HueBridgeApi(config["Hue"]["ip"], config["Hue"]["username"])
   
        
