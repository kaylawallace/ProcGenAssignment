#include "pch.h"
#include "RayCollisionDetection.h"
#include <iostream>


using namespace DirectX::SimpleMath;

RayCollisionDetection::RayCollisionDetection() noexcept(false)
{

}

/*
* This method is from https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution
*/
bool RayCollisionDetection::RayTriangleIntersect(const Vector3 &origin, const Vector3 &dir, const Vector3 &v0, const Vector3 &v1, const Vector3 &v2, float &t)
{
	// Compute plane's normal 
	Vector3 v0v1 = v1 - v0;
	Vector3 v0v2 = v2 - v0;

	Vector3 norm = v0v1.Cross(v0v2);
	float area2 = norm.Length();

	// Check if ray and plane are parallel
	float NdotRayDir = norm.Dot(dir);

	// Almost zero
	if (fabs(NdotRayDir) < std::numeric_limits<float>::epsilon())	
	{
		return false; // Parallel so no intersection
	}

	float d = -norm.Dot(v0);

	t = -(norm.Dot(origin) + d) / NdotRayDir;
	

	// Check if triangle is in behind ray 
	if (t < 0)
	{
		return false;	// Triangle is behind 
	}

	// Intersection point P
	Vector3 P = origin + t * dir;

	// Inside-outside test 
	Vector3 C;

	Vector3 edge0 = v1 - v0;
	Vector3 vp0 = P - v0;
	C = edge0.Cross(vp0);

	if (norm.Dot(C) < 0)
	{
		return false;	// P is on right side 
	}

	Vector3 edge1 = v2 - v1;
	Vector3 vp1 = P - v1;
	C = edge1.Cross(vp1);

	if (norm.Dot(C) < 0)
	{
		return false; 
	}

	Vector3 edge2 = v0 - v2;
	Vector3 vp2 = P - v2;
	C = edge2.Cross(vp2);

	if (norm.Dot(C) < 0)
	{
		return false; 
	}

	return true;
}


/*
* Method to check for collisions within a radius
*/
bool RayCollisionDetection::CompareVectorsWithinRadius(Vector3 camPos, Vector3 pointOnTriangle, float radius) 
{
	bool equal = true;
	equal = equal && (abs(camPos.y - pointOnTriangle.y) < radius);
	return equal;
}

/*
* Derived from https://stackoverflow.com/questions/25385361/point-within-a-triangle-barycentric-co-ordinates and 
* https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
*/
Vector3 RayCollisionDetection::BarycentricCoords(Triangle _triangle, Vector3 _currentPos) 
{
	float weight1 = FindWeight1(_currentPos, _triangle.positions[0], _triangle.positions[1], _triangle.positions[2]);
	float weight2 = FindWeight2(_currentPos, _triangle.positions[0], _triangle.positions[1], _triangle.positions[2]);
	float weight3 = FindWeight3(weight1, weight2);
	float yVal = (weight1 * _triangle.positions[0].y) + (weight2 * _triangle.positions[1].y) + (weight3 * _triangle.positions[2].y);
	return DirectX::SimpleMath::Vector3(0, yVal, 0);
}

float RayCollisionDetection::FindWeight1(DirectX::SimpleMath::Vector3 _curPos, DirectX::SimpleMath::Vector3 p1, DirectX::SimpleMath::Vector3 p2, DirectX::SimpleMath::Vector3 p3) 
{
	float num = ((p2.z - p3.z) * (_curPos.x - p3.x)) + ((p3.x - p2.x) * (_curPos.z - p3.z));
	float denom = ((p2.z - p3.z) * (p1.x - p3.x)) + ((p3.x - p2.x) * (p1.z - p3.z));
	return (num / denom);
}

float RayCollisionDetection::FindWeight2(DirectX::SimpleMath::Vector3 _curPos, DirectX::SimpleMath::Vector3 p1, DirectX::SimpleMath::Vector3 p2, DirectX::SimpleMath::Vector3 p3)
{
	float num = ((p3.z - p1.z) * (_curPos.x - p3.x)) + ((p1.x - p3.x) * (_curPos.z - p3.z));
	float denom = ((p2.z - p3.z) * (p1.x - p3.x)) + ((p3.x - p2.x) * (p1.z - p3.z));
	return (num / denom);
}

float RayCollisionDetection::FindWeight3(float weight1, float weight2) 
{
	return 1 - weight2 - weight1;
}