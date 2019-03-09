#ifndef AICS_AGENT_H
#define AICS_AGENT_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Eigen/Dense"
#include "Eigen/Geometry"

/*
*	Agent type flags for visualization
*	defined starting at 1000 to allow simPkgs to start identifying agent ids from 0 w/o issue
*	only 'first-class' visualization objects need to be defined here
*/
enum kVisType{
	vis_type_default = 1000,
	vis_type_fiber = 1001
};

namespace aics {
namespace agentsim {

class Agent
{
public:
	Agent();

	void SetLocation(Eigen::Vector3d newLocation);
	void SetRotation(Eigen::Vector3d newRotation);
	void SetInteractionDistance(float newDistance) { m_interaction_distance = newDistance; }
	void SetCollisionRadius(float newRadius) { m_collision_radius = newRadius; }
	void SetMass(float mass) { m_mass = mass; }
	void SetDiffusionCoefficient(float dc) { m_diffusion_coefficient = dc; }
	void SetName(std::string name) { m_agentName = name; }
	void SetState(std::string state) { m_agentState = state; }
	void SetTypeID(unsigned int typeID) { m_typeId = typeID; }
	void SetInverseChildRotationMatrix(Eigen::Matrix3d m) { m_inverseChildRotation = m; }
	void SetRotationFromChildren(Eigen::Matrix3d rm)
	{
			m_rotation = (m_inverseChildRotation * rm).eulerAngles(0, 1, 2);
	}

	const Eigen::Matrix4d GetGlobalTransform();
	const Eigen::Matrix4d GetTransform();
	const Eigen::Vector3d GetLocation() const { return m_location; }
	const Eigen::Vector3d GetRotation() const { return m_rotation; }
	const float GetInteractionDistance() { return m_interaction_distance; }
	const float GetCollisionRadius() { return m_collision_radius; }
	const float GetMass() { return m_mass; }
	const float GetDiffusionCoefficient() { return m_diffusion_coefficient; }
	const std::string GetName() const { return m_agentName; }
	const std::string GetState() const { return m_agentState; }
	const std::string GetID() { return m_agentID; }
	const unsigned int GetTypeID() { return m_typeId; }

	bool AddBoundPartner(std::shared_ptr<Agent> other);
	bool AddChildAgent(std::shared_ptr<Agent> other);

	std::shared_ptr<Agent> GetBoundPartner(std::size_t index);
	std::shared_ptr<Agent> GetChildAgent(std::size_t index);
	std::size_t GetNumChildAgents() { return m_childAgents.size(); }

	bool CanInteractWith(const Agent& other);
	bool IsCollidingWith(const Agent& other);

	void AddTag(std::string tag);
	const bool HasTag(std::string tag);

	void SetVisibility(bool visibility) { this->m_visibility = visibility; }
	const bool IsVisible() { return this->m_visibility; }

	void AddSubPoint(Eigen::Vector3d newPoint) { m_subPoints.push_back(newPoint); }
	void UpdateSubPoint(std::size_t index, Eigen::Vector3d updatedPoint)
	{
		if(index < 0 || index > this->m_subPoints.size()) return;
		if(index == this->m_subPoints.size())
		{
			AddSubPoint(updatedPoint);
		}

		this->m_subPoints[index] = updatedPoint;
	}
	bool HasSubPoints() { return this->m_subPoints.size() > 0; }
	std::size_t GetNumSubPoints() { return this->m_subPoints.size(); }
	Eigen::Vector3d GetSubPoint(std::size_t index) { return this->m_subPoints[index]; }

	kVisType GetVisType() { return this->m_visType; }
	void SetVisType(kVisType newType) { this->m_visType = newType; }

	const void PrintDbg() const;

private:
	void UpdateParentTransform(Eigen::Matrix4d parentTransform);
	Eigen::Vector3d m_location;
	Eigen::Vector3d m_rotation;
	Eigen::Affine3d m_localLocation;
	Eigen::Affine3d m_localRotation;
	Eigen::Matrix4d m_parentTransform;

	// If the children of this agent are considered to form a basis,
	//  this is the rotation matrix that is the inverse of the 'unrotated'
	//  child basis
	Eigen::Matrix3d m_inverseChildRotation;

	float m_diffusion_coefficient = 5.0f;
	float m_mass = 1.0f;
	float m_interaction_distance = 100.f;
	float m_collision_radius = 50.f;
	unsigned int m_typeId = 0;
	std::string m_agentID = "";
	std::string m_agentName = "";
	std::string m_agentState = "";
	bool m_visibility = true;

	std::vector<std::string> m_tags;
	std::vector<std::shared_ptr<Agent>> m_boundPartners;
	std::vector<std::shared_ptr<Agent>> m_childAgents;

	std::vector<Eigen::Vector3d> m_subPoints;
	kVisType m_visType = kVisType::vis_type_default;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_AGENT_H
