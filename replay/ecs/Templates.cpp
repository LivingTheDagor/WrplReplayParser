#include "ecs/EntityManager.h" // Templates.h included here
#include "ecs/Templates.h"


namespace ecs {

  Template *TemplateDB::getTemplate(const template_t t) {
    std::shared_lock lk(this->template_mtx);
    if (t < templates.size())
      return &templates[t];
    return nullptr;
  }

  Template * TemplateDB::getTemplateNoLock(template_t t) {
    if (t < templates.size())
      return &templates[t];
    return nullptr;
  }

  Template *TemplateDB::getTemplate(const std::string_view &t) {
    std::shared_lock lk(this->template_mtx);
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
    std::shared_lock lk(this->template_mtx);
    std::string spaces_b(spacing, ' ');
    LOG("{}{}:", spaces_b, t.name);
    for(const auto &tid : t.parents)
    {
      printTempl(*this->getTemplate(tid), spacing+4, types, comps);
    }
    for(const auto &comp : t.components)
    {
      auto type_name = types.getComponentData(comp.default_component.getTypeId())->name;
      auto comp_name = comps.getName(comp.comp_type_index);
      std::string spaces(spacing+4, ' ');
      LOG("{}{}({}); type: {}({})", spaces, comp_name, comp.comp_type_index, type_name, comp.default_component.getTypeId());
    }
  }

  void TemplateDB::DebugPrint() {
    std::shared_lock lk(this->template_mtx);
    for(const auto &t : this->templates)
    {
      printTempl(t, 0, *g_ecs_data->getComponentTypes(), *g_ecs_data->getDataComponents());
    }
  }

  void TemplateDB::DebugPrintTemplate(const std::string& templ) {

    std::shared_lock lk(this->template_mtx);
    auto t = this->getTemplate(templ);
    if(t)
      printTempl(*t, 0, *g_ecs_data->getComponentTypes(), *g_ecs_data->getDataComponents());
  }


  InstantiatedTemplate *TemplateDB::getInstTemplate(template_t t) {
    std::shared_lock lk(this->template_mtx);
    if(t < this->inst_templates.size())
      return this->inst_templates[t];
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
    {
      std::shared_lock lk(this->template_mtx);
      template_t id = this->getTemplateIdByNameNoLock(templ_name);
      if(DAGOR_LIKELY(id != INVALID_TEMPLATE_INDEX))
        return id;
    }
    std::vector<std::string> template_parts;
    std::vector<template_t> parents;
    auto combined_name = std::string(templ_name);
    split_on_plus(combined_name, template_parts);
    if(template_parts.size() == 1) // not a compound template and we couldnt find it earlier so bad
        EXCEPTION("Template {} not in database", template_parts[0].c_str());
    parents.reserve(template_parts.size());
    for (const auto &parts : template_parts)
    {
      const template_t it = this->getTemplateIdByName(parts);
      if(it == INVALID_TEMPLATE_INDEX)
      {
        EXCEPTION("Can't find template {} while building compound template '{}'", parts.c_str(), templ_name);
      }
      for(const template_t parent : parents)
      {
        if(it==parent)
        {
          EXCEPTION("Duplicate template {} found while build compound template '{}'", parts.c_str(), templ_name);
        }
      }
      parents.push_back(it);
    }
    // compound templates have no components, only parents
    {;
      std::unique_lock lk(this->template_mtx);
      template_t id = this->getTemplateIdByNameNoLock(templ_name);
      if(DAGOR_UNLIKELY(id != INVALID_TEMPLATE_INDEX)) {
        return id;
      }
      ZoneScoped;
      return this->AddTemplate(Template{templ_name, {}, std::move(parents)});
    }
  }


  void TemplateDB::instantiateTemplate(template_t t) {
    G_ASSERT(t<this->templates.size());
    {
      std::shared_lock lk(this->template_mtx);
      if (this->inst_templates[t]) {
        return; // already instantated
      }
    }
    {
      std::unique_lock lk(template_mtx);
      if (this->inst_templates[t]) {
        return; // already instantated
      }
      ZoneScoped;
      inst_templates[t] = new InstantiatedTemplate(t);
    }
  }

  template_t TemplateDB::getTemplateIdByName(std::string_view name) {
    std::shared_lock lk(this->template_mtx);
    auto it = this->template_lookup.find(name);
    if(it != this->template_lookup.end())
      return it->second;
    return INVALID_TEMPLATE_INDEX;
  }


  template_t TemplateDB::getTemplateIdByNameNoLock(std::string_view name) {
    auto it = this->template_lookup.find(name);
    if(it != this->template_lookup.end())
      return it->second;
    return INVALID_TEMPLATE_INDEX;
  }

  auto &TemplateDB::getTemplates() {
    return this->templates;
  }

  void TemplateDB::applyFrom(TemplateDB &&db) {
    //std::unique_lock lk(this->mtx); // lock not needed as only called during init
    for(auto &&templ : db.templates) {
      auto iter = this->template_lookup.find(templ.name);
      if (iter == this->template_lookup.end()) {
        this->AddTemplate(std::move(templ));
        continue;
      }
      auto this_templ = this->getTemplateNoLock(iter->second);

      for(auto &parent : templ.parents)
        this_templ->parents.push_back(parent);
      for(auto &&other_comp : templ.components) {
        bool set = false;
        for(auto &comp : this_templ->components) {
          if(other_comp.comp_type_index == comp.comp_type_index) {
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

  InstantiatedTemplate::InstantiatedTemplate(template_t p) {
    static ComponentRef eidComponent;
    if(eidComponent.getUserType() == 0)
    {
      auto idx = g_ecs_data->getComponentTypes()->findType(ECS_HASH("ecs::EntityId").hash);
      auto comp_data = g_ecs_data->getComponentTypes()->getComponentData(idx);
      eidComponent = ComponentRef(nullptr, comp_data->hash, idx, comp_data->size);
    }
    activated.resize(g_ecs_data->getDataComponents()->size(), false);
    components.emplace_back(0, eidComponent);
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

    auto tmpl = db.getTemplateNoLock(p);
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

}


