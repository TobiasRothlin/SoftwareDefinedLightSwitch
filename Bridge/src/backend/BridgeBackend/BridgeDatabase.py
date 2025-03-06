from .JsonDatabase import JsonDatabase


class BridgeDatabase(JsonDatabase):

    def __init__(self, path):
        super().__init__(path)

    def reset(self):
        self.data = {
            "config": {
                "ConnectedAPIs": {
                },
            },
            "inputs": {},
            "outputs": {},
            "mappings": {},
        }
        self.save()
