#pragma once
#ifdef _DEBUG
//#include "Toolkit.h"
#else
namespace Toolkit {
class ToolkitManager {
public:
	static ToolkitManager& Instance() {
		static ToolkitManager instance;
		return instance;
	}

	void Start() {
	}

	void Stop() {
	}

private:
	ToolkitManager() = default;
	~ToolkitManager() = default;
	ToolkitManager(const ToolkitManager&) = delete;
	ToolkitManager& operator=(const ToolkitManager&) = delete;
};
}
#endif
