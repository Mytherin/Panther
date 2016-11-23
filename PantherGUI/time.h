#pragma once

typedef long long time_t;

time_t GetTime();

class MouseClickInstance {
public:
	int x;
	int y;
	time_t time;
	int clicks;

	MouseClickInstance() : x(0), y(0), time(0), clicks(0) { }
};
