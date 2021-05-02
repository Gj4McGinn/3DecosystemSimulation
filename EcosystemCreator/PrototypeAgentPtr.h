#ifndef __PROTOTYPE_AGENT_PTR_h__
#define __PROTOTYPE_AGENT_PTR_h__

#include <GU/GU_Agent.h>
#include <GU/GU_AgentDefinition.h>
#include <GU/GU_AgentRig.h>
#include <GU/GU_AgentShapeLib.h>
#include <GU/GU_AgentLayer.h>
#include <GU/GU_PrimPacked.h>
#include <GU/GU_PrimPoly.h>
#include <GEO/GEO_AttributeCaptureRegion.h>
#include <GEO/GEO_AttributeIndexPairs.h>
#include <GA/GA_AIFIndexPair.h>
#include <GA/GA_Names.h>
#include "BNode.h"

#include <iostream>
#include <fstream>

// Inspired by SOP_BouncyAgent

namespace PrototypeAgentPtr
{
	extern int protocount;

	extern GU_Detail* generateGeom(std::shared_ptr<BNode> root);
	extern void traverseAndBuildGeo(GU_Detail* geo, std::shared_ptr<BNode> currNode,
		int divisions = 10);

	extern GU_AgentRigPtr createRig(const char* path, std::shared_ptr<BNode> root,
		std::vector<std::shared_ptr<BNode>>& inOrder);

	extern void addWeights(const GU_AgentRig& rig, 
		const GU_DetailHandle& geomHandle, 
		std::vector<std::shared_ptr<BNode>>& inOrder);

	extern GU_AgentShapeLibPtr createShapeLibrary(const char* path, 
		const GU_AgentRig& rig, GU_Detail* geo, 
		std::vector<std::shared_ptr<BNode>>& inOrder);

	extern GU_AgentLayerPtr createStartLayer(const char* path, 
		const GU_AgentRigPtr& rig, const GU_AgentShapeLibPtr &shapeLibrary);

	// TODO pass in environment (/ plant type) data
	//extern std::pair<GU_PrimPacked*, GU_AgentDefinitionPtr> 
	extern GU_AgentDefinitionPtr createDefinition(std::shared_ptr<BNode> root, 
		const char* path);

	extern GU_Agent* createAgent(GU_PrimPacked* packedPrim, 
		GU_AgentDefinitionPtr agentDef);
}

#endif