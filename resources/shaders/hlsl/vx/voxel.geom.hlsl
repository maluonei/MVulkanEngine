struct VSOutput
{
    float3 normal : NORMAL;
    //float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;
    //float3x3 TBN : TEXCOORD1;
};

struct GSOutput
{
    float3 normal : NORMAL;
    //float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 voxPos : TEXCOORD1;
    uint instanceID : INSTANCE_ID;
    float4 position : SV_POSITION;
    //float3x3 TBN : TEXCOORD1;
};

struct UniformBuffer
{
    float4x4 Projection;
    int3 volumeResolution;
    int padding2;
};
    
[[vk::binding(1, 0)]]
cbuffer ubo1 : register(b1)
{
    UniformBuffer ubo1;
};

float4 GetProjection(float4 pos, int axis){
    if(axis == 0){
        //return float4(pos.z, pos.yxw);
        return float4(pos.yzxw);
    }
    else if(axis == 1){
        //return float4(pos.x, pos.z, pos.yw);
        return float4(pos.zxyw);
    }
    else{
        return float4(pos.xyzw);
    }
}

[maxvertexcount(3)]
void main(triangle VSOutput inputs[3], inout TriangleStream<GSOutput> outputs, uint id : SV_GSInstanceID)
{
    float3 p0 = inputs[0].position.xyz;
    float3 p1 = inputs[1].position.xyz;
    float3 p2 = inputs[2].position.xyz;

    float3 normal = abs(cross(p1 - p0, p2 - p0));
    int axis = 0;
    if(normal.y > normal.x){
        axis = 1;
    }
    if(normal.z > normal.y && normal.z > normal.x){
        axis = 2;
    }
    
    [loop]
    for (int i = 0; i < 3; ++i){
        GSOutput output;
        output.normal = inputs[i].normal;
        output.texCoord = inputs[i].texCoord;

        float4 pos = inputs[i].position;
        float4 temp = float4(mul(ubo1.Projection, pos));
        output.voxPos = (temp.xyz + 1.f) * 0.5f * ubo1.volumeResolution.xyz;
        
        output.instanceID = inputs[i].instanceID;
        output.position = mul(ubo1.Projection, float4(GetProjection(pos, axis)));
        output.position.z = 0.5f * (output.position.z + 1.f);
        outputs.Append(output);
    }
    outputs.RestartStrip();
}