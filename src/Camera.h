#pragma once

#include "../Engine/Engine.h"

class Camera : public vn::Transform {
public:
	Camera(int Mode);

	vn::mat4 getViewMatrix() const;

	vn::mat4 getProjMatrix() const;

	void follow(vn::Transform &entity);

	void update();


	~Camera() = default;

private:
	float lerp;
	//Mode 0 is default, 1 is Left Eye, 2 is Right Eye
	int mode;

	vn::mat4 proj;

	vn::Transform *p_entity;
};

