#include "ecs/EntityManager.h" // Templates.h included here
#include "ecs/Templates.h"


namespace ecs {

  Template *TemplateDB::getTemplate(const template_t t) {
    if (t < templates.size())
      return &templates[t];
    return nullptr;
  }

  Template *TemplateDB::getTemplate(const std::string_view &t) {
    auto it = template_lookup.find(t);
    if (it != template_lookup.end())
      return getTemplate(it->second);
    return nullptr;
  }

  void printspaces(int count)
  {
    for(int i = 0; i < count; i++)
    {
      std::cout << " ";
    }
  }

  void TemplateDB::printTempl(const Template &t, int spacing, ComponentTypes &types, DataComponents &comps) {
    printspaces(spacing);
    std::cout << t.name.c_str() << ":\n";
    for(const auto &tid : t.parents)
    {
      printTempl(*this->getTemplate(tid), spacing+4, types, comps);
    }
    for(const auto &comp : t.components)
    {
      auto type_name = types.getComponentData(comp.default_component.getTypeId())->name;
      auto comp_name = comps.getName(comp.comp_type_index);
      printspaces(spacing+4);
      LOG("%s(%i); type: %s(%i)\n", comp_name.data(), comp.comp_type_index, type_name.data(), comp.default_component.getTypeId());
    }
  }

  void TemplateDB::DebugPrint() {
    for(const auto &t : this->templates)
    {
      printTempl(t, 0, *g_ecs_data->getComponentTypes(), *g_ecs_data->getDataComponents());
    }
  }

  void TemplateDB::DebugPrintTemplate(const std::string& templ) {
    auto t = this->getTemplate(templ);
    if(t)
      printTempl(*t, 0, *g_ecs_data->getComponentTypes(), *g_ecs_data->getDataComponents());
  }


  InstantiatedTemplate *TemplateDB::getInstTemplate(template_t t) {
    if(t < this->inst_templates.size())
      return &this->inst_templates[t];
    return nullptr;
  }

  void split_on_plus(const std::string& input, std::vector<std::string>& output) {
    size_t start = 0;
    size_t end = 0;
    while ((end = input.find('+', start)) != std::string::npos) {
      output.push_back(input.substr(start, end - start));
      start = end + 1;
    }
    // Add the last segment (or the whole string if '+' is not found)
    output.push_back(input.substr(start));
  }

  template_t TemplateDB::buildTemplateIdByName(const char *templ_name) {
    template_t id = this->getTemplateIdByName(templ_name);
    if(DAGOR_LIKELY(id != INVALID_TEMPLATE_INDEX))
      return id;
    std::vector<std::string> template_parts;
    std::vector<template_t> parents;
    auto combined_name = std::string(templ_name);
    split_on_plus(combined_name, template_parts);
    if(template_parts.size() == 1) // not a compound template and we couldnt find it earlier so bad
      EXCEPTION("Template %s not in database", template_parts[0].c_str());
    parents.reserve(template_parts.size());
    for (const auto &parts : template_parts)
    {
      const template_t it = this->getTemplateIdByName(parts);
      if(it == INVALID_TEMPLATE_INDEX)
      {
        EXCEPTION("Can't find template %s while building compound template '%s'", parts.c_str(), templ_name);
      }
      for(const template_t parent : parents)
      {
        if(it==parent)
        {
          EXCEPTION("Duplicate template %s found will build compound template '%s'", parts.c_str(), templ_name);
        }
      }
      parents.push_back(it);
    }
    // compound templates have no components, only parents
    return this->AddTemplate(std::move(Template{templ_name, {}, std::move(parents)}))  ;
  }


  void TemplateDB::instantiateTemplate(template_t t) {
    G_ASSERT(t<this->templates.size());
    this->inst_templates[t] = std::move(InstantiatedTemplate(t));
  }

  template_t TemplateDB::getTemplateIdByName(std::string_view name) {
    auto it = this->template_lookup.find(name);
    if(it != this->template_lookup.end())
      return it->second;
    return INVALID_TEMPLATE_INDEX;
  }

  std::vector<Template> &TemplateDB::getTemplates() {
    return this->templates;
  }

  void TemplateDB::applyFrom(TemplateDB &&db) {
    for(auto &&templ : db.templates) {
      auto iter = this->template_lookup.find(templ.name);
      if (iter == this->template_lookup.end()) {
        this->AddTemplate(std::move(templ));
        continue;
      }
      auto this_templ = this->getTemplate(iter->second);

      for(auto &parent : templ.parents)
        this_templ->parents.push_back(parent);
      for(auto &&other_comp : templ.components) {
        bool set = false;
        for(auto &comp : this_templ->components) {
          if(other_comp.comp_type_index == comp.comp_type_index) {
            comp.default_component.~Component();
            comp.default_component = std::move(other_comp.default_component);
            set = true;
            break;
          }
        }
        if(!set) {
          this_templ->components.push_back(std::move(other_comp));
        }
      }
    }
  }

  /*InstantiatedTemplate::InstantiatedTemplate(EntityId eid, template_t templ, ComponentsInitializer &&initializer) {
    Template * t = g_entity_mgr->getTemplateDB()->getTemplate(templ);
    components.resize(t->max_component_index);
    flags.resize(t->max_component_index);
    instantiated.resize(t->max_component_index);
    for (auto &comp: initializer) {
      instantiated.set(comp.cIndex);
      components[comp.cIndex] = std::move(comp.second);
      flags[comp.cIndex] = WAS_INITIALIZED;
    }
    for (auto &comp : t->components)
    {
      if(!instantiated.get(comp.comp_type_index)) // wasnt set by initializer
      {
        if(comp.default_component)
          components[comp.comp_type_index] = *comp.default_component; // copies
        else // no default component, must construct a new one
        {
          auto index = g_entity_mgr->getDataComponents()->getDataComponent(comp.comp_type_index)->componentIndex; // gets comptype index
          auto component_data = g_entity_mgr->getComponentTypes()->getComponentData(index);
          void * data = malloc(component_data->size); // mallocs needed initial data
          memset(data,0, component_data->size); // we dont care if its pod if we just always memset it
          if(need_constructor(component_data->flags)) // some types need to be constructed
            g_entity_mgr->getComponentTypes()->getCTM(index)->create(data, *g_entity_mgr, eid, index);
          components[comp.comp_type_index] = std::move(Component(data, component_data->hash, index, component_data->size)); // uses move ctor
        }

        instantiated.set(comp.comp_type_index, );
      }
    }
  }*/



  InstantiatedTemplate::InstantiatedTemplate(template_t p) {
    static ComponentRef eidCompoennt;
    if(eidCompoennt.getUserType() == 0)
    {
      auto idx = g_ecs_data->getComponentTypes()->findType(ECS_HASH("ecs::EntityId").hash);
      auto comp_data = g_ecs_data->getComponentTypes()->getComponentData(idx);
      eidCompoennt = ComponentRef(nullptr, comp_data->hash, idx, comp_data->size);
    }
    activated.resize(g_ecs_data->getDataComponents()->size(), false);
    components.emplace_back(0, eidCompoennt);
    this->RecurseTemplates(p, *g_ecs_data->getTemplateDB());
    // now that we have all our components, lets sort them. archetype construction needs these to be sorted
    std::sort(components.begin(), components.end(), ComponentRefTemplInfo::sort);
    this->component_indexes.resize(this->components.size());
    for(int i = 0; i < this->component_indexes.size(); i++)
    {
      this->component_indexes[i] = components[i].comp_type_index;
    }
    this->parent = p;
  }

  void InstantiatedTemplate::RecurseTemplates(template_t p, TemplateDB &db) {

    auto tmpl = db.getTemplate(p);
    for(auto &comp : tmpl->components)
    {
      if(!this->activated.test(comp.comp_type_index, true))
      {
        this->activated.set(comp.comp_type_index, true);
        this->components.emplace_back(comp);
      }
    }
    for(const auto &id : tmpl->parents)
    {
      RecurseTemplates(id, db);
    }
    //this->components.insert(components.end(), tmpl->components.begin(), tmpl->components.end());
  }

  archetype_t InstantiatedTemplate::InitArcheType(EntityManager *mgr) {
    return mgr->createArchetype(this);
  }
}


