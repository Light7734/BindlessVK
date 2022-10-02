#pragma once

#include "Components.hpp"
#include "Core/Base.hpp"

#include <entt/entt.hpp>
#include <glm/glm.hpp>


using Entity = entt::entity;

class Scene
{
public:
	Scene()
	{
	}
	~Scene()
{
        }

	void OnRender()
	{
		m_Registry.group(entt::get<StaticMeshComponent, TransformComponent>).each([&](StaticMeshComponent& pipelineComponent, TransformComponent& transformComp) {
			// renderer.
		});
	}

	inline Entity CreateEntity()
	{
		return m_Registry.create();
	}

	template<typename T, typename... Args>
	inline T& AddComponent(Entity entity, Args&&... args)
	{
		T& component = m_Registry.emplace<T>(entity, std::forward<Args>(args)...);
	}

	template<typename T>
	inline T& GetComponent(Entity entity)
	{
		return m_Registry.get<T>(entity);
	}

	template<typename T>
	inline bool HasComponent(Entity entity)
	{
		return m_Registry.any_of<T>(entity);
	}

	inline bool HasComponent(Entity entity)
	{
		return m_Registry.any_of<int>(entity);
	}

	template<typename T>
	inline size_t RemoveComponent(Entity entity)
	{
		return m_Registry.remove<T>(entity);
	}


private:
	entt::registry m_Registry;

	/*
        std::vector<OnEntityCreatedCallback> m_Callback;
    */
};
