#include "pch.h"
#include "Player.h"
#include <iostream>
#include "Camera.h"


void Player::Initialize()
{
	m_position.x = 0.f;
	m_position.y = 0.f;
	m_position.z = 0.f;

	m_rotation.x = 0.f;
	m_rotation.y = 0.f;
	m_rotation.z = 0.f;

	m_coinsCollected = 0;
	m_jumping = false;
}

void Player::Update()
{
	
}

DirectX::SimpleMath::Vector3 Player::GetPosition()
{
	return m_position;
}

DirectX::SimpleMath::Vector3 Player::GetRotation()
{
	return m_rotation;
}

void Player::SetPosition(DirectX::SimpleMath::Vector3 _position)
{
	m_position = _position;
	m_collider.Center = _position;
}

void Player::SetRotation(DirectX::SimpleMath::Vector3 _rotation)
{
	m_rotation = _rotation;
}

void Player::SetRadius(float _radius)
{
	m_collider.Radius = _radius;
}

BoundingSphere Player::GetCollider()
{
	return m_collider;
}

void Player::SetCoinsCollected(int _coins)
{
	m_coinsCollected = _coins;
}

int Player::GetCoinsCollected()
{
	return m_coinsCollected;
}

bool Player::GetGrounded()
{
	return m_grounded;
}

void Player::SetGrounded(bool _grounded)
{
	m_grounded = _grounded;
}

bool Player::GetJumping()
{
	return m_jumping;
}

void Player::SetJumping(bool _jumping)
{
	m_jumping = _jumping;
}