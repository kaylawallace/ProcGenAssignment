#pragma once

#include "ModelClass.h"
#include <DirectXCollision.h>
#include "StepTimer.h"
#include "Camera.h"

class Player
{
public:
	void Initialize();
	void Update();
	DirectX::SimpleMath::Vector3 GetPosition();
	DirectX::SimpleMath::Vector3 GetRotation();
	void SetPosition(DirectX::SimpleMath::Vector3 _position);
	void SetRotation(DirectX::SimpleMath::Vector3 _rotation);
	void SetRadius(float _radius);
	BoundingSphere GetCollider();
	void SetCoinsCollected(int _coins);
	int GetCoinsCollected();
	bool GetGrounded();
	void SetGrounded(bool _grounded);
	bool GetJumping();
	void SetJumping(bool _jumping);

private:
	DirectX::SimpleMath::Vector3 m_position;
	DirectX::SimpleMath::Vector3 m_rotation;
	float m_radius;
	BoundingSphere m_collider;
	int m_coinsCollected;
	bool m_grounded;
	bool m_jumping;
};

