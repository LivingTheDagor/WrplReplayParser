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
    const uint32_t time_ms;
    DIRECTION last_direction = DIRECTION::Fastforward;
    RewindAction(const uint32_t time_ms) : time_ms(time_ms) {}
    virtual ~RewindAction() = default;
    virtual void forward(EntityManager &mgr) = 0; // we are going forward in time
    virtual void backward(EntityManager &mgr) = 0; // we are going back in time
  };
}