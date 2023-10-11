// Light pixel shader
// Calculate diffuse lighting for a single directional light(also texturing)

Texture2D shaderTexture : register(t0);
Texture2D shaderTexture2 : register(t1);
Texture2D shaderTexture3 : register(t2);
SamplerState SampleType : register(s0);


cbuffer LightBuffer : register(b0)
{
    float4 ambientColor;
    float4 diffuseColor;
    float3 lightPosition;
    float padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 position3D : TEXCOORD2;
    float height : FLOAT;
};

float4 main(InputType input) : SV_TARGET
{
    float4 textureColor;
    float3 lightDir;
    float lightIntensity;
    float4 color;
    float4 heightColor;
	
    // Bounds between 'ground' textures and 'snow' texture
    static const float TEX_LOW_BOUND = 3.f;
    static const float TEX_HIGH_BOUND = 4.5f;
    
    // Bounds for blending the two ground textures 
    static const float SLOPE_LOW_BOUND = .3f;
    static const float SLOPE_HIGH_BOUND = .7f;
    
    // Sample all three textures for blending
    float4 lowCol = shaderTexture.Sample(SampleType, input.tex);
    float4 midCol = shaderTexture2.Sample(SampleType, input.tex);
    float4 highCol = shaderTexture3.Sample(SampleType, input.tex);
	
    // Calculate the slope 
    float slope = 1.f - input.normal.y;

	// Invert the light direction for calculations.
    lightDir = normalize(input.position3D - lightPosition);

	// Calculate the amount of light on this pixel.
    lightIntensity = saturate(dot(input.normal, -lightDir));

	// Determine the final amount of diffuse color based on the diffuse color combined with the light intensity.
    color = ambientColor + (diffuseColor * lightIntensity); //adding ambient
    color = saturate(color);

    if (slope < SLOPE_LOW_BOUND)
    {
        // blend the mid colour and the low colour when below the lowest slope boundary
        textureColor = lerp(midCol, lowCol, slope / SLOPE_LOW_BOUND);
    }
    else if (slope >= SLOPE_LOW_BOUND && slope < SLOPE_HIGH_BOUND)
    {
        // blend the low colour with the mid colour when in between bounds
        textureColor = lerp(lowCol, midCol, (slope - SLOPE_LOW_BOUND) * (1.f / (SLOPE_HIGH_BOUND - SLOPE_LOW_BOUND)));
    }
    else if (slope >= SLOPE_HIGH_BOUND)
    {
        // set the texture colour to the mid colour when above the slope bounds
        textureColor = midCol;
    }

    if (input.height >= TEX_LOW_BOUND && input.height < TEX_HIGH_BOUND)
    {
        // Blend the current texture colour (as set above) with the new texture when in between bounds
        textureColor = lerp(textureColor, highCol, (input.height - TEX_LOW_BOUND) * (1.f / (TEX_HIGH_BOUND - TEX_LOW_BOUND)));
    }
    else if (input.height >= TEX_HIGH_BOUND)
    {
        // Set the texture colour to the now texture colour when above the highest boundary 
        textureColor = highCol;
    }
    
	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
    color = color * textureColor;

    return color;
}

