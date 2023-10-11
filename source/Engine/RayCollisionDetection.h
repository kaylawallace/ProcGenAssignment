#pragma once

#include "Triangle.h"

class RayCollisionDetection
{
public:
	RayCollisionDetection() noexcept(false);

	bool RayTriangleIntersect(
		const DirectX::SimpleMath::Vector3 &origin,
		const DirectX::SimpleMath::Vector3 &dir,
		const DirectX::SimpleMath::Vector3 &v0,
		const DirectX::SimpleMath::Vector3 &v1,
		const DirectX::SimpleMath::Vector3 &v2,
		float &t
	);

	bool CompareVectorsWithinRadius(DirectX::SimpleMath::Vector3 camPos, DirectX::SimpleMath::Vector3 pointOnTriangle, float radius);

	DirectX::SimpleMath::Vector3 BarycentricCoords(Triangle _triangle, DirectX::SimpleMath::Vector3 _currentPos);
	float FindWeight1(DirectX::SimpleMath::Vector3 _curPos, DirectX::SimpleMath::Vector3 p1, DirectX::SimpleMath::Vector3 p2, DirectX::SimpleMath::Vector3 p3);
	float FindWeight2(DirectX::SimpleMath::Vector3 _curPos, DirectX::SimpleMath::Vector3 p1, DirectX::SimpleMath::Vector3 p2, DirectX::SimpleMath::Vector3 p3);
	float FindWeight3(float weight1, float weight2);
};