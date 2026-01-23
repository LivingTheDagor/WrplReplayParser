from abc import ABC, abstractmethod

class ReflectableObject(ABC):
    def __init__(self):
        pass

# we get var name from defined class, so we only need the var_id and optionally the special encoder this var may need, default is often enough though
class ReflectionVarMeta(ABC):
    def __init__(self, data_type: str, var_id: int, EncoderName=""):
        self.data_type = data_type
        self.var_id = var_id
        self.EncoderName = EncoderName
        self.has_custom_cider = EncoderName != ""

    def requestCoder(self, mgr: 'DataTypeManager'):
        if self.EncoderName == "":
            self.EncoderName = mgr.request_reflectionVar_serializer(self.data_type)