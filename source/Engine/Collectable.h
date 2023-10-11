#pragma once

#include "pch.h"
#include <DirectXCollision.h>
#include "modelclass.h"

class Collectable
{
public:
	Collectable();
	DirectX::SimpleMath::Vector3 GetPosition();
	void SetPosition(DirectX::SimpleMath::Vector3 _position);
	bool GetCollected();
	void SetCollected(bool _collected);
	void SetRadius(float _radius);
	BoundingSphere GetCollider();

private:
	bool m_collected;
	float m_colliderRadius; 
	DirectX::SimpleMath::Vector3 m_colliderCenter;
	DirectX::SimpleMath::Vector3 m_position;
	DirectX::BoundingSphere m_collider;
};