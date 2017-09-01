StructuredBuffer<float> InputA : register(t0);
StructuredBuffer<float> InputB : register(t1);
RWStructuredBuffer<float> Output : register(u2);

struct _NumOfElements
{
	float4 numOfElements;
};

cbuffer NumOfElements : register(b3) {
	_NumOfElements elems;
};

[numthreads(1,1,1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint index = DTid.x;
	if (float(index) >= elems.numOfElements.x) return;
	Output[index] = InputA[index] + InputB[index];
}