# MVulkanEngine

## TODO List  
1.culling  
2.gui
3.ssao
4.GI

Current Function:  
pbr:  
use PCF as soft shadow
![PBR](images/pbr.png)

ibl:
![IBL](images/ibl.png)

ssr:
version1.0, based on pbr, use 3-step ray-marching to find reflection ray's intersection point.
![SSR](images/ssr_v1.0.png)

rtao:
ray tracing ambient occlusion
![RTAO](images/rtao.png)

ddgi:
ddgi with rtao
![DDGI](images/ddgi_sponza.png)

mdf:
ray marching mesh distance field, generated using Jump Flood Algorithm:
![MDF](images/rayMarching_sdf.png)

sdfgi:
ddgi using sdf tracing
![SDFGI](images/sdfgi_sponza.png)

