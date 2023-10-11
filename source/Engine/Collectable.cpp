#include "pch.h"
#include "Collectable.h"

Collectable::Collectable()
{
	// Default coin values 
	m_collected = false;
	m_collider.Center = DirectX::SimpleMath::Vector3::Zero;
	m_collider.Radius = 1;
}

DirectX::SimpleMath::Vector3 Collectable::GetPosition()
{
	return m_position;
}

void Collectable::SetPosition(DirectX::SimpleMath::Vector3 _position)
{
	m_position = _position;
	m_collider.Center = _position;
}

bool Collectable::GetCollected()
{
	return m_collected;
}

void Collectable::SetCollected(bool _collected)
{
	m_collected = _collected;
}

void Collectable::SetRadius(float _radius)
{
	m_collider.Radius = _radius;
}

BoundingSphere Collectable::GetCollider()
{
	return m_collider;
}