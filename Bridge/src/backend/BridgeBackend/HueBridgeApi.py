import requests
import json
import time

class HueBridgeApi:

    def __init__(self, ip, username, verbose=False):
        self.ip = ip
        self.username = username
        self.vebose = verbose

    def __set_hue_state(self, light_id, state):
        """
        Sets the state of the light with the given id"""
        url = f"http://{self.ip}/api/{self.username}/lights/{light_id}/state"
        response = requests.put(url, data=json.dumps(state))
        if self.vebose:
            print(state)
            print("-------------------")
            print(response.json())

    def __get_hue_state(self, light_id):
        """
        Returns the state of the light with the given id"""
        url = f"http://{self.ip}/api/{self.username}/lights/{light_id}"
        response = requests.get(url)
        if self.vebose:
            print(response.json())

        return response.json()['state']

    def get_all_lamp_ids(self):
        """
        Returns a list with all the lamp ids"""

        url = f"http://{self.ip}/api/{self.username}/lights"
        response = requests.get(url)
        ids = list(response.json().keys())
        if self.vebose:
            print(ids)
        return ids
    
    def get_all_lamp_status(self):
        """
        Returns a dictionary with the lamp id as key and a boolean as value
        True if the lamp is reachable, False otherwise"""
        url = f"http://{self.ip}/api/{self.username}/lights"
        response = requests.get(url)
        if self.vebose:
            print(response.json())

        lamp_status = {}
        for id in response.json():
            lamp_status[id] = response.json()[id]['state']['reachable']
            if self.vebose:
                print(f"Lamp {id} is {'reachable' if response.json()[id]['state']['reachable'] else 'not reachable'}")
        return lamp_status
    
    def get_light_state(self, light_id):
        """
        Returns the state of the light with the given id"""
        return self.__get_hue_state(light_id)

    def change_light_state(self, light_id, on=None, sat=None, bri=None, hue=None, transitiontime=None,ct=None):
        """
        light_id: int
        on: bool
        sat: float (0-1)
        bri: float (0-1)
        hue: float (0-1)
        transitiontime: float (0-inf)
        ct: float (0-1)
        """
        state = {}
        if on is not None:
            state["on"] = on

        if sat is not None:
            if isinstance(sat, float):
                sat = int(sat*254)
            state["sat"] = sat
        
        if bri is not None:
            if isinstance(bri, float):
                bri = int(bri*254)
            state["bri"] = bri
        
        if hue is not None:
            if isinstance(hue, float):
                hue = int(hue*65535)
            state["hue"] = hue

        if ct is not None:
            if isinstance(ct, float):
                ct = int(ct*500)
            state["ct"] = ct

        if transitiontime is not None:
            if isinstance(transitiontime, float):
                transitiontime = int(transitiontime*100)
            state["transitiontime"] = transitiontime

        self.__set_hue_state(light_id, state)

    def toggle_light(self, light_id):
        """
        Toggles the light with the given id"""
        state = self.__get_hue_state(light_id)
        state["on"] = not state["on"]
        self.__set_hue_state(light_id, state)

            






if __name__ == "__main__":
    hue_bridge = HueBridgeApi("192.168.1.14", "HdFwM9X9b1tOQHTkZ4sEe8B8JWNUPFjD6FyxZIbl")
    ids = hue_bridge.get_all_lamp_ids()
    print(ids)
    status = hue_bridge.get_all_lamp_status()
    print(status)
    #hue_bridge.change_light_state(2, on=False,transitiontime=0.0,bri=1.0)
    #hue_bridge.toggle_light(2)
    #hue_bridge.change_light_state(2, on=True,transitiontime=0.0,bri=1.0,ct=0.6)