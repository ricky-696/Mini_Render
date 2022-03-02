#include "Device.h"

// 擴充的繪製功能
void Device::drawTriangle(vec3 v1, vec3 v2, vec3 v3, UI32 c) {
    drawLine(v1.x, v1.y, v2.x, v2.y, c);
    drawLine(v2.x, v2.y, v3.x, v3.y, c);
    drawLine(v3.x, v3.y, v1.x, v1.y, c);
}
void Device::fillTriangle(vec3 v1, vec3 v2, vec3 v3, UI32 c) {
	if (v1.y > v2.y)std::swap(v1, v2);
	if (v2.y > v3.y)std::swap(v2, v3);
	if (v1.y > v2.y)std::swap(v1, v2);

	if (v2.y == v3.y)
	{
		fillBottomFlatTriangle(v1, v2, v3, c);
	}
	else if (v1.y == v2.y)
	{
		fillTopFlatTriangle(v1, v2, v3, c);
	}
	else
	{
		vec3 v4{ (v1.x + ((v2.y - v1.y) / (v3.y - v1.y)) * (v3.x - v1.x)), v2.y };
		fillBottomFlatTriangle(v1, v2, v4, c);
		fillTopFlatTriangle(v2, v4, v3, c);
	}
}