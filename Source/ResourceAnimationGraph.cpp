#include "ResourceAnimationGraph.h"
#include "Application.h"
#include "ModuleUI.h"
#include "ModuleResourcesManager.h"
#include "PanelAnimationGraph.h"
#include "ResourceAnimation.h"

inline static float GetSquaredDistanceToBezierCurve(const ImVec2& point, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4);
inline bool containPoint(ImVec2 A, ImVec2 B, ImVec2 point);

ResourceAnimationGraph::ResourceAnimationGraph(resource_deff deff) :Resource(deff)
{
	Parent3dObject = deff.Parent3dObject;
	App->fs.getFileNameFromPath(asset);
}

ResourceAnimationGraph::~ResourceAnimationGraph()
{
}

void ResourceAnimationGraph::LoadToMemory()
{
	LoadGraph();
}

bool ResourceAnimationGraph::LoadGraph()
{
	//Get the buffer
	char* buffer = App->fs.ImportFile(binary.c_str());
	if (buffer == nullptr)
	{
		return false;
	}
	char* cursor = buffer;

	uint bytes = 0;

	int nodeAmount;
	memcpy(&nodeAmount, cursor, sizeof(int));
	cursor += sizeof(int);

	uint startUUID;
	memcpy(&startUUID, cursor, sizeof(uint));
	cursor += sizeof(uint);

	for (int i = 0; i < nodeAmount; ++i)
	{
		uint nameLength;
		memcpy(&nameLength, cursor, sizeof(uint));
		cursor += sizeof(uint);

		bytes = sizeof(char)*nameLength;
		char* auxName = new char[nameLength];
		memcpy(auxName, cursor, bytes);
		std::string name = auxName;
		name = name.substr(0, nameLength);
		RELEASE_ARRAY(auxName);
		cursor += bytes;

		float position[2];
		bytes = sizeof(float) * 2;
		memcpy(position, cursor, bytes);
		cursor += bytes;

		uint uids[4];
		bytes = sizeof(uint) * 4;
		memcpy(uids, cursor, bytes);
		cursor += bytes;

		std::string animName;
		std::string assetName;
		if (uids[1] != 0)
		{
			bytes = sizeof(char)*uids[1];
			char* auxAnim = new char[uids[1]];
			memcpy(auxAnim, cursor, bytes);
			animName = auxAnim;
			animName = animName.substr(0, uids[1]);
			RELEASE_ARRAY(auxAnim);
			cursor += bytes;

			bytes = sizeof(char)*uids[2];
			char* auxAsset = new char[uids[2]];
			memcpy(auxAsset, cursor, bytes);
			assetName = auxAsset;
			assetName = assetName.substr(0, uids[2]);
			RELEASE_ARRAY(auxAsset);
			cursor += bytes;
		}

		bool loop;
		memcpy(&loop, cursor, sizeof(bool));
		cursor += sizeof(bool);

		float speed;
		memcpy(&speed, cursor, sizeof(float));
		cursor += sizeof(float);

		Node* node = new Node(name.c_str(), this->uuid, { position[0], position[1] }, uids[0]);
		if (App->is_game)
			node->animationUID = App->resources->getResourceUuid(animName.c_str() , R_ANIMATION);
		else
			node->animationUID = App->resources->getAnimationResourceUuid(assetName.c_str(), animName.c_str());
		 
		node->loop = loop;
		node->speed = speed;

		for (int j = 0; j < uids[3]; ++j)
		{
			int type;
			bytes = sizeof(int);
			memcpy(&type, cursor, bytes);
			cursor += bytes;

			uint links[2];
			bytes = sizeof(uint) * 2;
			memcpy(links, cursor, bytes);
			cursor += bytes;

			NodeLink* link = node->addLink((linkType)type, true, links[0]);
			link->connect(links[1]);
		}

		uint transitionAmount;
		memcpy(&transitionAmount, cursor, sizeof(uint));
		cursor += sizeof(uint);
		for (int k = 0; k < transitionAmount; ++k)
		{
			float durations[2];
			bytes = sizeof(durations);
			memcpy(durations, cursor, bytes);
			cursor += bytes;

			uint transitionUids[3];
			bytes = sizeof(transitionUids);
			memcpy(transitionUids, cursor, bytes);
			cursor += bytes;

			Transition* trans = new Transition(transitionUids[0], transitionUids[1], uuid);
			trans->duration = durations[0];
			trans->nextStart = durations[1];
			for (int k = 0; k < transitionUids[2]; ++k)
			{
				Condition* cond = new Condition();

				uint ranges[3];
				memcpy(ranges, cursor, sizeof(uint) * 3);
				cursor += sizeof(uint) * 3;

				memcpy(&cond->conditionant, cursor, sizeof(float));
				cursor += sizeof(float);

				bytes = sizeof(char)*ranges[2];
				if (bytes > 0)
				{
					char* auxName = new char[ranges[2]];
					memcpy(auxName, cursor, bytes);
					std::string name = auxName;
					name = name.substr(0, ranges[2]);
					RELEASE_ARRAY(auxName);
					cond->string_conditionant = name;
				}
				cursor += bytes;

				cond->type = (conditionType)ranges[0];
				cond->variable_uuid = ranges[1];

				trans->conditions.push_back(cond);
			}
			node->transitions.push_back(trans);
		}

		nodes.insert(std::pair<uint, Node*>(uids[0], node));
	}

	for (std::map<uint, Node*>::iterator it_n = nodes.begin(); it_n != nodes.end(); ++it_n)
	{
		Node* node = (*it_n).second;

		for (std::list<Transition*>::iterator it_t = node->transitions.begin(); it_t != node->transitions.end(); ++it_t)
		{
			(*it_t)->setConnection((*it_t)->output, (*it_t)->input);
		}
	}
	start = getNode(startUUID);

	int variableAmount;
	memcpy(&variableAmount, cursor, sizeof(int));
	cursor += sizeof(int);
	for (int i = 0; i < variableAmount; ++i)
	{
		uint ranges[3];
		bytes = sizeof(ranges);
		memcpy(ranges, cursor, bytes);
		cursor += bytes;

		bytes = sizeof(char)*ranges[2];
		char* auxName = new char[ranges[2]];
		memcpy(auxName, cursor, bytes);
		std::string name = auxName;
		name = name.substr(0, ranges[2]);
		RELEASE_ARRAY(auxName);
		cursor += bytes;

		Variable* var = new Variable((variableType)ranges[1], name.c_str());
		var->uuid = ranges[0];
		blackboard.push_back(var);
	}

	RELEASE_ARRAY(buffer);
	return true;
}

void ResourceAnimationGraph::UnloadFromMemory()
{
	for (std::map<uint, Node*>::iterator it_n = nodes.begin(); it_n != nodes.end(); ++it_n)
	{
		RELEASE((*it_n).second);
	}
	nodes.clear();
	links.clear(); //Links are deleted from node destructor
	for (std::list<Variable*>::iterator it_v = blackboard.begin(); it_v != blackboard.end(); ++it_v)
	{
		RELEASE((*it_v));
	}
	blackboard.clear();
}

bool ResourceAnimationGraph::saveGraph() const
{
	uint size = 0;
	//Nodes: nameLength, name, position, UID, animation UID, num Links, loop
	size += sizeof(int)+sizeof(uint);
	for (std::map<uint, Node*>::const_iterator it_n = nodes.begin(); it_n != nodes.end(); ++it_n)
	{
		ResourceAnimation* anim = (ResourceAnimation*)App->resources->getResource((*it_n).second->animationUID);
		size += (*it_n).second->name.length()*sizeof(char) + 3*sizeof(float) + 5*sizeof(uint) + sizeof(bool);

		if (anim != nullptr)
		{
			size += anim->asset.length() * sizeof(char);
			size += anim->Parent3dObject.length() + sizeof(char);
		}

		for (std::list<NodeLink*>::iterator it_l = (*it_n).second->links.begin(); it_l != (*it_n).second->links.end(); ++it_l)
		{
			size += sizeof(int) + 2*sizeof(uint);
		}
		size += sizeof(uint);
		for (std::list<Transition*>::iterator it_t = (*it_n).second->transitions.begin(); it_t != (*it_n).second->transitions.end(); ++it_t)
		{
			size += 2*sizeof(float) + 3*sizeof(uint);
			for (std::list<Condition*>::iterator it_c = (*it_t)->conditions.begin(); it_c != (*it_t)->conditions.end(); ++it_c)
			{
				size += 3*sizeof(uint) + sizeof(float) + (*it_c)->string_conditionant.size()*sizeof(char);
			}
		}
	}
	size += sizeof(int);
	for (std::list<Variable*>::const_iterator it_v = blackboard.begin(); it_v != blackboard.end(); ++it_v)
	{
		size += 3 * sizeof(uint) + (*it_v)->name.size() * sizeof(char);
	}

	char* buffer = new char[size];
	char* cursor = buffer;

	uint bytes = 0;

	int nodeAmount = nodes.size();
	memcpy(cursor, &nodeAmount, sizeof(int));
	cursor += sizeof(int);

	uint uuid = (nodeAmount > 0) ? start->UID : 0;
	memcpy(cursor, &uuid, sizeof(uint));
	cursor += sizeof(uint);

	for (std::map<uint, Node*>::const_iterator it_n = nodes.begin(); it_n != nodes.end(); ++it_n)
	{
		bytes = (*it_n).second->name.length();
		memcpy(cursor, &bytes, sizeof(uint));
		cursor += sizeof(uint);

		bytes = sizeof(char)*bytes;
		memcpy(cursor, (*it_n).second->name.c_str(), bytes);
		cursor += bytes;

		float position[2] = { (*it_n).second->pos.x, (*it_n).second->pos.y };
		bytes = sizeof(float) * 2;
		memcpy(cursor, &position[0], bytes);
		cursor += bytes;

		ResourceAnimation* anim = (ResourceAnimation*)App->resources->getResource((*it_n).second->animationUID);
		uint animNameLength = (anim == nullptr)? 0 : anim->asset.length();
		uint assetLength = (anim == nullptr) ? 0 : anim->Parent3dObject.length();

		uint uids[4] = { (*it_n).second->UID, animNameLength, assetLength, (*it_n).second->links.size() };
		bytes = sizeof(uint) * 4;
		memcpy(cursor, &uids[0], bytes);
		cursor += bytes;

		if (anim != nullptr)
		{
			bytes = animNameLength * sizeof(char);
			memcpy(cursor, anim->asset.c_str(), bytes);
			cursor += bytes;

			bytes = assetLength * sizeof(char);
			memcpy(cursor, anim->Parent3dObject.c_str(), bytes);
			cursor += bytes;
		}

		memcpy(cursor, &(*it_n).second->loop, sizeof(bool));
		cursor += sizeof(bool);

		memcpy(cursor, &(*it_n).second->speed, sizeof(float));
		cursor += sizeof(float);

		for (std::list<NodeLink*>::iterator it_l = (*it_n).second->links.begin(); it_l != (*it_n).second->links.end(); ++it_l)
		{
			int type = (int)(*it_l)->type;
			bytes = sizeof(int);
			memcpy(cursor, &type, bytes);
			cursor += bytes;

			uint links[2] = { (*it_l)->UID, (*it_l)->connectedNodeLink };
			bytes = sizeof(uint) * 2;
			memcpy(cursor, &links[0], bytes);
			cursor += bytes;
		}

		uint transitionAmount = (*it_n).second->transitions.size();
		memcpy(cursor, &transitionAmount, sizeof(uint));
		cursor += sizeof(uint);

		for (std::list<Transition*>::iterator it_t = (*it_n).second->transitions.begin(); it_t != (*it_n).second->transitions.end(); ++it_t)
		{
			float durations[2] = { (*it_t)->duration ,(*it_t)->nextStart };
			bytes = sizeof(durations);
			memcpy(cursor, &durations[0], bytes);
			cursor += bytes;

			uint transitionUids[3] = { (*it_t)->output, (*it_t)->input, (*it_t)->conditions.size() };
			bytes = sizeof(transitionUids);
			memcpy(cursor, transitionUids, bytes);
			cursor += bytes;

			for (std::list<Condition*>::iterator it_c = (*it_t)->conditions.begin(); it_c != (*it_t)->conditions.end(); ++it_c)
			{
				uint ranges[3] = { (*it_c)->type, (*it_c)->variable_uuid, (*it_c)->string_conditionant.size() };
				bytes = sizeof(ranges);
				memcpy(cursor, &ranges[0], bytes);
				cursor += bytes;

				memcpy(cursor, &(*it_c)->conditionant, sizeof(float));
				cursor += sizeof(float);

				memcpy(cursor, (*it_c)->string_conditionant.c_str(), ranges[2] * sizeof(char));
				cursor += ranges[2] * sizeof(char);
			}
		}
	}
	int variableAmount = blackboard.size();
	memcpy(cursor, &variableAmount, sizeof(int));
	cursor += sizeof(int);
	for (std::list<Variable*>::const_iterator it_v = blackboard.begin(); it_v != blackboard.end(); ++it_v)
	{
		uint ranges[3] = { (*it_v)->uuid, (*it_v)->type, (*it_v)->name.size() };
		bytes = sizeof(ranges);
		memcpy(cursor, &ranges[0], bytes);
		cursor += bytes;

		memcpy(cursor, (*it_v)->name.c_str(), sizeof(char)*ranges[2]);
		cursor += sizeof(char)*ranges[2];
	}

	App->fs.ExportBuffer(buffer, size, (USER_GRAPHS_FOLDER + asset + GRAPH_EXTENSION).c_str());
	RELEASE_ARRAY(buffer);

	return true;
}

Node* ResourceAnimationGraph::addNode(const char* name, float2 pos)
{
	Node* node = new Node(name, uuid, pos);

	if (nodes.size() == 0)
		start = node;

	nodes.insert(std::pair<uint, Node*>(node->UID, node));

	return node;
}

void ResourceAnimationGraph::pushLink(NodeLink* link)
{
	links.insert(std::pair<uint, NodeLink*>(link->UID, link));
}

Variable* ResourceAnimationGraph::getVariable(uint UID) const
{
	Variable* ret = nullptr;

	for (std::list<Variable*>::const_iterator it = blackboard.begin(); it != blackboard.end(); ++it)
	{
		if ((*it)->uuid == UID)
		{
			ret = (*it);
			break;
		}
	}

	return ret;
}

uint ResourceAnimationGraph::getVariableUUID(const char* name, variableType type) const
{
	uint ret = 0;

	for (std::list<Variable*>::const_iterator it = blackboard.begin(); it != blackboard.end(); ++it)
	{
		if ((*it)->type == type && (*it)->name == name)
		{
			ret = (*it)->uuid;
			break;
		}
	}

	return ret;
}

Node* ResourceAnimationGraph::getNode(uint UID)
{
	if (nodes.find(UID) != nodes.end())
		return nodes[UID];

	return nullptr;
}

NodeLink* ResourceAnimationGraph::getLink(uint UID)
{
	if (links.find(UID) != links.end())
		return links[UID];

	return nullptr;
}

Node::Node(const char* name, uint graphUID, float2 pos, float2 size) : name(name), pos(pos), size(size), UID(random32bits()), graphUID(graphUID)
{
	addLink(OUTPUT_LINK);
}

Node::Node(const char * name, uint graphUID, float2 pos, uint forced_UID) : name(name), pos(pos), size({ 150,NODE_HEIGHT }), UID(forced_UID), graphUID(graphUID)
{
}

Node::~Node()
{
	for (std::list<NodeLink*>::iterator it_l = links.begin(); it_l != links.end(); ++it_l)
	{
		RELEASE((*it_l));
	}
	links.clear();
	for (std::list<Transition*>::iterator it_t = transitions.begin(); it_t != transitions.end(); ++it_t)
	{
		RELEASE((*it_t));
	}
	transitions.clear();
}

NodeLink* Node::addLink(linkType type, bool addToList, uint forced_uid)
{
	NodeLink* ret = new NodeLink(type, UID, graphUID);
	if (forced_uid != 0)
		ret->UID = forced_uid;

	((ResourceAnimationGraph*)App->resources->getResource(graphUID))->pushLink(ret);
	if (addToList)
		links.push_back(ret);

	switch (type)
	{
	case INPUT_LINK:
		++inputCount;
		break;
	case OUTPUT_LINK:
		++outputCount;
		break;
	}

	int maxCount = (inputCount > outputCount) ? inputCount : outputCount;
	int height = GRAPH_NODE_WINDOW_PADDING * 2 + GRAPH_LINK_RADIUS * 3 * maxCount;
	if (size.y < height)
		size.y = height;

	return ret;
}

void Node::removeLink(NodeLink* link)
{
	if (link->type == OUTPUT_LINK)
	{
		NodeLink* prev = nullptr;
		for (std::list<NodeLink*>::iterator it_l = links.begin(); it_l != links.end(); ++it_l)
		{
			if ((*it_l)->UID == link->UID)
			{
				break;
			}
			prev = (*it_l);
		}
		switch (prev->type)
		{
		case INPUT_LINK:
			--inputCount;
			break;
		case OUTPUT_LINK:
			--outputCount;
			break;
		}
		links.remove(prev);
		RELEASE(prev);

		//Remove transition
		for (std::list<Transition*>::iterator it = transitions.begin(); it != transitions.end(); ++it)
		{
			if ((*it)->output == link->UID)
			{
				Transition* trans = (*it);
				transitions.erase(it);
				RELEASE(trans);
				break;
			}
		}
	}

	switch (link->type)
	{
	case INPUT_LINK:
		--inputCount;
		break;
	case OUTPUT_LINK:
		--outputCount;
		break;
	}
	links.remove(link);
	RELEASE(link);

	//Recalculate height
	int maxCount = (inputCount > outputCount) ? inputCount : outputCount;
	int height = GRAPH_NODE_WINDOW_PADDING * 2 + GRAPH_LINK_RADIUS * 3 * maxCount;
	if (height > NODE_HEIGHT)
		size.y = height;
	else
		size.y = NODE_HEIGHT;
}

uint Node::drawLinks() const
{
	uint ret = 0;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ResourceAnimationGraph* graph = (ResourceAnimationGraph*)App->resources->getResource(graphUID);

	int OUTcount = 0, INcount = 0;

	for (std::list<NodeLink*>::const_iterator it_l = links.begin(); it_l != links.end(); ++it_l)
	{
		switch ((*it_l)->type)
		{
		case INPUT_LINK:
			++INcount;
			(*it_l)->nodePos = { 0, ((size.y / inputCount)*INcount) - ((size.y / inputCount) / 2) };
			break;
		case OUTPUT_LINK:
			++OUTcount;
			(*it_l)->nodePos = { size.x, ((size.y / outputCount)*OUTcount) - ((size.y / outputCount) / 2) };
			break;
		}
		ImVec2 linkPos = { gridPos.x + (*it_l)->nodePos.x, gridPos.y + (*it_l)->nodePos.y };

		draw_list->AddCircleFilled(linkPos, GRAPH_LINK_RADIUS, IM_COL32(25, 25, 25, 255));
		draw_list->AddCircle(linkPos, GRAPH_LINK_RADIUS, IM_COL32(130, 130, 130, 255), 12, 1.5f);
		if ((*it_l)->connectedNodeLink != 0)
			draw_list->AddCircleFilled(linkPos, GRAPH_LINK_RADIUS / 2, IM_COL32(130, 130, 130, 255));

		//Draw line
		float2 mousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
		if (!App->gui->p_animation_graph->hoveringTransitionMenu && (*it_l)->type == OUTPUT_LINK && (*it_l)->connectedNodeLink == 0 && ImGui::IsMouseClicked(0) && mousePos.Distance({ linkPos.x, linkPos.y }) <= GRAPH_LINK_RADIUS)
		{
			(*it_l)->linking = true;
			ret = (*it_l)->UID;
		}
		if ((*it_l)->linking)
		{
			draw_list->ChannelsSetCurrent(1);
			draw_list->AddBezierCurve(linkPos, { linkPos.x + (((*it_l)->type == OUTPUT_LINK) ? 50.0f : -50.0f), linkPos.y }, { mousePos.x - 50, mousePos.y }, { mousePos.x, mousePos.y }, IM_COL32(200, 200, 100, 255), 3.0f);
			draw_list->ChannelsSetCurrent(0);

			if (!ImGui::IsMouseDown(0))
				(*it_l)->linking = false;
		}
	}

	return ret;
}

bool Node::checkLink(Node* node)
{
	bool ret = false;

	for (std::list<Transition*>::iterator it_t = transitions.begin(); it_t != transitions.end(); ++it_t)
	{
		if ((*it_t)->destination == node->UID)
		{
			ret = true;
			break;
		}
	}

	return ret;
}

void Node::connectLink(uint linkUID)
{
	NodeLink* prev = nullptr;
	for (std::list<NodeLink*>::iterator it_l = links.begin(); it_l != links.end(); ++it_l)
	{
		if ((*it_l)->UID == linkUID)
		{
			if (prev == nullptr || prev->connectedNodeLink != 0)
			{
				//Create link	
				NodeLink* added = addLink(OUTPUT_LINK, false);
				if (prev == nullptr)
					links.push_front(added);
				else
					links.insert(it_l, added);
			}
			if (++it_l == links.end() || (*it_l)->connectedNodeLink != 0)
			{
				//Create link
				NodeLink* added = addLink(OUTPUT_LINK, false);
				if (it_l == links.end())
					links.push_back(added);
				else
					links.insert(it_l, added);
			}

			break;
		}
		prev = (*it_l);
	}
}

NodeLink::NodeLink(linkType type, uint nodeUID, uint resourceUID) : type(type), nodeUID(nodeUID), resourceUID(resourceUID), UID(random32bits())
{
}

NodeLink::~NodeLink()
{
	ResourceAnimationGraph* graph = (ResourceAnimationGraph*)App->resources->getResource(resourceUID);

	NodeLink* link = graph->getLink(connectedNodeLink);
	if (link != nullptr)
	{
		link->connectedNodeLink = 0;
		graph->getNode(link->nodeUID)->removeLink(link);
	}
}

Transition::Transition(uint output, uint input, uint graphUID) : output(output), input(input), graphUID(graphUID)
{
	ResourceAnimationGraph* graph = (ResourceAnimationGraph*)App->resources->getResource(graphUID);
	
	NodeLink* outputLink = graph->getLink(output);
	NodeLink* inputLink = graph->getLink(input);

	if (outputLink != nullptr && inputLink != nullptr)
	{
		Node* outputNode = graph->getNode(outputLink->nodeUID);
		Node* inputNode = graph->getNode(inputLink->nodeUID);
		if (outputNode != nullptr && inputNode != nullptr)
		{
			origin = outputNode->UID;
			destination = inputNode->UID;
		}
	}
}

Transition::~Transition()
{
	for (std::list<Condition*>::iterator it = conditions.begin(); it != conditions.end(); ++it)
	{
		RELEASE((*it));
	}
	conditions.clear();
}

bool Transition::drawLine(bool selected, bool inTransition)
{
	bool ret = false;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	ResourceAnimationGraph* graph = (ResourceAnimationGraph*)App->resources->getResource(graphUID);

	Node* originNode = graph->getNode(origin);
	Node* destinationNode = graph->getNode(destination);
	NodeLink* outputLink = graph->getLink(output);
	NodeLink* inputLink = graph->getLink(input);

	ImVec2 originPos = { originNode->gridPos.x + outputLink->nodePos.x, originNode->gridPos.y + outputLink->nodePos.y };
	ImVec2 destinationPos = { destinationNode->gridPos.x + inputLink->nodePos.x, destinationNode->gridPos.y + inputLink->nodePos.y };

	float lineThickness = 3.0f;
	float triangleSize = 10.0f;
	ImU32 color = IM_COL32(200, 200, 100, 255);
	if (inTransition)
	{
		color = IM_COL32(75, 255, 255, 255);
	}
	else if (selected)
	{
		color = IM_COL32(240, 240, 175, 255);
		lineThickness = 5.0f;
		triangleSize = 15.0f;
	}

	draw_list->ChannelsSetCurrent(1);
	draw_list->AddBezierCurve(originPos, { originPos.x + 50.0f, originPos.y }, { destinationPos.x - 50.0f, destinationPos.y }, destinationPos, color, lineThickness);
	draw_list->AddTriangleFilled(destinationPos, { destinationPos.x - triangleSize, destinationPos.y + triangleSize / 2 }, { destinationPos.x - triangleSize, destinationPos.y - triangleSize / 2 }, color);
	draw_list->ChannelsSetCurrent(0);

	if (ImGui::IsMouseClicked(0))
	{
		if (App->gui->p_animation_graph->linkingNode == 0 && containPoint(originPos, destinationPos, ImGui::GetMousePos()))
		{
			float distance = GetSquaredDistanceToBezierCurve(ImGui::GetMousePos(), originPos, { originPos.x + 50.0f, originPos.y }, { destinationPos.x - 50.0f, destinationPos.y }, destinationPos);
			if (distance <= 15.0f)
				ret = true;
		}
	}

	return ret;
}

void Transition::setConnection(uint output, uint input)
{
	this->output = output;
	this->input = input;

	ResourceAnimationGraph* graph = (ResourceAnimationGraph*)App->resources->getResource(graphUID);

	NodeLink* outputLink = graph->getLink(output);
	NodeLink* inputLink = graph->getLink(input);

	if (outputLink != nullptr && inputLink != nullptr)
	{
		Node* outputNode = graph->getNode(outputLink->nodeUID);
		Node* inputNode = graph->getNode(inputLink->nodeUID);
		if (outputNode != nullptr && inputNode != nullptr)
		{
			origin = outputNode->UID;
			destination = inputNode->UID;
		}
	}
}

inline static float ImVec2Dot(const ImVec2& S1, const ImVec2& S2) { return (S1.x*S2.x + S1.y*S2.y); }

inline static float GetSquaredDistancePointSegment(const ImVec2& P, const ImVec2& S1, const ImVec2& S2) {
	const float l2 = (S1.x - S2.x)*(S1.x - S2.x) + (S1.y - S2.y)*(S1.y - S2.y);
	if (l2 < 0.0000001f) return (P.x - S2.x)*(P.x - S2.x) + (P.y - S2.y)*(P.y - S2.y);   // S1 == S2 case
	ImVec2 T({ S2.x - S1.x,S2.y - S1.y });
	const float tf = ImVec2Dot({ P.x - S1.x, P.y - S1.y }, T) / l2;
	const float minTf = 1.f < tf ? 1.f : tf;
	const float t = 0.f > minTf ? 0.f : minTf;
	T = { S1.x + T.x*t,S1.y + T.y*t };  // T = Projection on the segment
	return (P.x - T.x)*(P.x - T.x) + (P.y - T.y)*(P.y - T.y);
}

inline static float GetSquaredDistanceToBezierCurve(const ImVec2& point, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4) {
	static const int num_segments = 4;   // Num Sampling Points In between p1 and p4
	static bool firstRun = true;
	static ImVec4 weights[num_segments];

	if (firstRun) {
		// static init here
		IM_ASSERT(num_segments > 0);    // This are needed for computing distances: cannot be zero
		firstRun = false;
		for (int i = 1; i <= num_segments; i++) {
			float t = (float)i / (float)(num_segments + 1);
			float u = 1.0f - t;
			weights[i - 1].x = u * u*u;
			weights[i - 1].y = 3 * u*u*t;
			weights[i - 1].z = 3 * u*t*t;
			weights[i - 1].w = t * t*t;
		}
	}

	float minSquaredDistance = FLT_MAX, tmp;   // FLT_MAX is probably in <limits.h>
	ImVec2 L = p1, tp;
	for (int i = 0; i < num_segments; i++) {
		const ImVec4& W = weights[i];
		tp.x = W.x*p1.x + W.y*p2.x + W.z*p3.x + W.w*p4.x;
		tp.y = W.x*p1.y + W.y*p2.y + W.z*p3.y + W.w*p4.y;

		tmp = GetSquaredDistancePointSegment(point, L, tp);
		if (minSquaredDistance > tmp) minSquaredDistance = tmp;
		L = tp;
	}
	tp = p4;
	tmp = GetSquaredDistancePointSegment(point, L, tp);
	if (minSquaredDistance > tmp) minSquaredDistance = tmp;

	return minSquaredDistance;
}

inline bool containPoint(ImVec2 A, ImVec2 B, ImVec2 point)
{
	ImVec2 bottomLeft = { Min(A.x, B.x), Max(A.x, B.x) };
	ImVec2 topRight = { Max(A.x, B.x), Min(A.y, B.y) };

	return point.x > bottomLeft.x && point.x < topRight.x && point.y < bottomLeft.y && point.y > topRight.y;
}