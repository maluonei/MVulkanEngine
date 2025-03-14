uint stepRNG(uint rngState) { return rngState * 747796405 + 1; }

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float stepAndOutputRNGFloat(inout uint rngState) {
  // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to
  // floating-point [0,1].
  rngState = stepRNG(rngState);
  uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
  word = (word >> 22) ^ word;
  return float(word) / 4294967295.0f;
}

float2 randomFloat2(inout uint rngState) {
  float x = stepAndOutputRNGFloat(rngState);
  float y = stepAndOutputRNGFloat(rngState);
  return float2(x, y);
}

float3 randomFloat3(inout uint rngState) {
  float x = stepAndOutputRNGFloat(rngState);
  float y = stepAndOutputRNGFloat(rngState);
  float z = stepAndOutputRNGFloat(rngState);
  return float3(x, y, z);
}