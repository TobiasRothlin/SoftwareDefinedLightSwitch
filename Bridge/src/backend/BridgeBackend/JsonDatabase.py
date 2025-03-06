import json


class JsonDatabase:

    def __init__(self, path):
        self.path = path
        self.data = {}

        self.did_file_exist = False
        try:
            with open(self.path, 'r') as file:
                self.did_file_exist = True
                self.load()
        except FileNotFoundError:
            print(f"File {self.path} not found, creating new file")
            self.save()
        

    def load(self):
        try:
            with open(self.path, 'r') as file:
                self.data = json.load(file)
        except FileNotFoundError:
            self.data = {}

    def save(self):
        with open(self.path, 'w') as file:
            json.dump(self.data, file)

    def get(self, key):
        return self.data.get(key)
    
    def set(self, key, value):
        self.data[key] = value
        self.save()

    def set_data(self,data):
        self.data = data
        self.save()