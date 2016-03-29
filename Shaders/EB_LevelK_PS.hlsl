// Specifies a range of float values
/*
* Structures
*/
struct Range {
	float MinDepth;
	float MaxDepth;
};

cbuffer CombiningInfo : register(b0)
{
	int CurrentLevel;
}

/*
* Input
*/
Texture2D<int> prevStartBuffer					: register(t0);
Texture2D<int> eLengthBuffer				: register(t1);

/*
* Output
*/
RWStructuredBuffer<Range> ranges			: register(u0);
RWTexture2D<int> startEB					: register(u1);
RWStructuredBuffer<int> mallocBuffer		: register(u2);

struct PS_IN
{
	float4 proj : SV_POSITION;
};

float2 Intersection(float2 a, float2 b)
{
	return float2(max(a.x, b.x), min(a.y, b.y));
}

void UpdateRanges(int startIndex, int count, uint2 coord)
{
	int fromIndex = prevStartBuffer[coord];
	int fromCount = eLengthBuffer[coord << (CurrentLevel - 1)];
	int fromEnd = fromIndex + fromCount - 1;

	int toIndex = startIndex;
	int toEnd = toIndex + count - 1;

	float2 toRange = (float2)ranges[toIndex];
	float2 fromRange = (float2)ranges[fromIndex];
	float2 currentBestRange = float2(100000, 0);

	for (int i = 0; i<fromCount + count - 1; i++)
	{
		float2 intersection = Intersection(toRange, fromRange);
		if (intersection.y - intersection.x > currentBestRange.y - currentBestRange.x)
			currentBestRange = intersection;

		if (fromRange.y < toRange.y) // from-range should advance
		{
			fromRange = (float2)ranges[++fromIndex];
		}
		else // to-range should advance
		{
			ranges[toIndex++] = (Range)currentBestRange;
			toRange = (float2)ranges[toIndex];
			currentBestRange = float2(100000, 0);
		}
	}
}

void main(float4 proj : SV_POSITION)
{
	uint2 coord = uint2(proj.xy);

	int startIndex = prevStartBuffer[coord << 1];

	int count = eLengthBuffer[coord << CurrentLevel];

	int startEBIndex;

	InterlockedAdd(mallocBuffer[0], count, startEBIndex);

	startEB[coord] = startEBIndex;

	for (int i = 0; i < count; i++)
		ranges[i + startEBIndex] = ranges[i + startIndex];

	UpdateRanges(startEBIndex, count, uint2(coord.x << 1 | 1, coord.y << 1));
	UpdateRanges(startEBIndex, count, uint2(coord.x << 1 | 1, coord.y << 1 | 1));
	UpdateRanges(startEBIndex, count, uint2(coord.x << 1, coord.y << 1 | 1));
}
