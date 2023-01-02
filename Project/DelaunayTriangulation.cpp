//---------------------------
// Includes
//---------------------------
#define _USE_MATH_DEFINES
#include "DelaunayTriangulation.h"
#include <cmath>

//---------------------------
// Member functions
//---------------------------
void DelaunayTriangulation::Triangulate(int screenSize, std::vector<DungeonRoom>& rooms)
{
	// Clear the used containers
	Clear();

	// Set up all data needed for the triangulation algorithm
	StartTriangulation(screenSize);

	// For each room
	for (const DungeonRoom& room : rooms)
	{
		// Add the center of the room to the triangulation
		AddPoint(room.GetPosition() + room.GetSize() / 2);
	}

	// Finish up the triangulation algorithm
	FinishTriangulation();
}

void DelaunayTriangulation::StartTriangulation(int screenSize)
{
	// Create super triangle vertices
	AddVertex({ 0,0 });
	AddVertex({ -500, 2500 });
	AddVertex({ 2500, -500 });

	// Create the super triangle
	AddTriangle(0, 1, 2);
}

void DelaunayTriangulation::AddPoint(const Vector2& point)
{
	// Add the center of the room as a vertex
	int newIndice{ AddVertex(point) };

	// Polygon formed by the intersecting triangles
	std::vector<int> intersectingPolygon{};

	// For each triangle (in reverse order so we can delete triangles safely)
	for (int i{ static_cast<int>(m_Triangles.size()) - 1 }; i >= 0; --i)
	{
		// Cast the index to a size_t
		const size_t index{ static_cast<size_t>(i) };

		// If the new point is not in a circumcircle of the triangle, continue to the next triangle
		if (!IsInsideCircumcircle(m_Triangles[index], newIndice)) continue;

		// If the intersecting polygon already has vertices
		if (intersectingPolygon.size() > 0)
		{
			// If the points are already in the intersecting polygon
			bool hasFirst{}, hasSecond{}, hasThird{};

			const size_t originalSize{ intersectingPolygon.size() };
			for (size_t polygonIdx{}; polygonIdx < originalSize; ++polygonIdx)
			{
				// If a vertex of the triangle is already in the intersecting polygon, set the corresponding boolean on true
				if (intersectingPolygon[polygonIdx] == m_Triangles[index].first)
				{
					hasFirst = true;
				}
				if (intersectingPolygon[polygonIdx] == m_Triangles[index].second)
				{
					hasSecond = true;
				}
				if (intersectingPolygon[polygonIdx] == m_Triangles[index].third)
				{
					hasThird = true;
				}
			}

			// Add the vertices of the triangle to the intersecting polygon if they are not yet in the polygon
			if (!hasFirst) intersectingPolygon.push_back(m_Triangles[index].first);
			if (!hasSecond) intersectingPolygon.push_back(m_Triangles[index].second);
			if (!hasThird) intersectingPolygon.push_back(m_Triangles[index].third);
		}
		else
		{
			// If the polygon does not have vertices yet, add the vertices of the triangle to the intersecting polygon
			intersectingPolygon.push_back(m_Triangles[index].first);
			intersectingPolygon.push_back(m_Triangles[index].second);
			intersectingPolygon.push_back(m_Triangles[index].third);
		}

		// Remove the current triangle
		RemoveTriangle(index);
	}

	// Sort the vertices of the polygon
	std::sort(
		intersectingPolygon.begin(),
		intersectingPolygon.end(),
		[&](int indice0, int indice1)
		{
			const Vector2 vector0{ m_Vertices[indice0] - point };
			const Vector2 vector1{ m_Vertices[indice1] - point };
			float angle0{ atan2f(static_cast<float>(vector0.y), static_cast<float>(vector0.x)) };
			float angle1{ atan2f(static_cast<float>(vector1.y), static_cast<float>(vector1.x)) };
			if (angle0 < 0) angle0 = angle0 + 2.0f * static_cast<float>(M_PI);
			if (angle1 < 0) angle1 = angle1 + 2.0f * static_cast<float>(M_PI);
			return angle0 < angle1;
		}
	);

	// For each edge in the intersecting polygon
	for (int i{}; i < intersectingPolygon.size(); ++i)
	{
		// Cast the index to a size_t
		const size_t index{ static_cast<size_t>(i) };

		// Create a triangle between the current edge and the new vertex
		AddTriangle(intersectingPolygon[index], intersectingPolygon[(index + 1) % intersectingPolygon.size()], newIndice);
	}
}

void DelaunayTriangulation::FinishTriangulation()
{
	// For each triangle
	for (int i{ static_cast<int>(m_Triangles.size()) - 1 }; i >= 0; --i)
	{
		// Cast the index to a size_t
		const size_t index{ static_cast<size_t>(i) };

		// For every vertex of the super triangle (index 0, 1 and 2)
		for (int indice{}; indice < 3; ++indice)
		{
			// If the triangle contains a vertex of the super triangle
			if (m_Triangles[index].first == indice ||
				m_Triangles[index].second == indice ||
				m_Triangles[index].third == indice)
			{
				// Remove this triangle
				RemoveTriangle(index);
				break;
			}
		}
	}
}

void DelaunayTriangulation::Clear()
{
	m_Triangles.clear();
	m_Vertices.clear();
}

bool DelaunayTriangulation::IsInsideCircumcircle(const Triangle& triangle, int indice)
{
	// Get the vertices of the current triangle
	const Vector2& v0{ m_Vertices[triangle.first] };
	const Vector2& v1{ m_Vertices[triangle.second] };
	const Vector2& v2{ m_Vertices[triangle.third] };
	// Get the current vertex to add
	const Vector2& vTest{ m_Vertices[indice] };

	// Calculate the slopes of the perpendicular lines of edges 01 and 02
	const float perpSlope0{ (v0.x - v1.x) / static_cast<float>(v1.y - v0.y + FLT_EPSILON) };
	const float perpSlope1{ (v0.x - v2.x) / static_cast<float>(v2.y - v0.y + FLT_EPSILON) };

	// Calculate the center of the edges
	const Vector2 edge01Center{ (v0 + v1) / 2 };
	const Vector2 edge02Center{ (v0 + v2) / 2 };

	// Calculate the intercept of the lines using the calculated slopes and the center of the edges
	const float intercept0{ edge01Center.y - perpSlope0 * edge01Center.x };
	const float intercept1{ edge02Center.y - perpSlope1 * edge02Center.x };

	// Calculate the intersection of the two perpendicular lines, this is the center of the circle
	int x{ static_cast<int>((intercept1 - intercept0) / (perpSlope0 - perpSlope1)) };
	int y{ static_cast<int>(perpSlope0 * x + intercept0) };

	// Calculate the radius of the circle
	const int r{ (x - v1.x) * (x - v1.x) + (y - v1.y) * (y - v1.y) };

	// Calculate the distance between the center and the new vertex
	const int rPoint{ (x - vTest.x) * (x - vTest.x) + (y - vTest.y) * (y - vTest.y) };

	// Return true if the new vertex is inside the circle
	return rPoint < r;
}
