
#ifndef MYEXTENSION_TEMPLATES_H
#define MYEXTENSION_TEMPLATES_H

#include "ecs/ComponentTypes.h"
#include "ecs/typesAndLimits.h"
#include "BitVector.h"
#include "ecs/componentsMap.h"
#include "ecs/archetypes.h"
#include <algorithm>

namespace ecs
{
  struct TransparentHash : std::hash<std::string_view> {
    using is_transparent = void;
  };
  struct TransparentEqual : std::equal_to<std::string_view> {
    using is_transparent = void;
  };

  class TemplateDB;
  struct InstantiatedTemplate;
  //template<typename T>
  struct ComponentTemplInfo
  {
    //static_assert(std::is_same<T, ComponentRef>::value_type ||  std::is_same<T, Component>::value_type);
    ecs::component_index_t comp_type_index;
    Component default_component; // holds the default data for this component.

    static bool sort(ComponentTemplInfo &ref1, ComponentTemplInfo &ref2)
    {
      return ref1.comp_type_index < ref2.comp_type_index;
    }
  };

  struct ComponentRefTemplInfo
  {

    ecs::component_index_t comp_type_index;
    ComponentRef default_component; // holds the default data for this component.
    ComponentRefTemplInfo()
    {
      comp_type_index = INVALID_COMPONENT_INDEX;
      default_component = ComponentRef(nullptr, 0, INVALID_COMPONENT_TYPE_INDEX, 0);
    }
    ComponentRefTemplInfo(component_index_t idx, ComponentRef default_)
    {
      comp_type_index = idx;
      default_component = default_;
    }

    explicit ComponentRefTemplInfo(ComponentTemplInfo const & ref)
    {
      comp_type_index = ref.comp_type_index;
      default_component = ref.default_component.getComponentRef();
    }

    ComponentRefTemplInfo& operator=(const ComponentTemplInfo& other) {
      comp_type_index = other.comp_type_index;
      default_component = other.default_component.getComponentRef();
      return *this;
    }
    ComponentRefTemplInfo(ComponentRefTemplInfo &&ref) noexcept = default;
    ComponentRefTemplInfo(ComponentRefTemplInfo &ref) = default;
    ComponentRefTemplInfo &operator=(ComponentRefTemplInfo &&ref) noexcept = default;
    ComponentRefTemplInfo &operator=(ComponentRefTemplInfo const &ref) = default;
    ~ComponentRefTemplInfo() = default;


    static bool sort(ComponentRefTemplInfo &ref1, ComponentRefTemplInfo &ref2)
    {
      return ref1.comp_type_index < ref2.comp_type_index;
    }
  };
  /// A Template represents all the components a particular Entity should have. it does not store a entity, simply describes one
  /// we are probably not going to include sanity checks cause that is what gaijin is for :)
  struct Template
  {
    Template(std::string_view n, std::vector<ComponentTemplInfo>&& comps, std::vector<template_t> &&parents)
        : name(n), components(std::move(comps)), parents(std::move(parents)) {
      for(const auto &comp : components)
      {
        if (comp.comp_type_index > max_component_index)
          max_component_index = comp.comp_type_index;
      }
    }
    // an invalid template has no name
    bool isInvalid()
    {
      return name.empty();
    }
    std::string &getName()
    {
      return this->name;
    }


    // Copy constructor
    Template(const Template& other)
        : name(other.name),
          components(other.components), // Components are copied
          parents(other.parents),       // Parents are copied
          max_component_index(other.max_component_index) {}

    // Copy assignment operator
    Template& operator=(const Template& other) {
      if (this == &other) return *this; // Guard against self-assignment
      name = other.name;
      components = other.components;
      parents = other.parents;
      max_component_index = other.max_component_index;
      return *this;
    }

    // Move constructor
    Template(Template&& other) noexcept
        : name(std::move(other.name)),
          components(std::move(other.components)),
          parents(std::move(other.parents)),
          max_component_index(other.max_component_index) {
      other.max_component_index = 0; // Reset max_component_index of the moved-from object
    }

    // Move assignment operator
    Template& operator=(Template&& other) noexcept {
      if (this == &other) return *this; // Guard against self-assignment
      name = std::move(other.name);
      components = std::move(other.components);
      parents = std::move(other.parents);
      max_component_index = other.max_component_index;
      other.max_component_index = 0; // Reset max_component_index of the moved-from object
      return *this;
    }
  protected:
    friend EntityManager;
    friend TemplateDB;
    friend InstantiatedTemplate;
    std::string name;
    std::vector<ComponentTemplInfo> components;
    std::vector<template_t> parents;
    component_type_t max_component_index = 0;
  };


  /// a compiled form of a template
  /// has a ref to all initialized components
  /// this will be used when an entity is created, as it has all the data efficently stored to init an entity
  /// TODO: add thread protection, inst templates are created on demand, so sequential replay reads can cause issues
  ///
  struct InstantiatedTemplate {
  protected:
    friend EntityManager; // this actually creates the entity
    enum Flags : uint8_t
    {
      WAS_INITIALIZED = 1 // data initialized by ComponentsInitializer or Replication
    };

    std::vector<ComponentRefTemplInfo> components;
    std::vector<component_index_t> component_indexes;
    //BitVector has_initializer{false};
    template_t parent = INVALID_TEMPLATE_INDEX;
    BitVector activated{false};

    // recurse the template and template parents of template p
    // populates
    void RecurseTemplates(template_t p, TemplateDB &db);



  public:
    InstantiatedTemplate() = default;

    InstantiatedTemplate(InstantiatedTemplate &ref) = default;
    InstantiatedTemplate &operator=(InstantiatedTemplate const &ref) = default;

    InstantiatedTemplate(InstantiatedTemplate &&ref) = default;
    InstantiatedTemplate &operator=(InstantiatedTemplate &&ref) = default;

    InstantiatedTemplate(template_t p);

    archetype_t InitArcheType(EntityManager *mgr);
  };

/// Represents a collection of templates

  class TemplateDB
  {
  public:
    Template * getTemplate(template_t t);
    InstantiatedTemplate * getInstTemplate(template_t t);
    Template * getTemplate(const std::string_view &t);
    Template *operator[] (const template_t t) { return getTemplate(t);}
    Template *operator[] (const std::string_view &t) { return getTemplate(t);}

    template_t AddTemplate(Template &&templ)
    {
      auto found = template_lookup.find(templ.name);
      if(found != template_lookup.end())
      {
        Template &t = this->templates[found->second];
        if(t.isInvalid()) // invalid template, lets move our new template into there instead
        {
          this->templates[found->second] = std::move(templ);
        }
        return found->second;
      }

      auto idx = templates.size();
      templates.push_back(std::move(templ));
      inst_templates.resize(templates.size());
      template_lookup.emplace(std::string(templates[idx].name), (template_t)idx);

      return (template_t)idx;
    }



    template_t ensureTemplate(std::string &p) {
      auto found = template_lookup.find(p);
      if(found != template_lookup.end())
        return found->second;
      Template temp{"", {}, {}};
      auto idx = templates.size();
      templates.push_back(std::move(temp));
      inst_templates.resize(templates.size());
      template_lookup.emplace(p, (template_t)idx);
      return (template_t)idx;
    }

    // ensures a template of name p will exist in lookup.
    // if the template doesnt exist, it creates a new, invalid one
    template_t ensureTemplate(const char *p) {
      auto sp = std::string(p);
      return ensureTemplate(sp);
    }

    //bool AddTemplate()
    void DebugPrint();
    void DebugPrintTemplate(const std::string& templ);
    template_t buildTemplateIdByName(const char *templ_name);
    void instantiateTemplate(template_t t);
    template_t getTemplateIdByName(std::string_view name);
    std::vector<Template> &getTemplates();

    void applyFrom(TemplateDB &&db);


  protected:

    void printTempl(const Template &t, int spacing, ComponentTypes &types, DataComponents &comps);
    friend EntityManager;
    std::vector<Template> templates;
    std::vector<InstantiatedTemplate> inst_templates;
    // std::string is to ensure name always exists
    // the TransparentHash and TransparentEqual are done to allow for indexing with a std::string
    std::unordered_map<std::string, template_t, TransparentHash, TransparentEqual> template_lookup; // maps template names to template_t. template_t is index into templates
  };
}



#endif //MYEXTENSION_TEMPLATES_H
