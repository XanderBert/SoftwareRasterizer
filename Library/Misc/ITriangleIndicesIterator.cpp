#include "ITriangleIndicesIterator.h"
#include "../src/DataTypes.h"


using namespace dae;

bool TriangleListIterator::HasNext() const
{
    return m_CurrentTriangleIndex < m_MeshIndices.size();
			
}

std::vector<uint32_t> TriangleListIterator::Next()
{
    auto vertices = GetVertexIndexesAtIndex(m_CurrentTriangleIndex);
			
    m_CurrentTriangleIndex += 3;

    return vertices;
		
}

//Todo change to pass vector of indices
std::vector<uint32_t> TriangleListIterator::GetVertexIndexesAtIndex(uint32_t index)
{
    //todo do out of range checks

			
    std::vector<uint32_t> triangleIndexes{};
    triangleIndexes.resize(3);
    
    
    triangleIndexes[0] = { m_MeshIndices[index] };
    triangleIndexes[1] = { m_MeshIndices[index + 1] };
    triangleIndexes[2] = { m_MeshIndices[index + 2] };
			
    return triangleIndexes;
}

void TriangleListIterator::ResetIndex()
{
    m_CurrentTriangleIndex = 0;
}











bool TriangleStripIterator::HasNext() const
{
    return m_CurrentTriangleIndex < m_MeshIndices.size() - 2;	
}

std::vector<uint32_t> TriangleStripIterator::Next()
{
    auto vertices = GetVertexIndexesAtIndex(m_CurrentTriangleIndex);
    ++m_CurrentTriangleIndex;
    return vertices;
}

std::vector<uint32_t> TriangleStripIterator::GetVertexIndexesAtIndex(uint32_t index)
{
    const bool swapVertices = index % 2;

    std::vector<uint32_t> triangleIndexes{};
    triangleIndexes.resize(3);
    
    
    triangleIndexes[0] = { m_MeshIndices[index] };
    triangleIndexes[1] = { m_MeshIndices[index + 1 * !swapVertices + 2 * swapVertices] };
    triangleIndexes[2] = { m_MeshIndices[index + 2 * !swapVertices + 1 * swapVertices] };
			
    return triangleIndexes;			
}

void TriangleStripIterator::ResetIndex()
{
    m_CurrentTriangleIndex = 0;
}
