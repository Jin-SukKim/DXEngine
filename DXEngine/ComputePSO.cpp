#include "pch.h"
#include "ComputePSO.h"

void DE::ComputePSO::operator=(const ComputePSO& pso)
{
	computeShader = pso.computeShader;
}
