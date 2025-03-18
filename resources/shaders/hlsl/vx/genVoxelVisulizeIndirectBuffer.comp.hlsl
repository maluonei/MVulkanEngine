struct DrawCommand {
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    int     vertexOffset;
    uint    firstInstance;
    float   paddings[3];
};

struct UniformBuffer0
{
    float3 aabbMin;
    float padding0;
    float3 aabbMax;
    float padding1;
    int3 voxelResolusion;
    float padding2;
};

struct ModelBuffer
{
    float4x4 Model;
};

[[vk::binding(0, 0)]]
cbuffer ubo0 : register(b0)
{
    UniformBuffer0 ubo0;
};


[[vk::binding(1, 0)]] RWStructuredBuffer<DrawCommand> outputCommands : register(u0);
[[vk::binding(2, 0)]] RWStructuredBuffer<ModelBuffer> Models : register(u1);
[[vk::binding(3, 0)]] Texture3D<float4> voxelTexture : register(t0);

[numthreads(8, 8, 8)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    //for(int i=0;i<4;i++){
    //    for(int j=0;j<4;j++){
    //        for(int k=0;k<4;k++){
    //            uint3 pixIndex = uint3(4, 4, 4) * DispatchThreadID + uint3(i, j, k);
//
    //            if(pixIndex.x >= ubo0.voxelResolusion.x || pixIndex.y >= ubo0.voxelResolusion.y || pixIndex.z >= ubo0.voxelResolusion.z)
    //            {
    //                continue;
    //            }
//
    //            int index = pixIndex.x * ubo0.voxelResolusion.y * ubo0.voxelResolusion.z + 
    //                        pixIndex.y * ubo0.voxelResolusion.z + 
    //                        pixIndex.z;
//
    //            if(voxelTexture[int3(pixIndex)].r > 0.f){
    //                float3 gridOffset = (ubo0.aabbMax - ubo0.aabbMin) / float3(ubo0.voxelResolusion - float3(1.f, 1.f, 1.f));
    //                float3 voxelPosition = ubo0.aabbMin + gridOffset * float3(pixIndex);
//
    //                DrawCommand command;
    //                command.indexCount = 24;
    //                command.instanceCount = 1;
    //                command.firstIndex = 0;
    //                command.vertexOffset = 0;
    //                command.firstInstance = index;
//
    //                outputCommands[index] = command;
//
    //                float4x4 model = float4x4( 
    //                    gridOffset.x, 0.f, 0.f, voxelPosition.x,
    //                    0.f, gridOffset.y, 0.f, voxelPosition.y,
    //                    0.f, 0.f, gridOffset.z, voxelPosition.z,
    //                    0.f, 0.f, 0.f, 1.f
    //                );
//
    //                ModelBuffer modelBuffer;
    //                modelBuffer.Model = model;
    //                Models[index] = modelBuffer;
    //            }
    //            else{
    //                DrawCommand command;
    //                command.indexCount = 0;
    //                command.instanceCount = 1;
    //                command.firstIndex = 0;
    //                command.vertexOffset = 0;
    //                command.firstInstance = 0;
//
    //                outputCommands[index] = command;
    //            }
    //        }
    //    }
    //}

    if(DispatchThreadID.x >= ubo0.voxelResolusion.x || DispatchThreadID.y >= ubo0.voxelResolusion.y || DispatchThreadID.z >= ubo0.voxelResolusion.z)
    {
        return;
    }
    
    int index = DispatchThreadID.x * ubo0.voxelResolusion.y * ubo0.voxelResolusion.z + 
                DispatchThreadID.y * ubo0.voxelResolusion.z + 
                DispatchThreadID.z;
    
    if(voxelTexture[int3(DispatchThreadID)].r > 0.f){
        float3 gridOffset = (ubo0.aabbMax - ubo0.aabbMin) / float3(ubo0.voxelResolusion - float3(1.f, 1.f, 1.f));
        float3 voxelPosition = ubo0.aabbMin + gridOffset * float3(DispatchThreadID);
    
        DrawCommand command;
        command.indexCount = 24;
        command.instanceCount = 1;
        command.firstIndex = 0;
        command.vertexOffset = 0;
        command.firstInstance = index;
    
        outputCommands[index] = command;
    
        float4x4 model = float4x4( 
            gridOffset.x, 0.f, 0.f, voxelPosition.x,
            0.f, gridOffset.y, 0.f, voxelPosition.y,
            0.f, 0.f, gridOffset.z, voxelPosition.z,
            0.f, 0.f, 0.f, 1.f
        );
    
        ModelBuffer modelBuffer;
        modelBuffer.Model = model;
        Models[index] = modelBuffer;
    }
    else{
        DrawCommand command;
        command.indexCount = 0;
        command.instanceCount = 1;
        command.firstIndex = 0;
        command.vertexOffset = 0;
        command.firstInstance = 0;
    
        outputCommands[index] = command;
    }      
}
