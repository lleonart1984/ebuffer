// Data for each fragment
struct Fragment {
	int Index;
	float MinDepth;
	float MaxDepth;
};

struct OccupiedRangeInfo
{
	int Start;
	int Count;
};

struct Range
{
	float MinDepth;
	float MaxDepth;
};

RWStructuredBuffer<OccupiedRangeInfo>	occupied	: register(u0);
RWStructuredBuffer<Range>				ranges		: register(u1);
RWTexture2D<int>					eLengthBuffer	: register(u2);

StructuredBuffer<Fragment>			fragments		: register(t0);
StructuredBuffer<int>				indices			: register(t1);
Texture2D<int>						startBuffer		: register(t2);
Texture2D<int>						countBuffer	    : register(t3);

static float epsilon = 0;

void main(float4 proj : SV_POSITION)
{
	uint2 coord = uint2(proj.xy);

	int startIndex = startBuffer[coord];
	int count = countBuffer[coord];

	if (count == 0)
	{
		eLengthBuffer[coord] = 1; // no occupied blocks means a single empty space block
		ranges[startIndex].MinDepth = 0;
		ranges[startIndex].MaxDepth = 100000;
		return;
	}

	int occupiedCount = 0;

	float lastEmptyBlockMinim = 0;
	float lastMinim = fragments[indices[startIndex]].MinDepth - epsilon;
	float lastMaxim = fragments[indices[startIndex]].MaxDepth + epsilon;
	int lastStart = startIndex;
	int lastCount = 1;
	int index = 0;

	for (int i = startIndex + 1; i<startIndex + count; i++)
	{
		int fragIndex = indices[i];
		float minDepth = fragments[fragIndex].MinDepth - epsilon;
		float maxDepth = fragments[fragIndex].MaxDepth + epsilon;

		if (lastMaxim <= minDepth)
		{
			index = startIndex + occupiedCount;
			occupied[index].Start = lastStart;
			occupied[index].Count = lastCount;

			ranges[index].MinDepth = lastEmptyBlockMinim;
			ranges[index].MaxDepth = lastMinim;
			lastEmptyBlockMinim = lastMaxim;
			occupiedCount++;

			lastMinim = minDepth;
			lastMaxim = maxDepth;
			lastStart = i;
			lastCount = 1;
		}
		else
		{
			lastMinim = min(lastMinim, minDepth);
			lastMaxim = max(lastMaxim, maxDepth);
			lastCount++;
		}
	}

	index = startIndex + occupiedCount;
	occupied[index].Start = lastStart;
	occupied[index].Count = lastCount;

	ranges[index].MinDepth = lastEmptyBlockMinim;
	ranges[index].MaxDepth = lastMinim;
	lastEmptyBlockMinim = lastMaxim;
	occupiedCount++;

	index = startIndex + occupiedCount;
	ranges[index].MinDepth = lastEmptyBlockMinim;
	ranges[index].MaxDepth = 100000;

	eLengthBuffer[coord] = occupiedCount + 1; // empty space blocks are one value greater than occupied blocks
}
