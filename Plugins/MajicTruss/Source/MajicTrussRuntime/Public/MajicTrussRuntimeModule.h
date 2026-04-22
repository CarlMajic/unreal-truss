#pragma once

#include "Modules/ModuleManager.h"

class FMajicTrussRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

