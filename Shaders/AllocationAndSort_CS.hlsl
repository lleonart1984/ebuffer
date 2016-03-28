// Data for each fragment
struct Fragment {
	int Index;
	float MinDepth;
	float MaxDepth;
};

///Input
// length of each fragment list.
Texture2D<int> countBuffer : register(t0);

Texture2D<int> firstBuffer : register(t1);

StructuredBuffer<int> nextBuffer : register(t2);

StructuredBuffer<Fragment> fragments : register(t3);

/// Output
// arrays start index after allocation in indices buffer
RWTexture2D<int> startBuffer : register(u0);
// counting buffer used to allocate
RWStructuredBuffer<int> mallocBuffer : register(u1);
RWStructuredBuffer<int> indices : register(u2);
RWStructuredBuffer<int> temp : register(u3);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint2 coord = uint2(DTid.x, DTid.y);

	/// Indices allocation in array
	int count = countBuffer[coord];
	int startIndex;
	InterlockedAdd(mallocBuffer[0], count + 1, startIndex);
	//determine start position of current list at fragments buffer.
	startBuffer[coord] = startIndex;
	int currentIndex = firstBuffer[coord];
	for (int i = 0; i<count; i++)
	{
		indices[startIndex + i] = currentIndex;
		currentIndex = nextBuffer[currentIndex];
	}

	/// Sorting indices with Bottom-Up Merge-Sort
	int Size = 1;

	while (Size < count)
	{
		int SizeBy2 = Size << 1;

		//for (int i=0; i<count; i+= SizeBy2)
		for (int m = 0; m <= count / SizeBy2; m++)
		{
			int i = m*SizeBy2;

			int currentStartIndex = startIndex + i;

			int2 start = currentStartIndex + int2(0, Size);
			int2 end = startIndex - 1 + min(count, i + int2(Size, SizeBy2));

			int total = currentStartIndex + end.y - start.x + 1;

			for (int j = currentStartIndex; j < total; j++)
			{
				bool takeFirst = start.y > end.y || (start.x <= end.x && fragments[indices[start.x]].MinDepth <= fragments[indices[start.y]].MinDepth);

				temp[j] = indices[takeFirst ? start.x : start.y];

				start += takeFirst ? int2(1, 0) : int2(0, 1);
			}
		}

		int last = startIndex + count;

		for (int l = startIndex; l<last; l++)
			indices[l] = temp[l];

		Size = SizeBy2;
	}
}