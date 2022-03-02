#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include "geometry.h"
#include "tool.h"
#include "screen.h"
#include "device.h"
#include "model.h"


using namespace std;
//-----------------------------------------
void onLoad();
void gameMain();
void update(float deltatime);
void render();
//-----------------------------------------
const int width = 800, height = 800;
Device device;

//read model

struct mesh
{
	vector<triangle> tris;

	bool LoadFromObjectFile(string sFilename)
	{
		ifstream f(sFilename);
		if (!f.is_open())
			return false;

		// Local cache of verts
		vector<vec3> verts;

		while (!f.eof())
		{
			char line[128];
			f.getline(line, 128);

			strstream s;
			s << line;

			char junk;

			if (line[0] == 'v')
			{
				vec3 v;
				float temp = 1.0f;
				s >> junk >> v.x >> v.y >> v.z >> temp;
				verts.push_back(v);
			}

			if (line[0] == 'f')
			{
				int f[3];
				s >> junk >> f[0] >> f[1] >> f[2];
				tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
			}
		}

		return true;
	}

};

mesh meshCube;

vec3 vCamera;
vec3 vLookDir;
float fYaw;
mat4x4 matProj = {};
float fTheta = 0.0f;
int showMode = 0, showClipping = 0;

int main(void) {

	const TCHAR* title = _T("Win32");

	if (screen_init(width, height, title))
		return -1;

	device.init(width, height, screen_fb);
	device.background = 0x0;	 
	
	//project matrix create
	matProj = Matrix_MakeProjection(90.0f, (float)device.height / (float)device.width, 0.1f, 1000.0f);
	meshCube.LoadFromObjectFile("mountains.obj");
	//meshCube.LoadFromObjectFile("axis.obj");	
	//meshCube.LoadFromObjectFile("teapot.obj");
	//meshCube.LoadFromObjectFile("videoship.obj");
	//meshCube.LoadFromObjectFile("cube.obj");

	while (screen_exit == 0 && screen_keys[VK_ESCAPE] == 0) {
		screen_dispatch(); // event

		gameMain();

		screen_update();// swow framebuffer to screen
		Sleep(1);
	}
	return 0;
}

void onLoad() {

}
void gameMain() {
	fpsCounting();
	swprintf(strBuffer, 100,
			 L"Render ver0.1, %dx%d, FPS:%4d, dt:%2dms",
			 device.width, device.height, fps, dt);
	SetWindowText(screen_handle, strBuffer);
	update(dt / 1000.0f);
	render();
}

void update(float deltatime) {
	if (screen_keys['R']) {
		vCamera = { 0,0,0 };
		fYaw = 0;
	}
	if (screen_keys[VK_UP]) {
		vCamera.y += 10.0f * dt / 1000;
	}
	if (screen_keys[VK_DOWN]) {
		vCamera.y -= 10.0f * dt / 1000;
	}
	if (screen_keys[VK_RIGHT]) {
		//vCamera.x += 10.0f * dt / 1000;
		// get vLookDir right normal vec
		vec3 vRight = { vLookDir.z,0,-vLookDir.x };
		vec3 newMove = Vector_Mul(vRight, 10.0 * dt / 1000);
		vCamera = Vector_Add(vCamera, newMove);

	}
	if (screen_keys[VK_LEFT]) {
		//vCamera.x -= 10.0f * dt / 1000;
		// get vLookDir left normal vec
		vec3 vLeft = { -vLookDir.z,0,vLookDir.x };
		vec3 newMove = Vector_Mul(vLeft, 10.0 * dt / 1000);
		vCamera = Vector_Add(vCamera, newMove);
	}

	vec3 vForward = Vector_Mul(vLookDir, 10.0 * dt / 1000);

	if (screen_keys['W']) {
		vCamera = Vector_Add(vCamera, vForward);
	}
	if (screen_keys['S']) {
		vCamera = Vector_Sub(vCamera, vForward);
	}
	if (screen_keys['A']) {
		fYaw -= 1.0f * dt / 1000;
	}
	if (screen_keys['D']) {
		fYaw += 1.0f * dt / 1000;
	}
	//printf("Camera: %.3f, %.3f, %.3f  RotYa:%.3f\n", vCamera.x, vCamera.y, vCamera.z, fYaw * 180.0f / 3.14f);
}

void render() {
	device.clear();
	mat4x4 matRotZ, matRotX,matRotY, matScale;
	//fTheta += 0.01f;
	// Rotation Z
	matRotZ = Matrix_MakeRotationZ(fTheta);

	// Rotation X
	matRotX = Matrix_MakeRotationX(fTheta);

	// Rotation y
	matRotY = Matrix_MakeRotationY(fYaw);

	mat4x4 matTrans; // translation
	matTrans = Matrix_MakeTranslation(0.0, 0.0f, 5.0f);

	// world matrix <對著整個世界做旋轉>
	mat4x4 matWorld;
	matWorld = Matrix_MakeIdentity(); //init
	matWorld = Matrix_MultiplyMatrix(matRotZ, matWorld); // Transform by rotation
	matWorld = Matrix_MultiplyMatrix(matRotX, matWorld); // Transform by rotation
	matWorld = Matrix_MultiplyMatrix(matTrans, matWorld); // Transform by translation;
	// 先旋轉再平移，由pos開始往外乘，跟巢狀func一樣:T*(Rx*(Rz* pos)))，(AB)C=A(BC)

	//--------------------------------camera----------------------------------------
	vec3 vUp = { 0,1,0 };	//天空 y軸
	vec3 vTarget = { 0,0,1 }; //一開始設定目標在z軸
	mat4x4 matCameraRot = Matrix_MakeRotationY(fYaw);//y的旋轉matrix
	vLookDir = Matrix_MultiplyVector(matCameraRot, vTarget);//看的方向
	vTarget = Vector_Add(vCamera,vLookDir);//把方向加進camera
	
	mat4x4 matCamera = Matrix_PointAt(vCamera,vTarget,vUp);
	mat4x4 matView = Matrix_QuickInverse(matCamera);


	// Store triagles for rastering later
	vector<triangle> vecTrianglesToRaster;

	for (auto tri : meshCube.tris) {
		triangle triProjected, triTransformed, triViewed;
		//---------------------------let tri into the world----------------------------------------
		triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
		triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
		triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

		//判斷正反面(法向量)
		vec3 normal, line1, line2;
		line1 = Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
		line2 = Vector_Sub(triTransformed.p[2], triTransformed.p[0]);
		//法向量(normal)
		normal = Vector_CrossProduct(line1, line2);
		// normalise = 求出法向量長度之後求單位向量
		normal = Vector_Normalise(normal);
		//扣掉camera的距離
		vec3 vCameraRay = Vector_Sub(triTransformed.p[0], vCamera);

		if (Vector_DotProduct(normal, vCameraRay) < 0.0f) { //x dot y dot z
			//light
			vec3 light_direction = { 1.0f, 1.0f, 1.0f };
			light_direction = Vector_Normalise(light_direction);

			//dp=光與面法向量的夾角
			float dp = max(0.1f, Vector_DotProduct(light_direction, normal));
			UI32 col = 255 * dp;
			UI32 c1 = col | col<< 8 | col << 16;
			//-------------------------------------------------------------------------------------------
			
			//convert world space to view space
			triViewed.p[0] = Matrix_MultiplyVector(matView, triTransformed.p[0]);
			triViewed.p[1] = Matrix_MultiplyVector(matView, triTransformed.p[1]);
			triViewed.p[2] = Matrix_MultiplyVector(matView, triTransformed.p[2]);
			
			//clip tri
			int nClippedTriangles = 0;
			triangle clipped[2];
			nClippedTriangles = Triangle_ClipAgainstPlane({ 0.0f,0.0f,0.1f }, { 0.0f,0.0f,0.1f }, triViewed, clipped[0], clipped[1],false);
			
			for (int n = 0; n < nClippedTriangles; n++)
			{

			//matproj=投影矩陣
			triProjected.p[0] = Matrix_MultiplyVector(matProj, clipped[n].p[0]);
			triProjected.p[1] = Matrix_MultiplyVector(matProj, clipped[n].p[1]);
			triProjected.p[2] = Matrix_MultiplyVector(matProj, clipped[n].p[2]);
			//---------------homogeneous coordinate-----------------------------
			triProjected.p[0] = Vector_Div(triProjected.p[0], triProjected.p[0].w);
			triProjected.p[1] = Vector_Div(triProjected.p[1], triProjected.p[1].w);
			triProjected.p[2] = Vector_Div(triProjected.p[2], triProjected.p[2].w);
			
			// sacle into screen(平移--->縮放)
			triProjected.p[0].y *= -1.0f;// y軸相反
			triProjected.p[1].y *= -1.0f;
			triProjected.p[2].y *= -1.0f;
			triProjected.p[0].x += 1.0f; triProjected.p[0].y += 1.0f;//世界+1
			triProjected.p[1].x += 1.0f; triProjected.p[1].y += 1.0f;
			triProjected.p[2].x += 1.0f; triProjected.p[2].y += 1.0f;
			triProjected.p[0].x *= 0.5f * (float)width;  //縮放世界
			triProjected.p[0].y *= 0.5f * (float)height;
			triProjected.p[1].x *= 0.5f * (float)width;
			triProjected.p[1].y *= 0.5f * (float)height;
			triProjected.p[2].x *= 0.5f * (float)width;
			triProjected.p[2].y *= 0.5f * (float)height;
			triProjected.col = c1;
			// Store triangle for sorting
			vecTrianglesToRaster.push_back(triProjected);
			//device.fillTriangle(triProjected.p[0], triProjected.p[1], triProjected.p[2], c1);
			//device.drawTriangle(triProjected.p[0], triProjected.p[1], triProjected.p[2], 0x000000);
			}
		}
	}

	// Sort triangles from back to front
	// 在使用Z-buffer之前的簡易作法
	sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle& t1, triangle& t2)
		{
			float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
			float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
			return z1 > z2;
		});
	for (auto& triToRaster : vecTrianglesToRaster)
	{
		// Clip triangles against all four screen edges, this could yield
		// a bunch of triangles, so create a queue that we traverse to 
		//  ensure we only test new triangles generated against planes
		triangle clipped[2];
		list<triangle> listTriangles;

		// Add initial triangle
		listTriangles.push_back(triToRaster);
		int nNewTriangles = 1;

		// 螢幕上下左右四個面
		for (int p = 0; p < 4; p++)
		{
			int nTrisToAdd = 0;
			while (nNewTriangles > 0)
			{
				// Take triangle from front of queue
				triangle test = listTriangles.front();
				listTriangles.pop_front();
				nNewTriangles--;

				// Clip it against a plane. We only need to test each 
				// subsequent plane, against subsequent new triangles
				// as all triangles after a plane clip are guaranteed
				// to lie on the inside of the plane. I like how this
				// comment is almost completely and utterly justified
				switch (p)
				{
				case 0:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1], showClipping); break;
				case 1:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, (float)device.height - 1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1], showClipping); break;
				case 2:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1], showClipping); break;
				case 3:	nTrisToAdd = Triangle_ClipAgainstPlane({ (float)device.width - 1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1], showClipping); break;
				}
				// Clipping may yield a variable number of triangles, so
				// add these new ones to the back of the queue for subsequent
				// clipping against next planes
				for (int w = 0; w < nTrisToAdd; w++)
					listTriangles.push_back(clipped[w]);
			}
			nNewTriangles = listTriangles.size();
		}


		// Draw the transformed, viewed, clipped, projected, sorted, clipped triangles
		for (auto& tri : listTriangles)
		{
			// Rasterize triangle
			switch (showMode)
			{
			case 0:
				device.fillTriangle(tri.p[0], tri.p[1], tri.p[2], tri.col);
				break;
			case 1:
				//drawTriangle(&device, tri.p[0], tri.p[1], tri.p[2], 0);
				device.drawLine( tri.p[0].x, tri.p[0].y, tri.p[1].x, tri.p[1].y, 0xff0000);
				device.drawLine( tri.p[1].x, tri.p[1].y, tri.p[2].x, tri.p[2].y, 0x00ff00);
				device.drawLine( tri.p[2].x, tri.p[2].y, tri.p[0].x, tri.p[0].y, 0x0000ff);
				break;
			case 2: {
				device.fillTriangle(tri.p[0], tri.p[1], tri.p[2], tri.col);
				device.drawTriangle(tri.p[0], tri.p[1], tri.p[2], 0);
				break;
			}
			}
		}
	}
}