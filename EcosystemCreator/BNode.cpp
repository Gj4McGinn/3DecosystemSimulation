#include "BNode.h"
#include "SOP_Branch.h"
#include "PlantSpecies.h"
//using namespace HDK_Sample;

/// Constructors
BNode::BNode() 
	: position(0.0), age(0.0), maxLength(3.0), thickness(0.1), parent(nullptr),
		plantVars(nullptr), children(), connectedModules(), rigIndex(-1)
{}

BNode::BNode(BNode* other)
	: position(other->getPos()), unitDir(other->getDir()), age(other->getAge()), 
		maxLength(other->getMaxLength()), thickness(other->getBaseRadius()), 
		baseRadius(other->getBaseRadius()), rigIndex(other->getRigIndex()),
		plantVars(other->getPlantVars()), root(other->isRoot()),
		parent(nullptr), children(), connectedModules()
{}

BNode::BNode(UT_Vector3 pos, UT_Vector3 dir, float branchAge, float length, float thick, bool isRootNode)
	: position(pos), unitDir(dir), age(branchAge), maxLength(length),
		thickness(thick), baseRadius(thick), root(isRootNode), 
		parent(nullptr), plantVars(nullptr), children(),
		connectedModules(), rigIndex(-1)
{
	unitDir.normalize();
}

BNode::BNode(vec3 start, vec3 end, float branchAge, float length, float thick, bool isRootNode)
	: age(branchAge), maxLength(length), thickness(thick), baseRadius(thick),
		root(isRootNode), parent(nullptr), plantVars(nullptr), children(), 
		connectedModules(), rigIndex(-1)
{
	position = UT_Vector3();
	position(0) = end[0];
	position(1) = end[1];
	position(2) = end[2];

	unitDir = UT_Vector3();
	unitDir(0) = end[0] - start[0];
	unitDir(1) = end[1] - start[1];
	unitDir(2) = end[2] - start[2];
	if (abs(unitDir.length2()) < 0.000001f) {
		unitDir(0) = 0.f;
		unitDir(1) = 1.f;
		unitDir(2) = 0.f;
	}
	unitDir.normalize();
}

BNode::~BNode() {
	if (this->parent != nullptr) {
	}
	for (std::shared_ptr<BNode> child : children) {
		child->setParent(nullptr);
		//delete child;
	}
	for (SOP_Branch* connectedModule : connectedModules) {
		connectedModule->destroySelf();
	}
}

std::shared_ptr<BNode> BNode::deepCopy(std::shared_ptr<BNode> par) {
	std::shared_ptr<BNode> newNode(new BNode(this));
	newNode->setParent(par);

	for (std::shared_ptr<BNode> child : children) {
		std::shared_ptr<BNode> copyChild = child->deepCopy(newNode);
		newNode->addChild(copyChild);
	}
	return newNode;
}

/// SETTERS
void BNode::setParent(std::shared_ptr<BNode> par)
{
	parent = par;
}

void BNode::setPlantVars(PlantSpeciesVariables* vars) {
	plantVars = vars;
}

void BNode::addChild(std::shared_ptr<BNode> child)
{
	children.push_back(child);
}

void BNode::addModuleChild(SOP_Branch* child) {
	connectedModules.push_back(child);
}

// Adjust all new age-based calculations
void BNode::setAge(float changeInAge, 
	std::vector<std::shared_ptr<BNode>>& terminalNodes, bool mature, bool decay) {
	age += changeInAge;

	// For roots of child modules
	if (parent && isRoot()) { position = parent->getPos(); }
	
	// For full branch-segments only, update length and position:
	else if (parent) {
		float branchLength = min(maxLength, age * plantVars->getBeta());

		position = parent->getPos() + branchLength * unitDir;

		// Calculate tropism offset using  static values
		float g1 = pow(0.95f, age * plantVars->getG1());   // Controls tropism decrease over time
		float g2 = -plantVars->getG2();                    // Controls tropism strength
		UT_Vector3 gDir = UT_Vector3(0.0f, -1.0f, 0.0f);  // Gravity Direction

		UT_Vector3 tOffset = (g1 * g2 * gDir) / max(age + g1, 0.05f);

		position += tOffset * branchLength; // scaled it for the effect to be proportionate
	}

	// Branch thickness update:
	thickness = max(0.015f, age * baseRadius * plantVars->getTC());
	// There's an age difference of 1 between terminal nodes and their children
	// This is how I've decided to deal with it
	if (parent && isRoot()) { thickness = parent->getThickness(); }

	// Update children
	for (std::shared_ptr<BNode> child : children) {
		child->setAge(changeInAge, terminalNodes, mature, decay);
	}

	if (mature && children.empty() && connectedModules.empty()) {
		terminalNodes.push_back(shared_from_this());
	}
	// Clear/cull modules if it is not mature (rewinding of time)
	else if (decay && !connectedModules.empty()) {
		for (SOP_Branch* connectedMod : connectedModules) {
			connectedMod->destroySelf();
		}
		connectedModules.clear();
	}
}

/// GETTERS
PlantSpeciesVariables* BNode::getPlantVars() {
	return plantVars;
}

bool BNode::isRoot() const {
	return root;
}

std::shared_ptr<BNode> BNode::getParent() {
	return parent;
}

std::vector<std::shared_ptr<BNode>>& BNode::getChildren() {
	return children;
}

UT_Vector3 BNode::getPos()
{
	return position;
}

UT_Vector3 BNode::getDir()
{
	return unitDir;
}

float BNode::getAge()
{
	return age;
}

float BNode::getMaxLength()
{
	return maxLength;
}

float BNode::getThickness()
{
	return thickness;
}

float BNode::getBaseRadius()
{
	return baseRadius;
}

int BNode::getRigIndex()
{
	return rigIndex;
}

void BNode::setRigIndex(int idx)
{
	rigIndex = idx;
}

UT_Matrix4 BNode::getWorldTransform() {
	UT_Matrix4 transform = UT_Matrix4(1.0f);
	UT_Vector3 c = UT_Vector3();

	if (!parent) {
		transform = UT_Matrix4(UT_Matrix3::dihedral(UT_Vector3(0.0f, 1.0f, 0.0f),
			getDir(), c, 1));
	}

	else if (isRoot()) {
		// Getting the angle of the parent branch segment
		UT_Vector3 parentDir;
		if (!parent->isRoot() && parent->getParent()) {
			parentDir = parent->getPos() - parent->getParent()->getPos();
		}
		else if (parent->isRoot() && parent->getParent() && parent->getParent()->getParent()) {
			// Skipping the terminal node since it's located in the same place as the root node
			parentDir = parent->getPos() - parent->getParent()->getParent()->getPos();
		}
		else { parentDir = parent->getDir(); }
		//parentDir.normalize();

		transform = UT_Matrix4(UT_Matrix3::dihedral(UT_Vector3(0.0f, 1.0f, 0.0f),
			parentDir, c, 1));
	}

	else {

		UT_Vector3 currDir = position - parent->getPos();
		transform = UT_Matrix4(UT_Matrix3::dihedral(UT_Vector3(0.0f, 1.0f, 0.0f),
			currDir, c, 1));
	}
	transform.setTranslates(position);
	return transform;
}

/// More forms of updating
void BNode::recTransformation(float ageDif, float radiusMultiplier, 
	float lengthMultiplier, UT_Matrix3& rotation) {
	age += ageDif;
	baseRadius *= radiusMultiplier;
	maxLength *= lengthMultiplier;
	unitDir = rowVecMult(unitDir, rotation);

	for (std::shared_ptr<BNode> child : children) { 
		child->recTransformation(ageDif, radiusMultiplier, 
			lengthMultiplier, rotation);
	}
}