/*
(C) Sebastian Aaltonen 2023

Modifications made by Eero Mutka, 08/2023:
* converted the library to C and to work with the fire headers

-- MIT License ----------------------------------------------------------

Copyright (c) 2023 Sebastian Aaltonen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

typedef U32 oaNodeIndex;

#define OA_NUM_TOP_BINS 32
#define OA_BINS_PER_LEAF 8
#define OA_TOP_BINS_INDEX_SHIFT 3
#define OA_LEAF_BINS_INDEX_MASK 0x7
#define OA_NUM_LEAF_BINS (OA_NUM_TOP_BINS * OA_BINS_PER_LEAF)

#define OA_ALLOCATION_NO_SPACE 0xffffffff
typedef struct oaAllocation {
	U32 offset; // may be OA_ALLOCATION_NO_SPACE
	oaNodeIndex metadata; // internal: node index
} oaAllocation;
static const oaAllocation oaAllocation_default = {OA_ALLOCATION_NO_SPACE, OA_ALLOCATION_NO_SPACE};

typedef struct oaStorageReport {
	uint32_t totalFreeSpace;
	uint32_t largestFreeRegion;
} oaStorageReport;

typedef struct oaRegion {
	uint32_t size;
	uint32_t count;
} oaRegion;

typedef struct oaStorageReportFull {
	oaRegion freeRegions[OA_NUM_LEAF_BINS];
} oaStorageReportFull;

#define OA_NODE_UNUSED 0xFFFFFFFF
typedef struct oaNode {
	U32 dataOffset;
	U32 dataSize;
	oaNodeIndex binListPrev;
	oaNodeIndex binListNext;
	oaNodeIndex neighborPrev;
	oaNodeIndex neighborNext;
	bool used; // TODO: Merge as bit flag
} oaNode;
static const oaNode oaNode_default = {0, 0, OA_NODE_UNUSED, OA_NODE_UNUSED, OA_NODE_UNUSED, OA_NODE_UNUSED, false};

typedef struct oaAllocator {
	U32 m_size;
	U32 m_maxAllocs;
	U32 m_freeStorage;

	U32 m_usedBinsTop;
	U8 m_usedBins[OA_NUM_TOP_BINS];
	oaNodeIndex m_binIndices[OA_NUM_LEAF_BINS];

	oaNode* m_nodes;
	oaNodeIndex* m_freeNodes;
	U32 m_freeOffset;
} oaAllocator;

static oaAllocator oa_init(U32 size, oaNode* nodes, oaNodeIndex* freeNodes, U32 maxAllocs);
static void oa_reset(oaAllocator* allocator);

static oaAllocation oa_allocate(oaAllocator* allocator, U32 size);
static void oa_free(oaAllocator* allocator, oaAllocation allocation);

static U32 oa_allocation_size(oaAllocator* allocator, oaAllocation allocation);
static oaStorageReport oa_storage_report(oaAllocator* allocator);


#ifdef DEBUG
#include <assert.h>
#define ASSERT(x) assert(x)
//#define DEBUG_VERBOSE
#else
#define ASSERT(x)
#endif

#ifdef DEBUG_VERBOSE
#include <stdio.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#endif

//#include <cstring>

inline U32 lzcnt_nonzero(U32 v)
{
#ifdef _MSC_VER
	unsigned long retVal;
	_BitScanReverse(&retVal, v);
	return 31 - retVal;
#else
	return __builtin_clz(v);
#endif
}

inline U32 tzcnt_nonzero(U32 v)
{
#ifdef _MSC_VER
	unsigned long retVal;
	_BitScanForward(&retVal, v);
	return retVal;
#else
	return __builtin_ctz(v);
#endif
}

#define SMALL_FLOAT_MANTISSA_BITS 3
#define SMALL_FLOAT_MANTISSA_VALUE (1 << SMALL_FLOAT_MANTISSA_BITS)
#define SMALL_FLOAT_MANTISSA_MASK  (SMALL_FLOAT_MANTISSA_VALUE - 1)
		
// Bin sizes follow floating point (exponent + mantissa) distribution (piecewise linear log approx)
// This ensures that for each size class, the average overhead percentage stays the same
static U32 uintToFloatRoundUp(U32 size) {
	U32 exp = 0;
	U32 mantissa = 0;
			
	if (size < SMALL_FLOAT_MANTISSA_VALUE)
	{
		// Denorm: 0..(MANTISSA_VALUE-1)
		mantissa = size;
	}
	else
	{
		// Normalized: Hidden high bit always 1. Not stored. Just like float.
		U32 leadingZeros = lzcnt_nonzero(size);
		U32 highestSetBit = 31 - leadingZeros;
				
		U32 mantissaStartBit = highestSetBit - SMALL_FLOAT_MANTISSA_BITS;
		exp = mantissaStartBit + 1;
		mantissa = (size >> mantissaStartBit) & SMALL_FLOAT_MANTISSA_MASK;
				
		U32 lowBitsMask = (1 << mantissaStartBit) - 1;
				
		// Round up!
		if ((size & lowBitsMask) != 0)
			mantissa++;
	}
			
	return (exp << SMALL_FLOAT_MANTISSA_BITS) + mantissa; // + allows mantissa->exp overflow for round up
}

static U32 uintToFloatRoundDown(U32 size) {
	U32 exp = 0;
	U32 mantissa = 0;
			
	if (size < SMALL_FLOAT_MANTISSA_VALUE)
	{
		// Denorm: 0..(MANTISSA_VALUE-1)
		mantissa = size;
	}
	else
	{
		// Normalized: Hidden high bit always 1. Not stored. Just like float.
		U32 leadingZeros = lzcnt_nonzero(size);
		U32 highestSetBit = 31 - leadingZeros;
				
		U32 mantissaStartBit = highestSetBit - SMALL_FLOAT_MANTISSA_BITS;
		exp = mantissaStartBit + 1;
		mantissa = (size >> mantissaStartBit) & SMALL_FLOAT_MANTISSA_MASK;
	}
			
	return (exp << SMALL_FLOAT_MANTISSA_BITS) | mantissa;
}
		
static U32 floatToUint(U32 floatValue) {
	U32 exponent = floatValue >> SMALL_FLOAT_MANTISSA_BITS;
	U32 mantissa = floatValue & SMALL_FLOAT_MANTISSA_MASK;
	if (exponent == 0)
	{
		// Denorms
		return mantissa;
	}
	else
	{
		return (mantissa | SMALL_FLOAT_MANTISSA_VALUE) << (exponent - 1);
	}
}

// Utility functions
static U32 findLowestSetBitAfter(U32 bitMask, U32 startBitIndex) {
	U32 maskBeforeStartIndex = (1 << startBitIndex) - 1;
	U32 maskAfterStartIndex = ~maskBeforeStartIndex;
	U32 bitsAfter = bitMask & maskAfterStartIndex;
	if (bitsAfter == 0) return OA_ALLOCATION_NO_SPACE;
	return tzcnt_nonzero(bitsAfter);
}

static oaAllocator oa_init(U32 size, oaNode* nodes, oaNodeIndex* freeNodes, U32 maxAllocs) {
	oaAllocator result = {.m_size = size, .m_maxAllocs = maxAllocs};
	if (sizeof(oaNodeIndex) == 2)
	{
		ASSERT(maxAllocs <= 65536);
	}
	
	result.m_nodes = nodes; //MemAllocN(oaNode, maxAllocs, arena);
	result.m_freeNodes = freeNodes; //MemAllocN(oaNodeIndex, maxAllocs, arena);

	oa_reset(&result);
	return result;
}

//Allocator::Allocator(Allocator &&other) :
//	m_size(other.m_size),
//	m_maxAllocs(other.m_maxAllocs),
//	m_freeStorage(other.m_freeStorage),
//	m_usedBinsTop(other.m_usedBinsTop),
//	m_nodes(other.m_nodes),
//	m_freeNodes(other.m_freeNodes),
//	m_freeOffset(other.m_freeOffset)
//{
//	memcpy(m_usedBins, other.m_usedBins, sizeof(uint8) * NUM_TOP_BINS);
//	memcpy(m_binIndices, other.m_binIndices, sizeof(NodeIndex) * NUM_LEAF_BINS);
//
//	other.m_nodes = nullptr;
//	other.m_freeNodes = nullptr;
//	other.m_freeOffset = 0;
//	other.m_maxAllocs = 0;
//	other.m_usedBinsTop = 0;
//}

static U32 oa_insert_node_into_bin(oaAllocator* allocator, U32 size, U32 dataOffset) {
	// Round down to bin index to ensure that bin >= alloc
	U32 binIndex = uintToFloatRoundDown(size);

	U32 topBinIndex = binIndex >> OA_TOP_BINS_INDEX_SHIFT;
	U32 leafBinIndex = binIndex & OA_LEAF_BINS_INDEX_MASK;

	// Bin was empty before?
	if (allocator->m_binIndices[binIndex] == OA_NODE_UNUSED)
	{
		// Set bin mask bits
		allocator->m_usedBins[topBinIndex] |= 1 << leafBinIndex;
		allocator->m_usedBinsTop |= 1 << topBinIndex;
	}

	// Take a freelist node and insert on top of the bin linked list (next = old top)
	U32 topNodeIndex = allocator->m_binIndices[binIndex];
	U32 nodeIndex = allocator->m_freeNodes[allocator->m_freeOffset--];
#ifdef DEBUG_VERBOSE
	printf("Getting node %u from freelist[%u]\n", nodeIndex, m_freeOffset + 1);
#endif
	
	oaNode node = oaNode_default;
	node.dataOffset = dataOffset;
	node.dataSize = size;
	node.binListNext = topNodeIndex;
	allocator->m_nodes[nodeIndex] = node;

	if (topNodeIndex != OA_NODE_UNUSED) allocator->m_nodes[topNodeIndex].binListPrev = nodeIndex;
	allocator->m_binIndices[binIndex] = nodeIndex;

	allocator->m_freeStorage += size;
#ifdef DEBUG_VERBOSE
	printf("Free storage: %u (+%u) (insertNodeIntoBin)\n", m_freeStorage, size);
#endif

	return nodeIndex;
}

static void oa_reset(oaAllocator* allocator) {
	allocator->m_freeStorage = 0;
	allocator->m_usedBinsTop = 0;
	allocator->m_freeOffset = allocator->m_maxAllocs - 1;

	for (U32 i = 0 ; i < OA_NUM_TOP_BINS; i++)
		allocator->m_usedBins[i] = 0;
		
	for (U32 i = 0 ; i < OA_NUM_LEAF_BINS; i++)
		allocator->m_binIndices[i] = OA_NODE_UNUSED;
	
	// Freelist is a stack. Nodes in inverse order so that [0] pops first.
	for (U32 i = 0; i < allocator->m_maxAllocs; i++)
	{
		allocator->m_nodes[i] = oaNode_default;
		allocator->m_freeNodes[i] = allocator->m_maxAllocs - i - 1;
	}
		
	// Start state: Whole storage as one big node
	// Algorithm will split remainders and push them back as smaller nodes
	oa_insert_node_into_bin(allocator, allocator->m_size, 0);
}

//Allocator::~Allocator()
//{		
//	delete[] m_nodes;
//	delete[] m_freeNodes;
//}
		
static oaAllocation oa_allocate(oaAllocator* allocator, U32 size) {
	// Out of allocations?
	if (allocator->m_freeOffset == 0)
	{
		return (oaAllocation){.offset = OA_ALLOCATION_NO_SPACE, .metadata = OA_ALLOCATION_NO_SPACE};
	}
		
	// Round up to bin index to ensure that alloc >= bin
	// Gives us min bin index that fits the size
	U32 minBinIndex = uintToFloatRoundUp(size);
		
	U32 minTopBinIndex = minBinIndex >> OA_TOP_BINS_INDEX_SHIFT;
	U32 minLeafBinIndex = minBinIndex & OA_LEAF_BINS_INDEX_MASK;
		
	U32 topBinIndex = minTopBinIndex;
	U32 leafBinIndex = OA_ALLOCATION_NO_SPACE;

	// If top bin exists, scan its leaf bin. This can fail (NO_SPACE).
	if (allocator->m_usedBinsTop & (1 << topBinIndex))
	{
		leafBinIndex = findLowestSetBitAfter(allocator->m_usedBins[topBinIndex], minLeafBinIndex);
	}
		
	// If we didn't find space in top bin, we search top bin from +1
	if (leafBinIndex == OA_ALLOCATION_NO_SPACE)
	{
		topBinIndex = findLowestSetBitAfter(allocator->m_usedBinsTop, minTopBinIndex + 1);
			
		// Out of space?
		if (topBinIndex == OA_ALLOCATION_NO_SPACE)
		{
			return (oaAllocation){.offset = OA_ALLOCATION_NO_SPACE, .metadata = OA_ALLOCATION_NO_SPACE};
		}

		// All leaf bins here fit the alloc, since the top bin was rounded up. Start leaf search from bit 0.
		// NOTE: This search can't fail since at least one leaf bit was set because the top bit was set.
		leafBinIndex = tzcnt_nonzero(allocator->m_usedBins[topBinIndex]);
	}
				
	U32 binIndex = (topBinIndex << OA_TOP_BINS_INDEX_SHIFT) | leafBinIndex;
		
	// Pop the top node of the bin. Bin top = node.next.
	U32 nodeIndex = allocator->m_binIndices[binIndex];
	oaNode* node = &allocator->m_nodes[nodeIndex];
	U32 nodeTotalSize = node->dataSize;
	node->dataSize = size;
	node->used = true;
	allocator->m_binIndices[binIndex] = node->binListNext;
	if (node->binListNext != OA_NODE_UNUSED) allocator->m_nodes[node->binListNext].binListPrev = OA_NODE_UNUSED;
	allocator->m_freeStorage -= nodeTotalSize;
#ifdef DEBUG_VERBOSE
	printf("Free storage: %u (-%u) (allocate)\n", m_freeStorage, nodeTotalSize);
#endif

	// Bin empty?
	if (allocator->m_binIndices[binIndex] == OA_NODE_UNUSED)
	{
		// Remove a leaf bin mask bit
		allocator->m_usedBins[topBinIndex] &= ~(1 << leafBinIndex);
			
		// All leaf bins empty?
		if (allocator->m_usedBins[topBinIndex] == 0)
		{
			// Remove a top bin mask bit
			allocator->m_usedBinsTop &= ~(1 << topBinIndex);
		}
	}
		
	// Push back reminder N elements to a lower bin
	U32 reminderSize = nodeTotalSize - size;
	if (reminderSize > 0)
	{
		U32 newNodeIndex = oa_insert_node_into_bin(allocator, reminderSize, node->dataOffset + size);
		
		// Link nodes next to each other so that we can merge them later if both are free
		// And update the old next neighbor to point to the new node (in middle)
		if (node->neighborNext != OA_NODE_UNUSED) allocator->m_nodes[node->neighborNext].neighborPrev = newNodeIndex;
		allocator->m_nodes[newNodeIndex].neighborPrev = nodeIndex;
		allocator->m_nodes[newNodeIndex].neighborNext = node->neighborNext;
		node->neighborNext = newNodeIndex;
	}
	
	return (oaAllocation){.offset = node->dataOffset, .metadata = nodeIndex};
}

static void oa_remove_node_from_bin(oaAllocator* allocator, U32 nodeIndex) {
	oaNode* node = &allocator->m_nodes[nodeIndex];

	if (node->binListPrev != OA_NODE_UNUSED)
	{
		// Easy case: We have previous node. Just remove this node from the middle of the list.
		allocator->m_nodes[node->binListPrev].binListNext = node->binListNext;
		if (node->binListNext != OA_NODE_UNUSED) allocator->m_nodes[node->binListNext].binListPrev = node->binListPrev;
	}
	else
	{
		// Hard case: We are the first node in a bin. Find the bin.

		// Round down to bin index to ensure that bin >= alloc
		U32 binIndex = uintToFloatRoundDown(node->dataSize);

		U32 topBinIndex = binIndex >> OA_TOP_BINS_INDEX_SHIFT;
		U32 leafBinIndex = binIndex & OA_LEAF_BINS_INDEX_MASK;

		allocator->m_binIndices[binIndex] = node->binListNext;
		if (node->binListNext != OA_NODE_UNUSED) allocator->m_nodes[node->binListNext].binListPrev = OA_NODE_UNUSED;

		// Bin empty?
		if (allocator->m_binIndices[binIndex] == OA_NODE_UNUSED)
		{
			// Remove a leaf bin mask bit
			allocator->m_usedBins[topBinIndex] &= ~(1 << leafBinIndex);

			// All leaf bins empty?
			if (allocator->m_usedBins[topBinIndex] == 0)
			{
				// Remove a top bin mask bit
				allocator->m_usedBinsTop &= ~(1 << topBinIndex);
			}
		}
	}

	// Insert the node to freelist
#ifdef DEBUG_VERBOSE
	printf("Putting node %u into freelist[%u] (removeNodeFromBin)\n", nodeIndex, m_freeOffset + 1);
#endif
	allocator->m_freeNodes[++allocator->m_freeOffset] = nodeIndex;

	allocator->m_freeStorage -= node->dataSize;
#ifdef DEBUG_VERBOSE
	printf("Free storage: %u (-%u) (removeNodeFromBin)\n", m_freeStorage, node.dataSize);
#endif
}

static void oa_free(oaAllocator* allocator, oaAllocation allocation) {
	ASSERT(allocation.metadata != Allocation::NO_SPACE);
	if (!allocator->m_nodes) return;
		
	U32 nodeIndex = allocation.metadata;
	oaNode* node = &allocator->m_nodes[nodeIndex];
		
	// Double delete check
	ASSERT(node.used == true);
		
	// Merge with neighbors...
	U32 offset = node->dataOffset;
	U32 size = node->dataSize;
		
	if ((node->neighborPrev != OA_NODE_UNUSED) && (allocator->m_nodes[node->neighborPrev].used == false))
	{
		// Previous (contiguous) free node: Change offset to previous node offset. Sum sizes
		oaNode* prevNode = &allocator->m_nodes[node->neighborPrev];
		offset = prevNode->dataOffset;
		size += prevNode->dataSize;
			
		// Remove node from the bin linked list and put it in the freelist
		oa_remove_node_from_bin(allocator, node->neighborPrev);
			
		ASSERT(prevNode.neighborNext == nodeIndex);
		node->neighborPrev = prevNode->neighborPrev;
	}
		
	if ((node->neighborNext != OA_NODE_UNUSED) && (allocator->m_nodes[node->neighborNext].used == false))
	{
		// Next (contiguous) free node: Offset remains the same. Sum sizes.
		oaNode* nextNode = &allocator->m_nodes[node->neighborNext];
		size += nextNode->dataSize;
			
		// Remove node from the bin linked list and put it in the freelist
		oa_remove_node_from_bin(allocator, node->neighborNext);
			
		ASSERT(nextNode.neighborPrev == nodeIndex);
		node->neighborNext = nextNode->neighborNext;
	}

	U32 neighborNext = node->neighborNext;
	U32 neighborPrev = node->neighborPrev;
		
	// Insert the removed node to freelist
#ifdef DEBUG_VERBOSE
	printf("Putting node %u into freelist[%u] (free)\n", nodeIndex, m_freeOffset + 1);
#endif
	allocator->m_freeNodes[++allocator->m_freeOffset] = nodeIndex;

	// Insert the (combined) free node to bin
	U32 combinedNodeIndex = oa_insert_node_into_bin(allocator, size, offset);

	// Connect neighbors with the new combined node
	if (neighborNext != OA_NODE_UNUSED)
	{
		allocator->m_nodes[combinedNodeIndex].neighborNext = neighborNext;
		allocator->m_nodes[neighborNext].neighborPrev = combinedNodeIndex;
	}
	if (neighborPrev != OA_NODE_UNUSED)
	{
		allocator->m_nodes[combinedNodeIndex].neighborPrev = neighborPrev;
		allocator->m_nodes[neighborPrev].neighborNext = combinedNodeIndex;
	}
}

static U32 oa_allocation_size(oaAllocator* allocator, oaAllocation allocation) {
	if (allocation.metadata == OA_ALLOCATION_NO_SPACE) return 0;
	if (!allocator->m_nodes) return 0;
		
	return allocator->m_nodes[allocation.metadata].dataSize;
}

static oaStorageReport oa_storage_report(oaAllocator* allocator) {
	U32 largestFreeRegion = 0;
	U32 freeStorage = 0;
		
	// Out of allocations? -> Zero free space
	if (allocator->m_freeOffset > 0)
	{
		freeStorage = allocator->m_freeStorage;
		if (allocator->m_usedBinsTop)
		{
			U32 topBinIndex = 31 - lzcnt_nonzero(allocator->m_usedBinsTop);
			U32 leafBinIndex = 31 - lzcnt_nonzero(allocator->m_usedBins[topBinIndex]);
			largestFreeRegion = floatToUint((topBinIndex << OA_TOP_BINS_INDEX_SHIFT) | leafBinIndex);
			ASSERT(freeStorage >= largestFreeRegion);
		}
	}

	return (oaStorageReport){.totalFreeSpace = freeStorage, .largestFreeRegion = largestFreeRegion};
}

//StorageReportFull Allocator::storageReportFull() const
//{
//	StorageReportFull report;
//	for (U32 i = 0; i < NUM_LEAF_BINS; i++)
//	{
//		U32 count = 0;
//		U32 nodeIndex = m_binIndices[i];
//		while (nodeIndex != Node::unused)
//		{
//			nodeIndex = m_nodes[nodeIndex].binListNext;
//			count++;
//		}
//		report.freeRegions[i] = { .size = SmallFloat::floatToUint(i), .count = count };
//	}
//	return report;
//}