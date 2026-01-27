#include "skinned_mesh.hlsli"

[maxvertexcount(3)]
void main(
	triangle VS_OUT input[3],
	inout TriangleStream<GS_OUT> output
)
{
	float3 A = input[1].world_position.xyz - input[0].world_position.xyz;
	float3 B = input[2].world_position.xyz - input[0].world_position.xyz;
	float3 face_normal = normalize(cross(B, A)); // flat shading
	
	GS_OUT element;
	
	element.position = input[0].position;
	element.world_position = input[0].world_position;
	element.texcoord = input[0].texcoord;
	element.color = input[0].color;
	element.face_normal = face_normal;
	element.color = input[0].color;
	output.Append(element);

	element.position = input[1].position;
	element.world_position = input[1].world_position;
	element.texcoord = input[1].texcoord;
	element.color = input[1].color;
	element.face_normal = face_normal;
	element.color = input[1].color;
	output.Append(element);

	element.position = input[2].position;
	element.world_position = input[2].world_position;
	element.texcoord = input[2].texcoord;
	element.color = input[2].color;
	element.face_normal = face_normal;
	element.color = input[2].color;
	output.Append(element);

}