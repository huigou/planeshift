/*
    Crystal Space Entity Layer
    Copyright (C) 2009 by Jorrit Tyberghein
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include "CelHPF.h"

CS_PLUGIN_NAMESPACE_BEGIN(celNavMesh)
{



/*
 * celHPath
 */

celHPath::celHPath (csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >& navmeshes)
    : scfImplementationType (this), navMeshes(navmeshes)
{
  reverse = false;
}

celHPath::~celHPath ()
{
}

void celHPath::Initialize(iCelPath* highLevelPath)
{
  hlPath = highLevelPath;

  // calculate path length
  {
    length = 0;
    advanced = 0;

    iMapNode* src = hlPath->GetFirst();
    iMapNode* dst = hlPath->Next();
    do
    {
      // cross-sector connections are coincident
      if(src->GetSector() == dst->GetSector())
      {
        length += (dst->GetPosition()-src->GetPosition()).Norm();
      }
      src = hlPath->Next();
      dst = hlPath->Next();
    }
    while(hlPath->HasNext());

    // restart path for traversal
    hlPath->Restart();
  }

  // Get first and last node of the high level graph
  firstNode = hlPath->GetFirst();
  lastNode = hlPath->GetLast();

  // Resize low level path array. High level paths always have two nodes for each segment of the
  // path in a sector (entry and exit point).
  size_t llSize = hlPath->GetNodeCount() / 2;
  llPaths.SetSize(llSize);
  currentllPosition = 0;

  // Construct first part of the low level path
  csRef<iMapNode> goal = hlPath->Next();
  currentSector = firstNode->GetSector();
  csRef<iCelNavMesh> navMesh = navMeshes.Get(currentSector, 0);
  csRef<iCelNavMeshPath> path = navMesh->ShortestPath(firstNode->GetPosition(), goal->GetPosition());
  llPaths[0] = path;

  // Set current node
  currentNode = firstNode;
}

bool celHPath::HasNextInternal () const
{
  if (!llPaths[currentllPosition]->HasNext())
  {
    if (currentllPosition + 1 >= llPaths.GetSize())
    {
      return false;
    }
  }

  return true;
}

bool celHPath::HasPreviousInternal () const
{
  if (!llPaths[currentllPosition]->HasPrevious())
  {
    if (currentllPosition <= 0)
    {
      return false;
    }
  }

  return true;
}

bool celHPath::HasNext () const
{
  if (reverse)
  {
    return HasPreviousInternal();
  }
  return HasNextInternal();
}

bool celHPath::HasPrevious () const
{
  if (reverse)
  {
    return HasNextInternal();
  }
  return HasPreviousInternal();
}

iMapNode* celHPath::NextInternal ()
{
  csVector3 position;
  
  if (!llPaths[currentllPosition]->HasNext())
  {
    advanced += llPaths[currentllPosition]->Length();
    if (currentllPosition + 1 >= llPaths.GetSize())
    {
      return 0;
    }

    if (!llPaths[currentllPosition + 1].IsValid())
    {
      csRef<iMapNode> from = hlPath->Next();
      csRef<iMapNode> goal = hlPath->Next();
      currentSector = from->GetSector();
      csRef<iCelNavMesh> navMesh = navMeshes.Get(currentSector, 0);
      csRef<iCelNavMeshPath> path = navMesh->ShortestPath(from->GetPosition(), goal->GetPosition());
      currentllPosition++;
      llPaths[currentllPosition] = path;
      // naively walk towards the goal if we don't have a path
      // this should only happen at warp portals
      if(!path->HasNext())
      {
        currentNode = goal;
        return goal;
      }
    }
    else
    {
      currentllPosition++;
    }
  }
  // We don't use the first point of each low level path (except for the first path),
  // since it should at the same position of the last point in the previous low level path.
  llPaths[currentllPosition]->Next(position); 

  csRef<iMapNode> node;
  node.AttachNew(new csMapNode(""));
  node->SetPosition(position);
  node->SetSector(currentSector);
  currentNode = node;

  return node;
}

iMapNode* celHPath::PreviousInternal ()
{
  csVector3 position;
  bool changellPath = !llPaths[currentllPosition]->HasPrevious();

  if (!changellPath)
  {
    llPaths[currentllPosition]->Previous(position); 
    if (!llPaths[currentllPosition]->HasPrevious() && currentllPosition > 0)
    {
      // We don't use the first point of each low level path (except for the first path),
      // since it should at the same position of the last point in the previous low level path.
      changellPath = true;
    }
  }

  if (changellPath)
  {
    advanced += llPaths[currentllPosition]->Length();
    if (currentllPosition <= 0)
    {
      return 0;
    }

    if (llPaths[currentllPosition - 1].IsValid())
    {
      currentllPosition--;
    }
    else
    {
      // This should only be reached if a user inverts a path after having read a few steps with Next(), and
      // doesn't restart the path. In practice, this will probably never happen.
      csRef<iMapNode> goal = hlPath->Previous();
      csRef<iMapNode> from = hlPath->Previous();
      currentSector = from->GetSector();
      csRef<iCelNavMesh> navMesh = navMeshes.Get(currentSector, 0);
      csRef<iCelNavMeshPath> path = navMesh->ShortestPath(from->GetPosition(), goal->GetPosition());
      while (path->HasNext())
      {
        path->Next(position);
      }
      currentllPosition--;
      llPaths[currentllPosition] = path;
    }
    
    llPaths[currentllPosition]->Current(position); // Always use the last point of a path
  }

  csRef<iMapNode> node;
  node.AttachNew(new csMapNode(""));
  node->SetPosition(position);
  node->SetSector(currentSector);
  currentNode = node;

  return node;
}

iMapNode* celHPath::Next ()
{
  if (reverse)
  {
    return PreviousInternal();
  }
  return NextInternal();
}

iMapNode* celHPath::Previous ()
{
  if (reverse)
  {
    return NextInternal();
  }
  return PreviousInternal();
}

iMapNode* celHPath::Current ()
{
  return currentNode;
}

iMapNode* celHPath::GetFirst ()
{
  if (reverse)
  {
    return lastNode;
  }
  return firstNode;
}

iMapNode* celHPath::GetLast ()
{
  if (reverse)
  {
    return firstNode;
  }
  return lastNode;
}

void celHPath::Invert ()
{
  reverse = reverse ? false : true;
}

void celHPath::Restart ()
{
  if (reverse)
  {
    while (HasNextInternal())
    {
      NextInternal();
    }
  }
  else
  {
    for (size_t i = 0; i < llPaths.GetSize(); i++)
    {
      if (llPaths[i].IsValid())
      {
        llPaths[i]->Restart();
      }
    }
    currentllPosition = 0;
    hlPath->Restart();

    // Set current node
    csRef<iMapNode> goal = hlPath->Next();
    currentSector = firstNode->GetSector();
    if (!llPaths[0].IsValid())
    {
      // This should be reached if the user inverts the path and then restarts it, for example
      csRef<iCelNavMesh> navMesh = navMeshes.Get(currentSector, 0);
      csRef<iCelNavMeshPath> path = navMesh->ShortestPath(firstNode->GetPosition(), goal->GetPosition());
      llPaths[0] = path;    
    }

    currentNode = firstNode;
  }

  // reset travelled distance
  advanced = 0;
}

float celHPath::GetDistance () const
{
  return length-advanced;
}

csList<csSimpleRenderMesh>* celHPath::GetDebugMeshes ()
{
  float halfHeight = 1.f;
  csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >::GlobalIterator it = navMeshes.GetIterator();
  if (it.HasNext())
  {
    halfHeight = it.Next()->GetParameters()->GetAgentHeight() / 2.0;
  }

  int backCount = 0;
  while (HasPrevious())
  {
    Previous();
    backCount++;
  }

  csRef<iMapNode> current = Current();
  csRef<iMapNode> previous = Current();
  csVector3 currentPosition = current->GetPosition();
  csVector3 previousPosition = previous->GetPosition();  

  DebugDrawCS dd;
  dd.depthMask(false);

  const unsigned int lineCol = duRGBA(255, 255, 255, 230);
  const unsigned int vertexCol = duRGBA(255, 150, 0, 230);
  dd.begin(DU_DRAW_LINES, 4.0f);
  while (HasNext())
  {
    previous = current;
    previousPosition = currentPosition;
    current = Next();
    currentPosition = current->GetPosition();
    
    dd.vertex(previousPosition[0], previousPosition[1] + halfHeight, previousPosition[2], lineCol);
    dd.vertex(currentPosition[0], currentPosition[1] + halfHeight, currentPosition[2], lineCol);
  }
  dd.end();

  Restart();
  currentPosition = Current()->GetPosition();
  dd.begin(DU_DRAW_POINTS, 10.0f);
  dd.vertex(currentPosition[0], currentPosition[1] + halfHeight, currentPosition[2], vertexCol);
  while (HasNext())
  {
    currentPosition = Next()->GetPosition();
    dd.vertex(currentPosition[0], currentPosition[1] + halfHeight, currentPosition[2], vertexCol);
  }
  dd.end();

  dd.depthMask(true);
  Restart();
  for (int i = 0; i < backCount; i++)
  {
    Next();
  }

  return dd.GetMeshes();
}



/*
 * celHNavStruct
 */

celHNavStruct::celHNavStruct (const iCelNavMeshParams* params, iObjectRegistry* objectRegistry) : scfImplementationType (this)
{
  this->objectRegistry = objectRegistry;
  parameters.AttachNew(params->Clone());
}

celHNavStruct::~celHNavStruct ()
{
}

void celHNavStruct::AddNavMesh(iCelNavMesh* navMesh)
{
  csPtrKey<iSector> key = navMesh->GetSector();
  navMeshes.Put(key, navMesh);
}

/*
 * This method builds a high level graph, where each node represents a portal. If there is an
 * edge from node 'a' to 'b', then there is a path connecting the portals represented by these
 * nodes, and the length of this path is equal to the edges weight.
 */
bool celHNavStruct::BuildHighLevelGraph()
{
  hlGraph.Invalidate();
  hlGraph = scfCreateInstance<iCelGraph>("cel.celgraph");
  if (!hlGraph)
  {
    csApplicationFramework::ReportError("failed to create cel graph");
    return false;
  }

  csHash<csRef<iCelNode>, csPtrKey<iSector> > nodes;

  int x = 0;
  csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >::GlobalIterator it = navMeshes.GetIterator();
  while (it.HasNext())
  {
    csPtrKey<iSector> sector;
    it.Next(sector);
    csSet<csPtrKey<iMeshWrapper> >::GlobalIterator portalMeshesIt = sector->GetPortalMeshes().GetIterator();
    while (portalMeshesIt.HasNext())
    {
      csRef<iPortalContainer> container = portalMeshesIt.Next()->GetPortalContainer();
      int size = container->GetPortalCount();
      for (int i = 0; i < size; i++)
      {
        csRef<iPortal> portal = container->GetPortal(i);
        portal->CompleteSector(0);
        csFlags flags = portal->GetFlags();

        // Get portal indices
        int indicesSize = portal->GetVertexIndicesCount();

        // Get portal vertices
        int verticesSize = portal->GetVerticesCount();
        const csVector3* verticesPortal = portal->GetWorldVertices();

        if (indicesSize >= 3)
        {
          // Calculate portal's bounding box
          csVector3 boundingMin = verticesPortal[0];
          csVector3 boundingMax = verticesPortal[0];
          for (int j = 1; j < verticesSize; j++)
          {
            for (int k = 0; k < 3; k++)
            {
              if (verticesPortal[j][k] < boundingMin[k])
              {
                boundingMin[k] = verticesPortal[j][k];
              }
              if (verticesPortal[j][k] > boundingMax[k])
              {
                boundingMax[k] = verticesPortal[j][k];
              }
            }
          }

          // Add the portal bounding box's center as a node in the current sector to the graph
          csVector3 center(((boundingMin[0] + boundingMax[0]) / 2), boundingMin[1],
              ((boundingMin[2] + boundingMax[2]) / 2));
          csRef<iMapNode> mapNode;
          mapNode.AttachNew(new csMapNode("hlg"));
          mapNode->SetPosition(center);
          mapNode->SetSector(sector);
          size_t nodeIndex;
          csRef<iCelNode> node = hlGraph->CreateEmptyNode(nodeIndex);
          node->SetName("hlg");
          node->SetMapNode(mapNode);

          // Add the portal bounding box's center as a node in the destination sector to the graph
          // Warp if necessary
          csVector3 center2 = center;
          if (flags.Check(CS_PORTAL_WARP))
          {
            csReversibleTransform warp = portal->GetWarp();
            center2 = warp.Other2This(center);
          }
          csRef<iMapNode> mapNode2;
          mapNode2.AttachNew(new csMapNode("hlg"));
          mapNode2->SetPosition(center2);
          mapNode2->SetSector(portal->GetSector());
          csRef<iCelNode> node2 = hlGraph->CreateEmptyNode(nodeIndex);
          node2->SetName("hlg");
          node2->SetMapNode(mapNode2);

          // Add an edge between the two points
          hlGraph->AddEdge(node, node2, true, 0.0f);

          nodes.Put(sector, node);
          nodes.Put(portal->GetSector(), node2);
        }
      }
    }
    x++;
  }

  // Create edges between nodes from the same sector
  csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >::GlobalIterator it2 = navMeshes.GetIterator();
  while (it2.HasNext())
  {
    csPtrKey<iSector> sector;
    csRef<iCelNavMesh> navMesh = it2.Next(sector);
    if (!navMesh)
    {
      //return false;
      csApplicationFramework::ReportError("sector %s does not have an associated navmesh",
          sector->QueryObject()->GetName());
      continue;
    }
    csArray<csRef<iCelNode> > sectorNodes = nodes.GetAll(sector);
    int size = sectorNodes.GetSize();
    for (int i = 0; i < size - 1; i++)
    {
      csRef<iCelNode> node1 = sectorNodes[i];
      size_t count = 0;
      for (int j = i + 1; j < size; j++)
      {
        csRef<iCelNode> node2 = sectorNodes[j];
        csRef<iCelNavMeshPath> path = navMesh->ShortestPath(node1->GetPosition(), node2->GetPosition(), 256);
        if (path->GetNodeCount() > 0)
        {
          csVector3 last;
          path->GetLast(last);
          csVector3 box = parameters->GetPolygonSearchBox();
          // Check if last calculated point is within reach of the last given point
          if (ABS(last[0] - node2->GetPosition()[0]) <= box[0] && 
              ABS(last[1] - node2->GetPosition()[1]) <= box[1] &&
              ABS(last[2] - node2->GetPosition()[2]) <= box[2]) 
          {
            count++;
            float length = path->Length();
            hlGraph->AddEdge(node1, node2, true, length);
            hlGraph->AddEdge(node2, node1, true, length);
          }
        }
      }
      if(!count)
      {
          csPrintf("found unconnected high-level node at %s in %s\n", node1->GetPosition().Description().GetData(), sector->QueryObject()->GetName());
      }
    }
  }

  return true;
}

void celHNavStruct::SetHighLevelGraph(iCelGraph* graph)
{
  hlGraph = graph;
}

iCelHPath* celHNavStruct::ShortestPath (const csVector3& from, iSector* fromSector, const csVector3& goal,
    iSector* goalSector)
{
  csRef<iMapNode> fromNode;
  fromNode.AttachNew(new csMapNode("n"));
  fromNode->SetPosition(from);
  fromNode->SetSector(fromSector);

  csRef<iMapNode> goalNode;
  goalNode.AttachNew(new csMapNode("n"));
  goalNode->SetPosition(goal);
  goalNode->SetSector(goalSector);

  return ShortestPath(fromNode, goalNode);
}

/*
 * Finding the shortest path in a navigation structure is done in two steps:
 * 1- Add the origin and destination points of the path to the high level graph, and connect
 * them to edges in their sectors. Then, find a path in this graph.
 * 2- For each two adjascent nodes in the high level path, find the corresponding low level
 * path between those points, using the navigation mesh for their sector.
 * After that, the nodes and edges that were added to the graph can be removed.
 */
iCelHPath* celHNavStruct::ShortestPath (iMapNode* from, iMapNode* goal)
{
  // Add from and goal nodes to the high level graph
  size_t fromNodeIdx, goalNodeIdx;
  csRef<iCelNode> fromNode = hlGraph->CreateEmptyNode(fromNodeIdx);
  csRef<iCelNode> goalNode = hlGraph->CreateEmptyNode(goalNodeIdx);
  fromNode->SetMapNode(from);
  goalNode->SetMapNode(goal);

  // Create edges between from and goal nodes and all portals from their respective sectors
  csPtrKey<iSector> fromSector = from->GetSector();
  csPtrKey<iSector> goalSector = goal->GetSector();  
  csRef<iCelNavMesh> fromNavMesh = navMeshes.Get(fromSector, 0);
  csRef<iCelNavMesh> goalNavMesh = navMeshes.Get(goalSector, 0);  
  if (!fromNavMesh || !goalNavMesh)
  {
    hlGraph->RemoveNode(fromNodeIdx);
    hlGraph->RemoveNode(goalNodeIdx);
    return 0;
  }
  csList<csRef<iCelNode> > tmpEdgesOrigins; // Only for edges from some node to goal node
  csList<size_t> tmpEdgesIndices;
  size_t size = hlGraph->GetNodeCount();
  for (size_t i = 0; i < size; i++)
  {
    if (i != fromNodeIdx && i != goalNodeIdx)
    {
      csRef<iCelNode> node = hlGraph->GetNode(i);
      csRef<iMapNode> mapNode = node->GetMapNode();
      csRef<iSector> sector = mapNode->GetSector();
      // From node
      if(!sector)
          continue;
      if (sector->QueryObject()->GetID() == fromSector->QueryObject()->GetID())
      {
        csRef<iCelNavMeshPath> tmpPath = fromNavMesh->ShortestPath(from->GetPosition(), mapNode->GetPosition());
        if (tmpPath->GetNodeCount() > 0)
        {
          csVector3 last;
          tmpPath->GetLast(last);
          csVector3 box = parameters->GetPolygonSearchBox();
          // Check if last calculated point is within reach of the last given point
          if (ABS(last[0] - node->GetPosition()[0]) <= box[0] && 
              ABS(last[1] - node->GetPosition()[1]) <= box[1] &&
              ABS(last[2] - node->GetPosition()[2]) <= box[2]) 
          {
            hlGraph->AddEdge(fromNode, node, true, tmpPath->Length());
          }
        }
      }
      // Goal node
      if (sector->QueryObject()->GetID() == goalSector->QueryObject()->GetID())
      {
        csRef<iCelNavMeshPath> tmpPath = goalNavMesh->ShortestPath(mapNode->GetPosition(), goalNode->GetPosition());
        if (tmpPath->GetNodeCount() > 0)
        {
          csVector3 last;
          tmpPath->GetLast(last);
          csVector3 box = parameters->GetPolygonSearchBox();
          // Check if last calculated point is within reach of the last given point
          if (ABS(last[0] - goalNode->GetPosition()[0]) <= box[0] && 
              ABS(last[1] - goalNode->GetPosition()[1]) <= box[1] &&
              ABS(last[2] - goalNode->GetPosition()[2]) <= box[2]) 
          {
            tmpEdgesOrigins.PushBack(node);
            tmpEdgesIndices.PushBack(hlGraph->AddEdge(node, goalNode, true, tmpPath->Length()));
          }
        }
      }
    }
  }
  if (fromSector->QueryObject()->GetID() == goalSector->QueryObject()->GetID())
  {
    csRef<iCelNavMeshPath> tmpPath = goalNavMesh->ShortestPath(from->GetPosition(), goal->GetPosition());
    if (tmpPath->GetNodeCount() > 0)
    {
      csVector3 last;
      tmpPath->GetLast(last);
      csVector3 box = parameters->GetPolygonSearchBox();
      // Check if last calculated point is within reach of the last given point
      if (ABS(last[0] - goalNode->GetPosition()[0]) <= box[0] && 
          ABS(last[1] - goalNode->GetPosition()[1]) <= box[1] &&
          ABS(last[2] - goalNode->GetPosition()[2]) <= box[2]) 
      {
        hlGraph->AddEdge(fromNode, goalNode, true, tmpPath->Length());
      }
    }
  }

  // Find path in high level graph
  csRef<iCelPath> hlPath = scfCreateInstance<iCelPath>("cel.celpath");
  if (!hlPath)
  {
    csList<size_t>::Iterator itIndices(tmpEdgesIndices);
    csList<csRef<iCelNode> >::Iterator itOrigins(tmpEdgesOrigins);
    while (itIndices.HasNext()) // No need to check both iterators, since they have the same element count
    {
      hlGraph->RemoveEdge(itOrigins.Next(), itIndices.Next());
    }
    hlGraph->RemoveNode(fromNodeIdx);
    hlGraph->RemoveNode(goalNodeIdx);
    return 0;
  }
  hlGraph->ShortestPath2(fromNode, goalNode, hlPath);

  // Remove edges that connect any node to the goal node
  csList<size_t>::Iterator itIndices(tmpEdgesIndices);
  csList<csRef<iCelNode> >::Iterator itOrigins(tmpEdgesOrigins);
  while (itIndices.HasNext()) // No need to check both iterators, since they have the same element count
  {
    hlGraph->RemoveEdge(itOrigins.Next(), itIndices.Next());
  }

  // Remove from and goal nodes from the high level graph (and the edges we added and didn't explicitly remove)
  hlGraph->RemoveNode(goalNodeIdx);
  hlGraph->RemoveNode(fromNodeIdx);  

  if (hlPath->GetNodeCount() == 0)
  {
    return 0;
  }
  
  // Initialize path
  path.AttachNew(new celHPath(navMeshes));
  path->Initialize(hlPath);

  return path;
}

/*
 * In order to update the navigation structure, we have to update the navigation meshes for the
 * affected area, as well as the high level graph. When a navmesh gets updated, a path between
 * two portals may be closed or opened, and we have to add or remove edges to the graph to
 * account for this.
 */
bool celHNavStruct::Update (const csBox3& boundingBox, iSector* sector)
{
  if (!sector) 
  {
    bool ret;
    csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >::GlobalIterator it = navMeshes.GetIterator();
    csPtrKey<iSector> key;
    while (it.HasNext())
    {
      csRef<iCelNavMesh> navMesh = it.Next(key);
      csBox3 navMeshBoundingBox = navMesh->GetBoundingBox();
      if (boundingBox.Overlap(navMeshBoundingBox))
      {
        ret = Update(boundingBox, key);
        if (!ret)
        {
          return false;
        }
      }
    }
  }

  csPtrKey<iSector> key = sector;
  csRef<iCelNavMesh> navMesh = navMeshes.Get(key, 0);
  navMesh->Update(boundingBox);

  // Update high level graph
  int nNodes = hlGraph->GetNodeCount();
  csArray<csRef<iCelNode> > sameSectorNodes(hlGraph->GetNodeCount());
  uint sectorId = sector->QueryObject()->GetID();
  int sameSectorSize = 0;
  for (int i = 0; i < nNodes; ++i)
  {
    csRef<iCelNode> node = hlGraph->GetNode(i);
    uint nodeSectorId = node->GetMapNode()->GetSector()->QueryObject()->GetID();
    if (nodeSectorId == sectorId)
    {
      sameSectorNodes.Push(node);
      sameSectorSize++;
    }
  }
  for (int i = 0; i < sameSectorSize; ++i)
  {
    csRef<iCelNode> node = sameSectorNodes[i];

    // Check edges from the high level graph that are on the updated sector, to update weights and
    // see if any of them was obstructed.
    int nEdges = node->GetEdgeCount();
    for (int j = nEdges - 1; j > 0; j--)
    {
      csRef<iCelEdge> edge = node->GetEdge(j);
      csRef<iCelNode> node2 = edge->GetSuccessor();
      uint edgeSectorId = node2->GetMapNode()->GetSector()->QueryObject()->GetID();
      if (edgeSectorId == sectorId)
      {
        csRef<iCelNavMeshPath> path = navMesh->ShortestPath(node->GetPosition(), node2->GetPosition(), 256);
        if (path->GetNodeCount() > 0)
        {
          csVector3 last;
          path->GetLast(last);
          csVector3 box = parameters->GetPolygonSearchBox();
          // Check if last calculated point is within reach of the last given point
          if (ABS(last[0] - node2->GetPosition()[0]) <= box[0] && 
              ABS(last[1] - node2->GetPosition()[1]) <= box[1] &&
              ABS(last[2] - node2->GetPosition()[2]) <= box[2]) 
          {
            float length = path->Length();
            edge->SetWeight(length);
          }
          else
          {
            node->RemoveEdge(j);
          }
        }
        else
        {
          node->RemoveEdge(j);
        }
      }
    }

    // Check if any edges need to be added to the high level graph (a path might have opened)
    for (int j = i + 1; j < sameSectorSize; ++j)
    {
      csRef<iCelNode> node2 = sameSectorNodes[j];
      bool found = false;
      for (int k = 0; j < nEdges; ++k)
      {
        if (node2 == node->GetEdge(k)->GetSuccessor())
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        csRef<iCelNavMeshPath> path = navMesh->ShortestPath(node->GetPosition(), node2->GetPosition(), 256);
        if (path->GetNodeCount() > 0)
        {
          csVector3 last;
          path->GetLast(last);
          csVector3 box = parameters->GetPolygonSearchBox();
          // Check if last calculated point is within reach of the last given point
          if (ABS(last[0] - node2->GetPosition()[0]) <= box[0] && 
              ABS(last[1] - node2->GetPosition()[1]) <= box[1] &&
              ABS(last[2] - node2->GetPosition()[2]) <= box[2]) 
          {
            float length = path->Length();
            hlGraph->AddEdge(node, node2, true, length);
            hlGraph->AddEdge(node2, node, true, length);
          }
        }
      }
    }
  }

  return true;
}

bool celHNavStruct::Update (const csOBB& boundingBox, iSector* sector)
{
  csBox3 aabb;
  aabb.AddBoundingVertex(boundingBox.GetCorner(0));
  for (int i = 1; i < 8; i++)
  {
    aabb.AddBoundingVertexSmart(boundingBox.GetCorner(i));
  }

  return Update(aabb, sector);
}

void celHNavStruct::SaveParameters (iDocumentNode* node)
{
  csRef<iDocumentNode> param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("agentheight");
  param->SetAttributeAsFloat("value", parameters->GetAgentHeight());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("agentradius");
  param->SetAttributeAsFloat("value", parameters->GetAgentRadius());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("agentmaxslopeangle");
  param->SetAttributeAsFloat("value", parameters->GetAgentMaxSlopeAngle());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("agentmaxclimb");
  param->SetAttributeAsFloat("value", parameters->GetAgentMaxClimb());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("cellsize");
  param->SetAttributeAsFloat("value", parameters->GetCellSize());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("cellheight");
  param->SetAttributeAsFloat("value", parameters->GetCellHeight());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("maxsimplificationerror");
  param->SetAttributeAsFloat("value", parameters->GetMaxSimplificationError());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("detailsampledist");
  param->SetAttributeAsFloat("value", parameters->GetDetailSampleDist());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("detailsamplemaxerror");
  param->SetAttributeAsFloat("value", parameters->GetDetailSampleMaxError());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("maxedgelength");
  param->SetAttributeAsInt("value", parameters->GetMaxEdgeLength());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("minregionsize");
  param->SetAttributeAsInt("value", parameters->GetMinRegionSize());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("mergeregionsize");
  param->SetAttributeAsInt("value", parameters->GetMergeRegionSize());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("maxvertsperpoly");
  param->SetAttributeAsInt("value", parameters->GetMaxVertsPerPoly());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("tilesize");
  param->SetAttributeAsInt("value", parameters->GetTileSize());

  param = node->CreateNodeBefore(CS_NODE_ELEMENT);
  param->SetValue("bordersize");
  param->SetAttributeAsInt("value", parameters->GetBorderSize());
}

void celHNavStruct::SaveNavMeshes (iDocumentNode* node, iVFS* vfs)
{
  csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >::GlobalIterator navMeshIt = navMeshes.GetIterator();
  csPtrKey<iSector> key;
  int i = 1;
  csString fileName;
  while (navMeshIt.HasNext())
  {
    csRef<iCelNavMesh> navMesh = navMeshIt.Next(key);
    csRef<iDocumentNode> navMeshNode = node->CreateNodeBefore(CS_NODE_ELEMENT);
    navMeshNode->SetValue("navmesh");
    navMeshNode->SetAttributeAsInt("id", i);
    navMeshNode->SetAttribute("sector", key->QueryObject()->GetName());
    fileName = i;
    csRef<iFile> meshFile = vfs->Open(fileName.GetDataSafe(), VFS_FILE_WRITE);
    navMesh->SaveToFile(meshFile);
    i++;
  }
}

void celHNavStruct::SaveHighLevelGraph (iDocumentNode* node1, iDocumentNode* node2)
{
  // Nodes
  size_t size = hlGraph->GetNodeCount();
  for (size_t i = 0; i < size; i++)
  {
    csRef<iCelNode> graphNode = hlGraph->GetNode(i);
    csRef<iDocumentNode> n = node1->CreateNodeBefore(CS_NODE_ELEMENT);
    n->SetValue("node");
    n->SetAttributeAsInt("id", i);
    csVector3 position = graphNode->GetPosition();
    n->SetAttributeAsFloat("x", position[0]);
    n->SetAttributeAsFloat("y", position[1]);
    n->SetAttributeAsFloat("z", position[2]);
    n->SetAttribute("sector", graphNode->GetMapNode()->GetSector()->QueryObject()->GetName());
    csString name;
    name = i;    
    graphNode->SetName(name.GetDataSafe());
  }

  // Edges
  for (size_t i = 0; i < size; i++)
  {
    csRef<iCelNode> graphNode = hlGraph->GetNode(i);
    csRefArray<iCelEdge> edges = graphNode->GetEdges();
    for (size_t j = 0; j < edges.GetSize(); j++)
    {
      csRef<iDocumentNode> e = node2->CreateNodeBefore(CS_NODE_ELEMENT);
      e->SetValue("edge");
      e->SetAttribute("from", graphNode->GetName());
      e->SetAttribute("to", edges[j]->GetSuccessor()->GetName());
      e->SetAttributeAsFloat("weight", edges[j]->GetWeight());
    }    
  }
}

bool celHNavStruct::SaveToFile (iVFS* vfs, const char* directory)
{
  csRef<iDocumentSystem> docsys = csLoadPluginCheck<iDocumentSystem>(objectRegistry, "crystalspace.documentsystem.tinyxml");
  if (!docsys)
  {
    return false;
  }

  csString workingDir(vfs->GetCwd());
  vfs->ChDir(directory);

  // Create XML file
  csRef<iDocument> doc = docsys->CreateDocument();
  csRef<iDocumentNode> root = doc->CreateRoot();
  csRef<iDocumentNode> mainNode = root->CreateNodeBefore(CS_NODE_ELEMENT);
  mainNode->SetValue("iCelHNavStruct");

  // Create parameters subtree
  csRef<iDocumentNode> params = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
  params->SetValue("parameters");
  SaveParameters(params);

  // Create navmeshes subtree
  csRef<iDocumentNode> navmeshes = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
  navmeshes->SetValue("navmeshes");
  SaveNavMeshes(navmeshes, vfs);

  // Create highlevelgraph subtree
  csRef<iDocumentNode> hlgraph = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
  hlgraph->SetValue("highlevelgraph");
  csRef<iDocumentNode> nodes = hlgraph->CreateNodeBefore(CS_NODE_ELEMENT);
  nodes->SetValue("nodes");
  csRef<iDocumentNode> edges = hlgraph->CreateNodeBefore(CS_NODE_ELEMENT);
  edges->SetValue("edges");
  SaveHighLevelGraph(nodes, edges);

  // Write xml file
  const char* log = doc->Write(vfs, "navstruct.xml");
  vfs->ChDir(workingDir.GetDataSafe());
  vfs->Sync();

  if (log)
  {    
    return false;
  }

  return true;
}

const iCelNavMeshParams* celHNavStruct::GetNavMeshParams () const
{
  return parameters;
}

csList<csSimpleRenderMesh>* celHNavStruct::GetDebugMeshes () const
{
  csList<csSimpleRenderMesh>* meshes = new csList<csSimpleRenderMesh>();
  csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >::ConstGlobalIterator it = navMeshes.GetIterator();
  while (it.HasNext())
  {
    csRef<iCelNavMesh> navMesh = it.Next();
    csList<csSimpleRenderMesh>* tmp = navMesh->GetDebugMeshes();
    csList<csSimpleRenderMesh>::Iterator tmpIt(*tmp);
    while (tmpIt.HasNext())
    {
      meshes->PushBack(tmpIt.Next());
    }
    delete tmp;
  }
  return meshes;
}

csList<csSimpleRenderMesh>* celHNavStruct::GetAgentDebugMeshes (const csVector3& pos, int red, int green,
                                                                int blue, int alpha) const
{
  csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >::ConstGlobalIterator it = navMeshes.GetIterator();
  if (it.HasNext())
  {
    csRef<iCelNavMesh> navMesh = it.Next();
    return navMesh->GetAgentDebugMeshes(pos, red, green, blue, alpha);
  }
  return 0;
}



/*
 * celHNavStructBuilder
 */

SCF_IMPLEMENT_FACTORY (celHNavStructBuilder)

celHNavStructBuilder::celHNavStructBuilder (iBase* parent) : scfImplementationType (this, parent)
{
  sectors = 0;
  parameters.AttachNew(new celNavMeshParams());
}

celHNavStructBuilder::~celHNavStructBuilder ()
{
  delete sectors;
}

bool celHNavStructBuilder::Initialize (iObjectRegistry* objectRegistry) 
{
  this->objectRegistry = objectRegistry;
  return true;
}

bool celHNavStructBuilder::SetSectors (csList<iSector*> sectorList)
{
  delete sectors;
  sectors = new csList<iSector*>(sectorList);
  builders.Empty();
  return InstantiateNavMeshBuilders();
}

bool celHNavStructBuilder::InstantiateNavMeshBuilders ()
{
  csList<iSector*>::Iterator it(*sectors);
  while (it.HasNext())
  {
    csPtrKey<iSector> key = it.Next();
    CS_ASSERT(static_cast<iSector*>(key));
    csRef<iCelNavMeshBuilder> builder = csLoadPlugin<iCelNavMeshBuilder>(objectRegistry, "cel.navmeshbuilder");
    if (!builder)
    {
      return false;
    }
    builder->SetNavMeshParams(parameters);
    builder->SetSector(key);
    builders.Put(key, builder);
  }
  return true;
}

iCelHNavStruct* celHNavStructBuilder::BuildHNavStruct ()
{
  if (!sectors)
  {
    return 0;
  }

  navStruct.AttachNew(new celHNavStruct(parameters, objectRegistry));

  csHash<csRef<iCelNavMeshBuilder>, csPtrKey<iSector> >::GlobalIterator it = builders.GetIterator();
  csRefArray<iThreadReturn> results;
  while (it.HasNext())
  {
    csRef<iCelNavMeshBuilder> builder = it.Next();
    results.Push(builder->BuildNavMesh());
  }

  while(!results.IsEmpty())
  {
    for(size_t i = 0; i < results.GetSize(); i++)
    {
      csRef<iThreadReturn> ret = results.Get(i);
      if(ret->IsFinished())
      {
        if(ret->WasSuccessful())
        {
          csRef<iCelNavMesh> mesh = scfQueryInterface<iCelNavMesh>(ret->GetResultRefPtr());
          navStruct->AddNavMesh(mesh);
        }
        results.DeleteIndex(i);
        i--;
      }
    }
  }

  navStruct->BuildHighLevelGraph();

  return navStruct;
}

bool celHNavStructBuilder::ParseParameters (iDocumentNode* node, iCelNavMeshParams* params)
{
  csRef<iDocumentNode> param = node->GetNode("agentheight");
  float value = param->GetAttributeValueAsFloat("value");
  params->SetAgentHeight(value);

  param = node->GetNode("agentradius");
  value = param->GetAttributeValueAsFloat("value");
  params->SetAgentRadius(value);

  param = node->GetNode("agentmaxslopeangle");
  value = param->GetAttributeValueAsFloat("value");
  params->SetAgentMaxSlopeAngle(value);

  param = node->GetNode("agentmaxclimb");
  value = param->GetAttributeValueAsFloat("value");
  params->SetAgentMaxClimb(value);

  param = node->GetNode("cellsize");
  value = param->GetAttributeValueAsFloat("value");
  params->SetCellSize(value);

  param = node->GetNode("cellheight");
  value = param->GetAttributeValueAsFloat("value");
  params->SetCellHeight(value);

  param = node->GetNode("maxsimplificationerror");
  value = param->GetAttributeValueAsFloat("value");
  params->SetMaxSimplificationError(value);

  param = node->GetNode("detailsampledist");
  value = param->GetAttributeValueAsFloat("value");
  params->SetDetailSampleDist(value);

  param = node->GetNode("detailsamplemaxerror");
  value = param->GetAttributeValueAsFloat("value");
  params->SetDetailSampleMaxError(value);

  param = node->GetNode("maxedgelength");
  int value2 = param->GetAttributeValueAsInt("value");
  params->SetMaxEdgeLength(value2);

  param = node->GetNode("minregionsize");
  value2 = param->GetAttributeValueAsInt("value");
  params->SetMinRegionSize(value2);

  param = node->GetNode("mergeregionsize");
  value2 = param->GetAttributeValueAsInt("value");
  params->SetMergeRegionSize(value2);

  param = node->GetNode("maxvertsperpoly");
  value2 = param->GetAttributeValueAsInt("value");
  params->SetMaxVertsPerPoly(value2);

  param = node->GetNode("tilesize");
  value2 = param->GetAttributeValueAsInt("value");
  params->SetTileSize(value2);

  param = node->GetNode("bordersize");
  value2 = param->GetAttributeValueAsInt("value");
  params->SetBorderSize(value2);

  return true;
}

bool celHNavStructBuilder::ParseMeshes (iDocumentNode* node, csHash<csRef<iSector>, const char*>& sectors, 
                                        celHNavStruct* navStruct, iVFS* vfs, iCelNavMeshParams* /*params*/)
{
  csRef<iEngine> engine = csLoadPluginCheck<iEngine>(objectRegistry, "crystalspace.engine.3d");
  if (!engine)
  {
    return false;
  }

  csRef<iDocumentNodeIterator> it = node->GetNodes("navmesh");
  while (it->HasNext())
  {
    csRef<iDocumentNode> navMeshNode = it->Next();

    // Get sector
    const char* sectorName = navMeshNode->GetAttributeValue("sector");
    csString sectorNameString(sectorName);
    if(!sectors.In(sectorName))
    {
      iSector* sector = engine->FindSector(sectorName);
      if(sector)
      {
          sectors.Put(sectorName, sector);
      }
      else
      {
          csString msg;
          msg.Format("invalid sector %s in navmesh\n", sectorName);
          CS_ASSERT_MSG(msg.GetData(),false);
      }
    }

    // Get navmesh
    int id = navMeshNode->GetAttributeValueAsInt("id");
    csString fileName;
    fileName = id;
    csRef<iFile> file = vfs->Open(fileName.GetDataSafe(), VFS_FILE_READ);
    csRef<iCelNavMeshBuilder> builder = csLoadPluginCheck<iCelNavMeshBuilder>(objectRegistry, "cel.navmeshbuilder");
    csRef<iCelNavMesh> navMesh = builder->LoadNavMesh(file);
    navStruct->AddNavMesh(navMesh);
  }
  return true;
}

bool celHNavStructBuilder::ParseGraph (iDocumentNode* node, iCelGraph* graph, csHash<csRef<iSector>, const char*> sectors)
{
  csRef<iDocumentNode> nodesNode = node->GetNode("nodes");
  csRef<iDocumentNodeIterator> it = nodesNode->GetNodes("node");
  size_t nodeCount = it->GetEndPosition();
  csArray<csRef<iCelNode> > celNodes(nodeCount);
  celNodes.SetSize(nodeCount);
  while (it->HasNext())
  {
    csRef<iDocumentNode> graphNode = it->Next();
    size_t id = graphNode->GetAttributeValueAsInt("id");

    // Get sector
    const char* sectorName = graphNode->GetAttributeValue("sector");
    csRef<iSector> sector = sectors.Get(sectorName, 0);
    if(!sector.IsValid())
    {
        csString msg;
        msg.Format("invalid sector %s in navmesh graph node\n", sectorName);
        CS_ASSERT_MSG(msg.GetData(),false);
    }

    // Get position
    csVector3 position;
    float x = graphNode->GetAttributeValueAsFloat("x");
    float y = graphNode->GetAttributeValueAsFloat("y");
    float z = graphNode->GetAttributeValueAsFloat("z");
    position.Set(x, y, z);
    
    // Crete node
    csRef<iCelNode> node = graph->CreateEmptyNode(id);
    celNodes[id] = node;
    csRef<iMapNode> mapNode;
    mapNode.AttachNew(new csMapNode("n"));
    mapNode->SetPosition(position);
    mapNode->SetSector(sector);
    node->SetMapNode(mapNode);
  }

  csRef<iDocumentNode> edgesNode = node->GetNode("edges");
  it = edgesNode->GetNodes("edge");
  while (it->HasNext())
  {
    csRef<iDocumentNode> graphEdge = it->Next();
    int from = graphEdge->GetAttributeValueAsInt("from");
    int to = graphEdge->GetAttributeValueAsInt("to");
    float weight = graphEdge->GetAttributeValueAsFloat("weight");
    celNodes[from]->AddSuccessor(celNodes[to], true, weight);
  }

  return true;
}

iCelHNavStruct* celHNavStructBuilder::LoadHNavStruct (iVFS* vfs, const char* directory)
{
  csRef<iDocumentSystem> docsys = csLoadPluginCheck<iDocumentSystem>(objectRegistry, "crystalspace.documentsystem.tinyxml");
  if (!docsys)
  {
    return 0;
  }

  // Mount file
  csString workingDir(vfs->GetCwd());
  vfs->ChDir(directory);

  // Read XML file
  csRef<iDocument> doc = docsys->CreateDocument();
  csRef<iFile> xmlFile = vfs->Open("navstruct.xml", VFS_FILE_READ);
  const char* log = doc->Parse(xmlFile);
  if (log)
  {
    return 0;
  }
  csRef<iDocumentNode> root = doc->GetRoot();
  csRef<iDocumentNode> mainNode = root->GetNode("iCelHNavStruct");

  // Read parameters
  csRef<iDocumentNode> paramsNode = mainNode->GetNode("parameters");
  csRef<iCelNavMeshParams> params;
  params.AttachNew(new celNavMeshParams());
  ParseParameters(paramsNode, params);
  
  // Create navigation structure
  csRef<celHNavStruct> navStruct;
  navStruct.AttachNew(new celHNavStruct(params, objectRegistry));

  // Read navigation meshes
  csRef<iDocumentNode> meshesNode = mainNode->GetNode("navmeshes");
  csHash<csRef<iSector>, const char*> sectors;
  if(!ParseMeshes(meshesNode, sectors, navStruct, vfs, params))
  {
      return false;
  }

  // Read high level graph
  csRef<iDocumentNode> graphNode = mainNode->GetNode("highlevelgraph");
  csRef<iCelGraph> graph = scfCreateInstance<iCelGraph>("cel.celgraph");
  if(!ParseGraph(graphNode, graph, sectors))
  {
      return false;
  }
  navStruct->SetHighLevelGraph(graph);

  this->navStruct = navStruct;
  vfs->ChDir(workingDir.GetDataSafe());

  return navStruct;
}

const iCelNavMeshParams* celHNavStructBuilder::GetNavMeshParams () const
{
  return parameters;
}

void celHNavStructBuilder::SetNavMeshParams (const iCelNavMeshParams* parameters)
{
  this->parameters.AttachNew(new celNavMeshParams(parameters));
  if (sectors)
  {
    csHash<csRef<iCelNavMeshBuilder>, csPtrKey<iSector> >::GlobalIterator it = builders.GetIterator();
    while (it.HasNext())
    {
      it.Next()->SetNavMeshParams(parameters);
    }    
  }
}

} CS_PLUGIN_NAMESPACE_END(celNavMesh)
