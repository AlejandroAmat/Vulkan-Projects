#pragma once

// indices of locations of queue family in gpu

struct QueueFamilyIndices {
	int graphicsFamily = -1; //loction

	bool isValid() {
		return graphicsFamily >= 0;
	}
};