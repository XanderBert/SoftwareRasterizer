#pragma once
#include <vector>

namespace dae
{
	struct Mesh;
	class ITriangleIndicesIterator
	{
	public:

		ITriangleIndicesIterator(const std::vector<uint32_t>& meshIndices) : m_MeshIndices(meshIndices) {}
		virtual ~ITriangleIndicesIterator() = default;

		ITriangleIndicesIterator(const ITriangleIndicesIterator& other) = delete;
		ITriangleIndicesIterator(ITriangleIndicesIterator&& other) noexcept = delete;
		ITriangleIndicesIterator& operator=(const ITriangleIndicesIterator& other) = delete;
		ITriangleIndicesIterator& operator=(ITriangleIndicesIterator&& other) noexcept = delete;
		
		virtual bool HasNext() const = 0;
		virtual std::vector<uint32_t> Next() = 0;
		virtual std::vector<uint32_t> GetVertexIndexesAtIndex(uint32_t index) = 0;
		virtual void ResetIndex() = 0;
			
	protected:
		int m_CurrentTriangleIndex{0};
		std::vector<uint32_t> m_MeshIndices{};
	};

	class TriangleListIterator final : public ITriangleIndicesIterator
	{
	public:
		TriangleListIterator(const std::vector<uint32_t>& meshIndices) : ITriangleIndicesIterator(meshIndices) {}
		~TriangleListIterator() override = default;


		TriangleListIterator(const TriangleListIterator& other) = delete;
		TriangleListIterator(TriangleListIterator&& other) noexcept = delete;
		TriangleListIterator& operator=(const TriangleListIterator& other) = delete;
		TriangleListIterator& operator=(TriangleListIterator&& other) noexcept = delete;
		
		bool HasNext() const override;
		std::vector<uint32_t> Next() override;
		std::vector<uint32_t> GetVertexIndexesAtIndex(uint32_t index) override;
		
		void ResetIndex() override;
	};



	class TriangleStripIterator final : public ITriangleIndicesIterator
	{
	public:
		TriangleStripIterator(const std::vector<uint32_t>& meshIndices) : ITriangleIndicesIterator(meshIndices) {}
		~TriangleStripIterator() override = default;

		TriangleStripIterator(const TriangleStripIterator& other) = delete;
		TriangleStripIterator(TriangleStripIterator&& other) noexcept = delete;
		TriangleStripIterator& operator=(const TriangleStripIterator& other) = delete;
		TriangleStripIterator& operator=(TriangleStripIterator&& other) noexcept = delete;
		
		
		bool HasNext() const override;
		std::vector<uint32_t> Next() override;
		std::vector<uint32_t> GetVertexIndexesAtIndex(uint32_t index) override;
		
		void ResetIndex() override;
	};
}
