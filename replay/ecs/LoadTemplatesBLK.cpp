
#include "FileSystem.h"
#include "VROMFs.h"
#include "ecs/EntityManager.h"
#include "ecs/ComponentTypesDefs.h"
#include <unordered_set>
#include "regex"

namespace ecs {
  template<typename T>
  type_index_t getTypeIndex() {
    static ComponentTypes *types;
    if (!types) {
      types = g_ecs_data->getComponentTypes();
    }
    auto index = types->findType(ComponentTypeInfo<T>::type);
    if (index == INVALID_COMPONENT_TYPE_INDEX)
      EXCEPTION("Unable to find type {}", ComponentTypeInfo<T>::type_name);
    return index;
  }

  component_index_t getComponentIndex(const char *name, type_index_t idx) {
    static DataComponents *types;
    static ComponentTypes *cTypes;
    if (!types) {
      types = g_ecs_data->getDataComponents();
      cTypes = g_ecs_data->getComponentTypes();
    }
    auto name_hash = ECS_HASH(name);
    auto index = types->getIndex(name_hash.hash);
    if (index == INVALID_COMPONENT_TYPE_INDEX)
    {
      index = types->createComponent(name_hash, idx, nullptr, *cTypes);
      if (index == INVALID_COMPONENT_TYPE_INDEX)
        EXCEPTION("Unable to create type {}", name);
    }
    return index;
  }
}


// reserved name declare info about the template instead of declaring components directly
inline bool is_reserved_name(std::string &name) {
  return name[0] == '_';
}

inline bool is_reserved_name(std::string_view &name) {
  return name[0] == '_';
}

inline bool is_reserved_name(const char *name) {
  return name[0] == '_';
}

void parseNameType(const std::string &input, std::string &name, std::string &type) {
  size_t pos = input.find(':');
  if (pos == std::string::npos) {
    name = input;
  } else {
    name = input.substr(0, pos);
    type = input.substr(pos + 1);
  }
}

namespace ecs {
  struct TemplateParseContext {

    std::vector<ecs::ComponentTemplInfo> *components;
    std::vector<ecs::template_t> *parents;
    TemplateDB *db;
    std::string templName;
    int valueNid;
    bool singleton = false;
    static const std::unordered_set<std::string> allowed_tags;
    std::string mangle_type_name{};

    void parse(DataBlock &blk);
  };
}


class LoadContext;
namespace ecs {


  static void load_object(TemplateParseContext &ctx, const char *name, DataBlock &blk);

  static void load_array(TemplateParseContext &ctx, const char *name, DataBlock &);

  template<typename T>
  static void load_list(TemplateParseContext &ctx, const char *name, DataBlock &);


  static inline ecs::string load_blk_str_impl(TemplateParseContext &ctx, DataBlock &blk) {
    return blk.getStrByNameId(ctx.valueNid, "");
  }

  static inline void load_blk_str(TemplateParseContext &ctx, const char *name, DataBlock &blk) {
    ecs::string str = load_blk_str_impl(ctx, blk);
    auto index = getComponentIndex(name, getTypeIndex<ecs::string>());
    ctx.components->emplace_back(index, std::move(str));
  }


  template<typename T, T (DataBlock::*getter)(int) const, typename U = T>
  static inline void load_blk_type(TemplateParseContext &ctx, const char *name, DataBlock &blk) {
    auto index = getComponentIndex(name, getTypeIndex<U>());
    ctx.components->emplace_back(index, U(std::invoke(getter, blk, blk.findParam(ctx.valueNid))));
  }

  typedef void (*block_load_t)(TemplateParseContext &ctx, const char *comp_name, DataBlock &);

  struct BlockLoaderDesc {
    const char *typeName = NULL;
    size_t typeNameLen = 0;
    block_load_t load = NULL;
    component_type_t compType = 0;
  };


#define LIST_TYPE(name, tn)                                                               \
  {TYPENAMEANDLEN("list<" #name ">"), &load_list<tn>, ComponentTypeInfo<List<tn>>::type},
#define LIST_TYPES              \
  LIST_TYPE(eid, ecs::EntityId) \
  LIST_TYPE(i, int)             \
  LIST_TYPE(i8, int8_t)         \
  LIST_TYPE(i16, int16_t)       \
  LIST_TYPE(i32, int)           \
  LIST_TYPE(i64, int64_t)       \
  LIST_TYPE(u8, uint8_t)        \
  LIST_TYPE(u16, uint16_t)      \
  LIST_TYPE(u32, uint32_t)      \
  LIST_TYPE(u64, uint64_t)      \
  LIST_TYPE(t, ecs::string)     \
  LIST_TYPE(r, float)           \
  LIST_TYPE(p2, Point2)         \
  LIST_TYPE(p3, Point3)         \
  LIST_TYPE(p4, Point4)         \
  LIST_TYPE(ip2, IPoint2)       \
  LIST_TYPE(ip3, IPoint3)       \
  LIST_TYPE(ip4, IPoint4)       \
  LIST_TYPE(b, bool)            \
  LIST_TYPE(m, TMatrix)         \
  LIST_TYPE(c, E3DCOLOR)

#define TYPENAMEANDLEN(x) x, sizeof(x) - 1
  // we only gonna use this to not parse shared components, I could care less about them right now
#define SHARED_PREFIX     "shared:"
#define SHARED_PREFIX_LEN uint32_t(7) // uint32_t(strlen(SHARED_PREFIX))


  static const BlockLoaderDesc block_loaders[] = {LIST_TYPES {TYPENAMEANDLEN("object"), &load_object},
                                                  {TYPENAMEANDLEN("array"), &load_array},
                                                  {TYPENAMEANDLEN("tag"),
                                                   [](TemplateParseContext &ctx, const char *comp_name,
                                                      DataBlock &) {
                                                     auto index = getComponentIndex(comp_name, getTypeIndex<ecs::Tag>());
                                                     ctx.components->emplace_back(index, Component(ecs::Tag()));
                                                   },
                                                   ComponentTypeInfo<Tag>::type},
                                                  {TYPENAMEANDLEN("eid"),
                                                   [](TemplateParseContext &ctx, const char *name,
                                                      DataBlock &blk) {
                                                     ecs::EntityId eid;
                                                     const int valueParamId = blk.findParam(ctx.valueNid);
                                                     if (valueParamId >= 0)
                                                       eid = ecs::EntityId((ecs::entity_id_t) blk.getInt(valueParamId));
                                                     auto index = getComponentIndex(name, getTypeIndex<EntityId>());
                                                     ctx.components->emplace_back(index, Component(eid));
                                                   },
                                                   ComponentTypeInfo<EntityId>::type},
                                                  {TYPENAMEANDLEN("t"), &load_blk_str,
                                                   ComponentTypeInfo<ecs::string>::type},
                                                  {TYPENAMEANDLEN("i"), &load_blk_type<int, &DataBlock::getInt>,
                                                   ComponentTypeInfo<int>::type},
                                                  {TYPENAMEANDLEN("r"), &load_blk_type<float, &DataBlock::getReal>,
                                                   ComponentTypeInfo<float>::type},
                                                  {TYPENAMEANDLEN("p2"), &load_blk_type<Point2, &DataBlock::getPoint2>,
                                                   ComponentTypeInfo<Point2>::type},
                                                  {TYPENAMEANDLEN("p3"), &load_blk_type<Point3, &DataBlock::getPoint3>,
                                                   ComponentTypeInfo<Point3>::type},
                                                  {TYPENAMEANDLEN("dp3"),
                                                   &load_blk_type<Point3, &DataBlock::getPoint3, DPoint3>,
                                                   ComponentTypeInfo<DPoint3>::type},
                                                  {TYPENAMEANDLEN("p4"), &load_blk_type<Point4, &DataBlock::getPoint4>,
                                                   ComponentTypeInfo<Point4>::type},
                                                  {TYPENAMEANDLEN("ip2"),
                                                   &load_blk_type<IPoint2, &DataBlock::getIPoint2>,
                                                   ComponentTypeInfo<IPoint2>::type},
                                                  {TYPENAMEANDLEN("ip3"),
                                                   &load_blk_type<IPoint3, &DataBlock::getIPoint3>,
                                                   ComponentTypeInfo<IPoint3>::type},
                                                  {TYPENAMEANDLEN("b"), &load_blk_type<bool, &DataBlock::getBool>,
                                                   ComponentTypeInfo<bool>::type},
                                                  {TYPENAMEANDLEN("m"), &load_blk_type<TMatrix, &DataBlock::getTMatrix>,
                                                   ComponentTypeInfo<TMatrix>::type},
                                                  {TYPENAMEANDLEN("c"),
                                                   &load_blk_type<E3DCOLOR, &DataBlock::getE3DColor>,
                                                   ComponentTypeInfo<E3DCOLOR>::type},
                                                  {TYPENAMEANDLEN("i8"),
                                                   &load_blk_type<int, &DataBlock::getInt, int8_t>,
                                                   ComponentTypeInfo<int8_t>::type},
                                                  {TYPENAMEANDLEN("i16"),
                                                   &load_blk_type<int, &DataBlock::getInt, int16_t>,
                                                   ComponentTypeInfo<int16_t>::type},
                                                  {TYPENAMEANDLEN("i32"), &load_blk_type<int, &DataBlock::getInt>,
                                                   ComponentTypeInfo<int>::type},
                                                  {TYPENAMEANDLEN("i64"),
                                                   &load_blk_type<uint64_t, &DataBlock::getUInt64>, // SSSSH
                                                   ComponentTypeInfo<int64_t>::type},
                                                  {TYPENAMEANDLEN("u8"),
                                                   &load_blk_type<int, &DataBlock::getInt, uint8_t>,
                                                   ComponentTypeInfo<uint8_t>::type},
                                                  {TYPENAMEANDLEN("u16"),
                                                   &load_blk_type<int, &DataBlock::getInt, uint16_t>,
                                                   ComponentTypeInfo<uint16_t>::type},
                                                  {TYPENAMEANDLEN("u32"),
                                                   &load_blk_type<int, &DataBlock::getInt, uint32_t>,
                                                   ComponentTypeInfo<uint32_t>::type},
                                                  {TYPENAMEANDLEN("u64"),
                                                   &load_blk_type<uint64_t, &DataBlock::getUInt64, uint64_t>,
                                                   ComponentTypeInfo<uint64_t>::type},
                                                  {TYPENAMEANDLEN("BBox3"),
                                                   [](TemplateParseContext &ctx, const char *name,
                                                      DataBlock &) {
                                                     auto index = getComponentIndex(name, getTypeIndex<BBox3>());
                                                     ctx.components->emplace_back(index, Component(BBox3()));
                                                   },
                                                   ComponentTypeInfo<BBox3>::type}};
#undef TYPEANDLEN
#undef LIST_TYPES
#undef LIST_TYPE


  template<typename Cb>
  static inline void load_component_list_impl(TemplateParseContext &ctx, DataBlock &blk, const std::string &type_name, Cb cb) {
    auto prevList = ctx.components;
    std::vector<ecs::ComponentTemplInfo> olist{};
    olist.reserve(500); // there is an issue with the resise op, so we want to avoid it as much as possible.
    ctx.components = &olist;
    //ctx.info = nullptr;
    //ctx.db = nullptr;
    //auto sets = ctx.sets;
    //ctx.sets = decltype(sets)();
    ctx.mangle_type_name = type_name;
    ctx.parse(blk);
    ctx.mangle_type_name = "";
    ctx.components = prevList;
    //ctx.info = prevInfo;
    //ctx.db = prevDb;
    //ctx.sets = sets;
    cb(std::move(olist));
  }

  static inline ecs::Object load_object_impl(TemplateParseContext &ctx, DataBlock &blk) {
    ecs::Object out_object;
    uint32_t cap = blk.paramCount() + blk.blockCount();
    out_object.reserve(cap);
    auto datacmps = g_ecs_data->getDataComponents();
    load_component_list_impl(ctx, blk, "ecs_object", [&out_object, datacmps](std::vector<ecs::ComponentTemplInfo> &&clist) {
      for (auto &&objElem: clist)
        out_object.addMember(datacmps->getDataComponent(objElem.comp_type_index)->getName().data(),
                             std::move(objElem.default_component));
    });
    if (out_object.size() <=
        ((cap * 3u) / 4u)) // if we are wasting at least 25% or more (TODO: tune this formula according to real
      // world usage)
      out_object.shrink_to_fit();
    return out_object;
  }

  static inline ecs::Array load_array_impl(TemplateParseContext &ctx, DataBlock &blk) {
    ecs::Array out_array;
    out_array.reserve((ecs::Array::base_type::size_type)(blk.paramCount() + blk.blockCount()));
    load_component_list_impl(ctx, blk, "ecs_array", [&out_array](std::vector<ecs::ComponentTemplInfo> &&clist) {
      for (auto &&arrElem: clist)
        out_array.push_back(std::move(arrElem.default_component));
    });
    if (out_array.size() <=
        ((out_array.capacity() * 3u) / 4u)) // if we are wasting at least 25% or more (TODO: tune this formula
      // according to real world usage)
      out_array.shrink_to_fit();
    return out_array;
  }

  template<typename T>
  static inline T load_array_impl(TemplateParseContext &ctx, DataBlock &blk) {
    T out_array; // ecs::list<T>

    out_array.reserve((typename T::size_type)(blk.paramCount() + blk.blockCount()));
    std::string mangle_name = typeid(T).name();
    mangle_name += "ecs_list_";
    load_component_list_impl(ctx, blk, mangle_name, [&ctx, &out_array, blk](std::vector<ecs::ComponentTemplInfo> &&clist) {
      for (auto &&arrElem: clist) {
        if (arrElem.default_component.is<typename T::value_type>())
          out_array.push_back(std::move(*(typename T::value_type *) arrElem.default_component.getRawData()));
        else {
          EXCEPTION("debug this I dont know how to fix this format string");
}

      }
    });
    if (out_array.size() <=
        ((out_array.capacity() * 3u) / 4u)) // if we are wasting at least 25% or more (TODO: tune this formula
      // according to real world usage)
      out_array.shrink_to_fit();
    return std::move(out_array);
  }

  static void load_object(TemplateParseContext &ctx, const char *name, DataBlock &blk) {
    ecs::Object object = load_object_impl(ctx, blk);
    auto cidx = getTypeIndex<ecs::Object>();
    auto index = getComponentIndex(name, cidx);
    ctx.components->emplace_back(index, std::move(object));
  }


  static void load_array(TemplateParseContext &ctx, const char *name, DataBlock &blk) {
    ecs::Array array = load_array_impl(ctx, blk);
    auto cidx = getTypeIndex<ecs::Array>();
    auto index = getComponentIndex(name, cidx);
    ctx.components->emplace_back(index, std::move(array));
  }


  template<typename T>
  static void load_list(TemplateParseContext &ctx, const char * name, DataBlock &blk) {
    ecs::List<T> list = load_array_impl<ecs::List<T>>(ctx, blk);
    auto index = getComponentIndex(name, getTypeIndex<ecs::List<T>>());
    ctx.components->emplace_back(index, std::move(list));
  }

  static inline const BlockLoaderDesc *find_type_block_loader(std::string_view tp_name) {
    auto pred = [tp_name](const BlockLoaderDesc &d) {
      return std::string_view{d.typeName, d.typeNameLen} == tp_name;
    };
    auto it = std::find_if(std::begin(block_loaders), std::end(block_loaders), pred);
    return it == std::end(block_loaders) ? nullptr : it;
  }

  std::string mangle_name(const char * n, const std::string &parent_type, const std::string &child_type) {
    std::string n1(n);
    n1 += "_" + parent_type + "_" + child_type;
    return n1;
  }

  void TemplateParseContext::parse(DataBlock &blk) {
    //std::cout << "parsing template " << blk.getBlockName() << "\n";
    const char *ftags = blk.getStr("_tags", nullptr);
    // load if no tags or if the returned iterator isnt end
    //if(blk.getBlockName() == "dynamic_hair_renderer")
    //  std::cout << "womp womp\n";
    bool do_load = !ftags || allowed_tags.find(ftags) != allowed_tags.end();
    if (!do_load)
      return;
    int singletonId = blk.getNameId("_singleton");
    //int tagsId = blk.getNameId("_tags");
    int useId = blk.getNameId("_use");
    int trackedId = blk.getNameId("_tracked");
    int trackId = blk.getNameId("_track");
    int replicatedId = blk.getNameId("_replicated");
    int replicateId = blk.getNameId("_replicate");
    int hideId = blk.getNameId("_hide");
    int groundId = blk.getNameId("_group");
    int tagsId = blk.getNameId("_tags");
    int reservedReplId = blk.getNameId("_skipInitialReplication");
    int overrideId = blk.getNameId("_override");
    for (int i = 0; i < blk.paramCount(); i++) {
      if (is_reserved_name(blk.getParamName(i))) {
        int nid = blk.getParamNameId(i);
        if (nid == singletonId)
          singleton = true;
        else if (nid == useId) {
          auto str = blk.getStr(i);
          std::string name = std::string(str);
          template_t tid = db->ensureTemplate(name); // sometimes we can ask for a template before it is created
          this->parents->emplace_back(tid);
        } else if (nid == trackId || nid == trackedId || nid == replicateId || nid == replicatedId ||
                   nid == hideId || nid == tagsId || nid == reservedReplId || nid == overrideId) {} // dont care about these right now
        else {
          EXCEPTION("Unkown reserved name type {} parsing template {}", blk.getParamName(i), blk.getBlockName().data());
        }
      } else {

        uint8_t type_id = blk.getParamType(i);
        //LOG("parsing %s type derived from param", blk.getParamName(i));
        Component comp;
        switch (type_id) {
          case DataBlock::TYPE_NONE:
            EXCEPTION("Invalid Type");
          case DataBlock::TYPE_STRING: {
            comp = ecs::Component{ecs::string(blk.getStr(i))};
            break;
          }
#define parse_type(case_, ret_name) \
            case DataBlock::TYPE_##case_: {\
            comp = std::move(ecs::Component{blk.get##ret_name(i)});       \
              break;               \
            }
          parse_type(INT, Int)
          parse_type(REAL, Real)
          parse_type(POINT2, Point2)
          parse_type(POINT3, Point3)
          parse_type(POINT4, Point4)
          parse_type(IPOINT2, IPoint2)
          parse_type(IPOINT3, IPoint3)
          parse_type(BOOL, Bool)
          parse_type(E3DCOLOR, E3DColor)
          parse_type(MATRIX, TMatrix)
          case DataBlock::TYPE_UINT64: {
            comp = ecs::Component{blk.getUInt64(i)};
            break;
          }
          case DataBlock::TYPE_COUNT:
          default:
            EXCEPTION("Invalid Type");
        }
        auto name_ = this->mangle_type_name.empty() ? std::string(blk.getParamName(i)) : mangle_name(blk.getParamName(i), this->mangle_type_name, DataBlock::ParamTypeNames[blk.getParamType(i)]);
        auto hash = ECS_HASH(name_.c_str());
        ecs::component_index_t idx = g_ecs_data->getDataComponents()->getIndex(hash.hash);
        if (idx == INVALID_COMPONENT_INDEX) {
          idx = g_ecs_data->createComponent(hash, comp.getTypeId(), nullptr);
        }
        this->components->emplace_back(idx, std::move(comp));
      }
    }

    for (int i = 0; i < blk.blockCount(); i++) {

      DataBlock *subBlock = blk.getBlock(i).get();
      if (subBlock->getBlockNameId() == groundId)
        parse(*subBlock);
      const char *ftags = subBlock->getStr("_tags", nullptr);
      do_load = !ftags || allowed_tags.find(ftags) != allowed_tags.end();
      if (!do_load)
        continue;
      if (is_reserved_name(subBlock->getBlockName().data())) {
        // all the game does here is just add metadata, which has zero use for me
        continue;
      }
      auto dataComps = ecs::g_ecs_data->getDataComponents();
      auto comps = ecs::g_ecs_data->getComponentTypes();
      std::string name, type;
      parseNameType(std::string(subBlock->getBlockName()), name, type);
      //component_index_t comp_index = INVALID_COMPONENT_INDEX;
      if (!type.empty()) {
        if (type.starts_with(SHARED_PREFIX)) // cant handle those
          continue;
      }
      if (type.empty()) // name talks about preexisting component
      {
        auto hash = ECS_HASH(name.c_str()).hash;

        component_index_t index = dataComps->getIndex(hash);
        if (index == INVALID_COMPONENT_INDEX)
          EXCEPTION("Failed to find datacomponent {}", name);
        components->emplace_back(index,
                                 Component{nullptr,
                                                     hash,
                                                     dataComps->getDataComponent(index)->componentIndex,
                                                     comps->getComponentData(
                                                         dataComps->getDataComponent(index)->componentIndex)->size});
        continue;
      } else if (auto typeLoader = find_type_block_loader(type)) {
        typeLoader->load(*this,name.c_str(), *subBlock);

      } else {
        auto hash = ECS_HASH(type.c_str());
        ecs::type_index_t index = comps->findType(hash.hash);
        if (index == INVALID_COMPONENT_TYPE_INDEX)
          EXCEPTION("Unable to find type {}", type);
        G_ASSERT(this->mangle_type_name.empty()); // this should never encounter name mangling
        //auto name_ = this->mangle_type_name.empty() ? name : mangle_name(name.c_str(), this->mangle_type_name, type);

        component_index_t idx = dataComps->createComponent(ECS_HASH(name.c_str()), index, nullptr, *comps);
        components->emplace_back(idx, Component{
            nullptr, hash.hash, index, comps->getComponentData(index)->size
        });
      }
    }
  }


  const std::unordered_set<std::string> TemplateParseContext::allowed_tags = {"gameClient",
                                                                              "ecsDebug",
                                                                              "render",
                                                                              "sound",
                                                                              "hangar",
                                                                              "anyScene",
                                                                              "soundNet",
                                                                             // "server" // need for Event system
        }; // only care about these




}

#if 0
#define PATH_DELIM      '\\'
#define PATH_DELIM_BACK '/'
#define PATH_DELIM_STR  "\\"
#else
#define PATH_DELIM      '/'
#define PATH_DELIM_BACK '\\'
#define PATH_DELIM_STR  "/"
#endif

const char *dd_get_fname(const char *path)
{
  if (!path || !path[0])
    return path;

  const char *p = strrchr(path, PATH_DELIM_BACK);
  if (p)
    p++;
  else
    p = path;

  const char *p1 = strrchr(p, PATH_DELIM);
  if (p1)
    p = p1 + 1;

  return p;
}


// stores various information needed while loading templates
class LoadContext {
  std::unordered_set<std::string> ResolvedImports; // imports that have already been resolved and parsed
  ecs::TemplateDB *db = ecs::g_ecs_data->getTemplateDB();
public:
  bool have_we_parsed(const std::string &path) {
    std::string copy = path;
    char * ptr = const_cast<char *>(copy.data());
    for(size_t i = 0; i < copy.size(); i++) {
      if(ptr[i] == '\\') {
        ptr[i] = '/';
      }
      ptr[i] = (char)std::tolower(ptr[i]);
    }
    auto iter = this->ResolvedImports.find(copy);
    if(iter == this->ResolvedImports.end())
    {
      this->ResolvedImports.emplace(copy);
      return false;
    }
    return true;
  }

  void parse_templates_from_blk(DataBlock *blk, ecs::TemplateDB *overrides) {
    for (int i = 0; i < blk->blockCount(); i++) // each 'block' is a template
    {
      DataBlock *tmpl_blk = blk->getBlock(i).get();
      auto db_ = db;
      if (is_reserved_name(tmpl_blk->getBlockName().data()))
      {
        //LOGE("Reserved names are not supported");
        continue;
      }
      if(tmpl_blk->getBool("_override", false))
        db_ = overrides;
      std::vector<ecs::ComponentTemplInfo> components;
      components.reserve(500); // there is an issue with the resise op, so we want to avoid it as much as possible. TODO
      std::vector<ecs::template_t> parents;
      ecs::TemplateParseContext ctx{&components, &parents, db_, blk->getBlockName().data(), blk->getNameId("_value")};
      ctx.templName = tmpl_blk->getBlockName().data();
      ctx.parse(*tmpl_blk);
      ecs::Template templ{std::string(tmpl_blk->getBlockName()), std::move(components), std::move(parents)};
      db_->AddTemplate(std::move(templ));
    }
  }


  /// resolves the imports for one path
  void resolve_one_import(const char *path, fs::path &src_folder, bool is_optional, ecs::TemplateDB *overrides) {
    bool abs_path = (path[0] == '#');
    bool mnt_path = (path[0] == '%');
    if (abs_path)
      path++;
    // checks if this is a wildcard import
    if (strchr(path, '*')) {
      std::vector<std::string> files{};
      // imp_fn == path
      const char *fn_with_ext = dd_get_fname(path);
      G_ASSERTF(strchr(path, '*') >= fn_with_ext, "imp_fn={}{}", abs_path ? "#" : "", path);
      std::string fn_match_re{};
      fn_match_re.reserve(strlen(fn_with_ext) * 2 + 1);
      for (const char *p = fn_with_ext; *p; p++)
        if (*p == '*')
          fn_match_re += ".*";
        else if (strchr("\\()[].+?^$", *p))
        {
          fn_match_re += '\\';
          fn_match_re += *p;
        }
        else
          fn_match_re += *p;
      fn_match_re += '$';

      std::regex pattern{fn_match_re};

      std::string str{};
      char buffer[512];
      snprintf(buffer, sizeof(buffer), "%s/%.*s",
               (abs_path || mnt_path) ? "" : src_folder.string().c_str(),
               static_cast<int>(fn_with_ext - path)-1,
               path);
      std::string buff_str{buffer};
      file_mgr.find_files_in_folder(files, buff_str);
      bool anyFileLoaded = false;
      for(const auto &s : files)
      {
        if(std::regex_match(s, pattern))
        {
          anyFileLoaded = true;
          DataBlock imp_blk;
          bool was_parsed = this->have_we_parsed(s);
          if(was_parsed)
            continue;
          if(load(imp_blk, s.c_str()))
          {
            resolve_imports(&imp_blk, overrides);
          }
        }
      }
      if(!anyFileLoaded && !is_optional)
        EXCEPTION("No such files on directory <{}>. Please make import_optional or add files to directory", path);
      //EXCEPTION("WILD CARD INPUTS NOT SUPPORTED %s %s\n", src_folder.string().c_str(), path);
    } else {
      DataBlock imp_blk;
      // if it is a absolute or mounted path, then keep as it
      // if it is not either, then assume its an offset from the current directory
      auto combined_str = src_folder.string() + "/" + std::string(path);
      auto combined_path = fs::path(combined_str);
      auto path_ = (abs_path || mnt_path) ? path : combined_path;
      bool was_parsed = this->have_we_parsed(path_.string());
      if(was_parsed)
        return;
      if (load(imp_blk, path_.string().c_str())) {
        resolve_imports(&imp_blk, overrides);
      } else if (!is_optional) {
        EXCEPTION("Failed to load {}", path_.string());
      }
    }

  }

  /// attempts to resolve all the imports for a template blk.
  /// once those are resolved, it will initiate parsing of blk
  void resolve_imports(DataBlock *blk, ecs::TemplateDB *overrides) {

    const char *src_fn = blk->resolveFilename();
    if (src_fn) {
      fs::path path{src_fn};
      path = path.parent_path();
      const int importNid = blk->getNameId("import");
      const int importOptionalNid = blk->getNameId("import_optional");
      for (int i = 0; i < blk->paramCount(); i++) {
        const int paramNid = blk->getParamNameId(i);
        if (paramNid == importNid || paramNid == importOptionalNid) {
          const bool isOptional = paramNid == importOptionalNid;
          //std::cout << "resolving import: " << blk->getStr(i) << "\n";
          resolve_one_import(blk->getStr(i), path, isOptional, overrides);
        }
      }
    }

    //std::cout << "parsing file: " <<  src_fn << "\n";
    parse_templates_from_blk(blk, overrides);
  }

  // entry point
  void parse_blk(const char *path) {
    DataBlock blk{};
    ecs::TemplateDB overrides{};
    G_ASSERT(load(blk, path));
    resolve_imports(&blk, &overrides);
    ecs::g_ecs_data->getTemplateDB()->applyFrom(std::move(overrides));
  }
};

void parseTemplates() {
  auto file = file_mgr.getFile("templates/entities.blk");
  LoadContext ctx{};
  ctx.parse_blk("templates/entities.blk");
}