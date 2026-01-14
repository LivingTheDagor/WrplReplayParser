

#ifndef WTFILEUTILS_PARAM_H
#define WTFILEUTILS_PARAM_H
namespace dblk {
  struct Param
  {
    uint32_t nameId : 24;
    uint32_t type : 8;
    uint32_t v;
  };
}
#endif //WTFILEUTILS_PARAM_H
