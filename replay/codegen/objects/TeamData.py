
from .obj_base import ReflectableObject
from .obj_base import ReflectionVarMeta as Var

class TeamData(ReflectableObject):
    score = Var("uint16_t", 2)
    tickets = Var("uint16_t", 3)
    orderCooldownTotal = Var("uint32_t", 4)
    orderCooldownLeft = Var("uint32_t", 5)
    spawnScore = Var("uint32_t", 6)
    roundScore = Var("uint32_t", 7)