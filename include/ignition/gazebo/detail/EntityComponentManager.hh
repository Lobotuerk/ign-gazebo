/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#ifndef IGNITION_GAZEBO_DETAIL_ENTITYCOMPONENTMANAGER_HH_
#define IGNITION_GAZEBO_DETAIL_ENTITYCOMPONENTMANAGER_HH_

#include <map>
#include <set>
#include <utility>

#include "ignition/gazebo/EntityComponentManager.hh"

namespace ignition
{
namespace gazebo
{
//////////////////////////////////////////////////
template<typename ComponentTypeT>
ComponentTypeId EntityComponentManager::ComponentType()
{
  // Get a unique identifier to the component type
  return typeid(ComponentTypeT).hash_code();
}

//////////////////////////////////////////////////
template<typename ComponentTypeT>
ComponentKey EntityComponentManager::CreateComponent(const Entity _entity,
            const ComponentTypeT &_data)
{
  // Get a unique identifier to the component type
  const ComponentTypeId typeId = ComponentType<ComponentTypeT>();

  // Create the component storage if one does not exist for
  // the component type.
  if (!this->HasComponentType(typeId))
  {
    this->RegisterComponentType(typeId,
          new ComponentStorage<ComponentTypeT>());
  }

  return this->CreateComponentImplementation(_entity, typeId, &_data);
}

//////////////////////////////////////////////////
template<typename ComponentTypeT>
const ComponentTypeT *EntityComponentManager::Component(
    const Entity _entity) const
{
  // Get a unique identifier to the component type
  const ComponentTypeId typeId = ComponentType<ComponentTypeT>();

  return static_cast<const ComponentTypeT *>(
      this->ComponentImplementation(_entity, typeId));
}

//////////////////////////////////////////////////
template<typename ComponentTypeT>
ComponentTypeT *EntityComponentManager::Component(const Entity _entity)
{
  // Get a unique identifier to the component type
  const ComponentTypeId typeId = ComponentType<ComponentTypeT>();

  return static_cast<ComponentTypeT *>(
      this->ComponentImplementation(_entity, typeId));
}

//////////////////////////////////////////////////
template<typename ComponentTypeT>
const ComponentTypeT *EntityComponentManager::Component(
    const ComponentKey &_key) const
{
  return static_cast<const ComponentTypeT *>(
      this->ComponentImplementation(_key));
}

//////////////////////////////////////////////////
template<typename ComponentTypeT>
ComponentTypeT *EntityComponentManager::Component(const ComponentKey &_key)
{
  return static_cast<ComponentTypeT *>(
      this->ComponentImplementation(_key));
}

//////////////////////////////////////////////////
template<typename ComponentTypeT>
const ComponentTypeT *EntityComponentManager::First() const
{
  return static_cast<const ComponentTypeT *>(
      this->First(this->ComponentType<ComponentTypeT>()));
}

//////////////////////////////////////////////////
template<typename ComponentTypeT>
ComponentTypeT *EntityComponentManager::First()
{
  return static_cast<ComponentTypeT *>(
      this->First(this->ComponentType<ComponentTypeT>()));
}

//////////////////////////////////////////////////
template<typename ...ComponentTypeTs>
Entity EntityComponentManager::EntityByComponents(
    const ComponentTypeTs &..._desiredComponents) const
{
  // Get all entities which have components of the desired types
  const auto &view = this->FindView<ComponentTypeTs...>();

  // Iterate over entities
  Entity result{kNullEntity};
  for (const Entity entity : view.entities)
  {
    bool different{false};

    // Iterate over desired components, comparing each of them to the
    // equivalent component in the entity.
    ForEach([&](const auto &_desiredComponent)
    {
      auto entityComponent = this->Component<
          std::remove_cv_t<std::remove_reference_t<
              decltype(_desiredComponent)>>>(entity);

      if (*entityComponent != _desiredComponent)
      {
        different = true;
      }
    }, _desiredComponents...);

    if (!different)
    {
      result = entity;
      break;
    }
  }

  return result;
}

//////////////////////////////////////////////////
template <typename T>
struct EntityComponentManager::identity  // NOLINT
{
  using type = T;
};

//////////////////////////////////////////////////
template<typename ...ComponentTypeTs>
void EntityComponentManager::EachNoCache(typename identity<std::function<
    bool(const Entity &_entity, const ComponentTypeTs *...)>>::type _f) const
{
  for (const Entity &entity : this->Entities())
  {
    auto types = std::set<ComponentTypeId>{
        this->ComponentType<ComponentTypeTs>()...};

    if (this->EntityMatches(entity, types))
    {
      if (!_f(entity,
              this->Component<ComponentTypeTs>(entity)...))
      {
        break;
      }
    }
  }
}

//////////////////////////////////////////////////
template<typename ...ComponentTypeTs>
void EntityComponentManager::EachNoCache(typename identity<std::function<
    bool(const Entity &_entity, ComponentTypeTs *...)>>::type _f)
{
  for (const Entity &entity : this->Entities())
  {
    auto types = std::set<ComponentTypeId>{
        this->ComponentType<ComponentTypeTs>()...};

    if (this->EntityMatches(entity, types))
    {
      if (!_f(entity,
              this->Component<ComponentTypeTs>(entity)...))
      {
        break;
      }
    }
  }
}

//////////////////////////////////////////////////
template<typename ...ComponentTypeTs>
void EntityComponentManager::Each(typename identity<std::function<
    bool(const Entity &_entity, const ComponentTypeTs *...)>>::type _f) const
{
  // Get the view. This will create a new view if one does not already
  // exist.
  detail::View &view = this->FindView<ComponentTypeTs...>();

  // Iterate over the entities in the view, and invoke the callback
  // function.
  for (const Entity entity : view.entities)
  {
    if (!_f(entity, view.Component<ComponentTypeTs>(entity, this)...))
    {
      break;
    }
  }
}

//////////////////////////////////////////////////
template<typename ...ComponentTypeTs>
void EntityComponentManager::Each(typename identity<std::function<
    bool(const Entity &_entity, ComponentTypeTs *...)>>::type _f)
{
  // Get the view. This will create a new view if one does not already
  // exist.
  detail::View &view = this->FindView<ComponentTypeTs...>();

  // Iterate over the entities in the view, and invoke the callback
  // function.
  for (const Entity entity : view.entities)
  {
    if (!_f(entity, view.Component<ComponentTypeTs>(entity, this)...))
    {
      break;
    }
  }
}

//////////////////////////////////////////////////
template <class Function, class... ComponentTypeTs>
void EntityComponentManager::ForEach(Function _f,
    const ComponentTypeTs &... _components)
{
  (_f(_components), ...);
}

//////////////////////////////////////////////////
template <typename... ComponentTypeTs>
void EntityComponentManager::EachNew(typename identity<std::function<
    bool(const Entity &_entity, ComponentTypeTs *...)>>::type _f)
{
  // Get the view. This will create a new view if one does not already
  // exist.
  detail::View &view = this->FindView<ComponentTypeTs...>();

  // Iterate over the entities in the view and in the newly created
  // entities list, and invoke the callback
  // function.
  for (const Entity entity : view.newEntities)
  {
    if (!_f(entity, view.Component<ComponentTypeTs>(entity, this)...))
    {
      break;
    }
  }
}

//////////////////////////////////////////////////
template <typename... ComponentTypeTs>
void EntityComponentManager::EachNew(typename identity<std::function<
    bool(const Entity &_entity, const ComponentTypeTs *...)>>::type _f) const
{
  // Get the view. This will create a new view if one does not already
  // exist.
  detail::View &view = this->FindView<ComponentTypeTs...>();

  // Iterate over the entities in the view and in the newly created
  // entities list, and invoke the callback
  // function.
  for (const Entity entity : view.newEntities)
  {
    if (!_f(entity, view.Component<ComponentTypeTs>(entity, this)...))
    {
      break;
    }
  }
}

//////////////////////////////////////////////////
template<typename ...ComponentTypeTs>
void EntityComponentManager::EachErased(typename identity<std::function<
    bool(const Entity &_entity, const ComponentTypeTs *...)>>::type _f) const
{
  // Get the view. This will create a new view if one does not already
  // exist.
  detail::View &view = this->FindView<ComponentTypeTs...>();

  // Iterate over the entities in the view and in the newly created
  // entities list, and invoke the callback
  // function.
  for (const Entity entity : view.toEraseEntities)
  {
    if (!_f(entity, view.Component<ComponentTypeTs>(entity, this)...))
    {
      break;
    }
  }
}

//////////////////////////////////////////////////
template<typename FirstComponent,
         typename ...RemainingComponents,
         typename std::enable_if<
           sizeof...(RemainingComponents) == 0, int>::type>
void EntityComponentManager::AddComponentsToView(detail::View &_view,
    const Entity _entity) const
{
  const ComponentTypeId typeId = ComponentType<FirstComponent>();
  const ComponentId compId =
      this->EntityComponentIdFromType(_entity, typeId);
  if (compId >= 0)
  {
    // Add the component to the view.
    _view.AddComponent(_entity, typeId, compId);
  }
  else
  {
    ignerr << "Entity[" << _entity << "] has no component of type["
      << typeId << "]. This should never happen.\n";
  }
}

//////////////////////////////////////////////////
template<typename FirstComponent,
         typename ...RemainingComponents,
         typename std::enable_if<
           sizeof...(RemainingComponents) != 0, int>::type>
void EntityComponentManager::AddComponentsToView(detail::View &_view,
    const Entity _entity) const
{
  const ComponentTypeId typeId = ComponentType<FirstComponent>();
  const ComponentId compId =
      this->EntityComponentIdFromType(_entity, typeId);
  if (compId >= 0)
  {
    // Add the component to the view.
    _view.AddComponent(_entity, typeId, compId);
  }
  else
  {
    ignerr << "Entity[" << _entity << "] has no component of type["
      << typeId << "]. This should never happen.\n";
  }

  // Add the remaining components to the view.
  this->AddComponentsToView<RemainingComponents...>(_view, _entity);
}

//////////////////////////////////////////////////
template<typename ...ComponentTypeTs>
detail::View &EntityComponentManager::FindView() const
{
  auto types = std::set<ComponentTypeId>{
      this->ComponentType<ComponentTypeTs>()...};

  std::map<detail::ComponentTypeKey, detail::View>::iterator viewIter;

  // Find the view. If the view doesn't exist, then create a new view.
  if (!this->FindView(types, viewIter))
  {
    detail::View view;
    // Add all the entities that match the component types to the
    // view.
    for (const Entity &entity : this->Entities())
    {
      if (this->EntityMatches(entity, types))
      {
        view.AddEntity(entity, this->IsNewEntity(entity));
        // If there is a request to delete this entity, update the view as
        // well
        if (this->IsMarkedForErasure(entity))
        {
          view.AddEntityToErased(entity);
        }

        // Store pointers to all the components. This recursively adds
        // all the ComponentTypeTs that belong to the entity to the view.
        this->AddComponentsToView<ComponentTypeTs...>(view, entity);
      }
    }

    // Store the view.
    return this->AddView(types, std::move(view))->second;
  }

  return viewIter->second;
}
}
}

#endif