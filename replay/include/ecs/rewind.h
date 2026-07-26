#pragma once
namespace ecs {
  class EntityManager;
  enum class DIRECTION {
    Rewind,
    Fastforward,
  };

  class RewindAction {
    friend EntityManager;

  public:
    DIRECTION last_direction = DIRECTION::Fastforward;
    RewindAction() = default;
    virtual ~RewindAction() = default;
    virtual void forward(EntityManager &mgr) = 0; // we are going forward in time
    virtual void backward(EntityManager &mgr) = 0; // we are going back in time
  };
}