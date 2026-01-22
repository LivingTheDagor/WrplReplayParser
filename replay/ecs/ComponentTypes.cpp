#include "ecs/EntityManager.h"
#include "ecs//ComponentTypes/objectType.h"
#include "ecs//ComponentTypes/arrayType.h"
#include "ecs/ComponentTypes/listType.h"
#include "ecs/ComponentTypes.h"


namespace ecs {
  CompileComponentTypeRegister *CompileComponentTypeRegister::tail = nullptr;
  ComponentSerializer default_serializer;
  size_t pull_components_type = 1;
  //const int MAX_STRING_LENGTH = 32768; // just for safety. Keep string size reasonable please!
  std::string Array::toString() const {
    std::ostringstream os;
    if(this->empty())
    {
      os << fmt::format("({})[]", this->size());
      return os.str();
    }
    os << fmt::format("({})\n    [\n", this->size());
    auto comps = g_ecs_data->getComponentTypes();
    for(const auto &comp : *this)
    {
      auto ctm = comps->getCTM(comp.getTypeId());
      auto str = ctm->toString((void *)comp.getRawData());
      os << "        ";
      os << comps->getName(comp.getTypeId());
      os << ": ";
      os << str;
      os << "        \n";
    }
    os << "    ]\n";
    return os.str();
  }

  type_index_t find_component_type_index(ecs::component_type_t component_type, ecs::EntityManager *emgr)
  {
    return g_ecs_data.get()->getComponentTypes()->findType(component_type);
  }
}; // namespace ecs


namespace ecs {
  type_index_t ComponentTypes::registerType(const char *name, ecs::component_type_t type, uint32_t data_size,
                                            ecs::ComponentSerializer *io,
                                            ecs::create_ctm_t ctm, ecs::destroy_ctm_t dtm, ComponentTypeFlags flags) {
    const type_index_t ctypeId = findType(type);
    if (ctypeId != INVALID_COMPONENT_TYPE_INDEX)
    {
      auto cData = this->getComponentData(ctypeId);
      if (strcmp(name, cData->name.data()) != 0)
      {
        EXCEPTION("component type <{}> with same hash ={:#x} as <{}> is already registered, hash collision.", cData->name.data(), type,
                  name);
        return INVALID_COMPONENT_TYPE_INDEX;
      }
      // This is not severe error per se but without logger developers are not noticing it during development
      EXCEPTION("ecs type <{}>({:#x}) is already registered", name, type);
      return ctypeId;
    }
    G_ASSERT_RETURN(getTypeCount() < INVALID_COMPONENT_TYPE_INDEX - 1, INVALID_COMPONENT_TYPE_INDEX);

    typesIndex.emplace(type, getTypeCount());
    types.push_back({io, data_size, type, std::string_view(name), nullptr, ctm, dtm, flags});
    //return (type_index_t)getTypeCount() - 1;
    LOG("created component {} that is '{}{}' type <{}> hash<{:#x}> of size {}",
        getTypeCount()-1,
        need_constructor(flags) ? "creatable" : "pod",
        io ? " io" : "",
        name,
        type,
        data_size);
    return (type_index_t)getTypeCount() - 1;
  }

  void ComponentTypes::initialize() {
    //clear();
    LOG("ecs: initialize component Types");
    // initialize eid and tag first, for debugging purposes?
    for (CompileComponentTypeRegister *start = CompileComponentTypeRegister::tail; start; start = start->next) {
      registerType(start->name, start->name_hash, start->size, start->io, start->ctm, start->dtm, start->flags);
    }
  }

  ComponentTypes::~ComponentTypes() {
    for(auto &x : this->types)
    {
      if(x.ctm)
      {
        x.destroy(x.ctm);
        x.ctm = nullptr;
      }
    }
  }

  ecs::Component &
  ecs::Object::insertWithCollision(hash_container_t::const_iterator hashIt, const HashedConstString str) {
    G_ASSERT(!noCollisions);
    if (hashIt != hashContainer.end() && *hashIt == str.hash) {
      auto it = container.begin() + (hashIt - hashContainer.begin());
      for (auto e = hashContainer.end(); hashIt != e; ++hashIt, ++it) {
        if (*hashIt != str.hash)
          break;
        int cmp = strcmp(it->first.c_str(), str.str);
        if (cmp == 0)
          return it->second;
        if (cmp > 0)
          break;
      }
    }
    if (hashIt == hashContainer.end()) {
      hashContainer.emplace_back(str.hash);
      return container.emplace_back(value_type{str.str, Component{}}).second;
    }
    auto it = container.begin() + (hashIt - hashContainer.begin());
    hashContainer.emplace(hashIt, str.hash);
    return container.emplace(it, value_type{str.str, Component{}})->second;
  }

  ecs::Object::const_iterator ecs::Object::findAsWithCollision(hash_container_t::const_iterator hashIt,
                                                               const HashedConstString str) const {
    G_ASSERT(!noCollisions);
    G_ASSERTF_RETURN(str.str, end(), "find_as can't be called without string on Object with hash collisions");
    auto start = container.begin() + (hashIt - hashContainer.begin());
    for (auto it = start, e = container.end(); it != e; ++it, ++hashIt) {
      if (*hashIt != str.hash)
        break;
      if (it->first == str.str)
        return it;
    }
    return end();
  }

  bool ecs::Object::slowIsEqual(const ecs::Object &a) const {
    G_ASSERT(!noCollisions && a.size() == size() && !a.noCollisions);
    auto ahi = a.hashContainer.begin();
    auto bhi = hashContainer.begin();
    for (auto ai = a.begin(), bi = begin(), be = end(); bi != be; ++bi, ++ai, ++ahi, ++bhi)
      if (*ahi != *bhi || ai->first != bi->first || ai->second != bi->second)
        return false;
    return true;
  }

  void ecs::Object::validate() const {
    G_ASSERT(hashContainer.size() == container.size());
    if (!size())
      return;
    auto hi = hashContainer.begin();
    for (auto i = begin(), e = end(); i != e; ++i, ++hi)
      G_ASSERT(*hi == ECS_HASH_SLOW(i->first.c_str()).hash);

    if (size() <= 1)
      return;
    if (noCollisions) {
      for (auto hi = hashContainer.begin(), he = hashContainer.end() - 1; hi != he; ++hi)
        G_ASSERT(*hi < *(hi + 1));
    } else {
      auto hi = hashContainer.begin();
      for (auto i = begin(), e = end() - 1; i != e; ++i, ++hi) {
        G_ASSERT(*hi <= *(hi + 1));
        if (*hi == *(hi + 1))
          G_ASSERT(strcmp(i->first.c_str(), (i + 1)->first.c_str()) < 0);
      }
    }
  }

  std::string ecs::Object::toString() const {
    std::ostringstream oss;
    oss << "{";
    auto comps = g_ecs_data->getComponentTypes();
    for(const auto &s : this->container)
    {
      auto ctm = comps->getCTM(s.second.getTypeId());
      oss << fmt::format("{}: '{}', ", s.first, ctm->toString((void *)s.second.getRawData()));
    }
    oss << "}";
    return oss.str();
  }

} // ecs