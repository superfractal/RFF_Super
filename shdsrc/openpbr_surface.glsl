//
// Created by Opus 5.5 on 2026-11-04
// Modified by Opus 5.5 on 2026-11-06
//

// OpenPBR Surface (ASWF specification 1.1, Apache-2.0) layered over native relief; independent GPL-3.0-only code from its published equations, see NOTICE.

// GGX split-sum albedo, correlated Smith, integrated here (debug/openpbr_fit): x weights F0, y the Schlick edge, z the F82 dip; rows sqrt(alpha) 0.1414..1, columns view cosine 0..1.
const vec3 OPENPBR_GGX_ALBEDO[256] = vec3[](
    vec3(0.2354, 0.7380, 0.0275), vec3(0.3227, 0.6272, 0.0435), vec3(0.5202, 0.4650, 0.0545), vec3(0.6745, 0.3188, 0.0509), vec3(0.7874, 0.2088, 0.0406), vec3(0.8667, 0.1308, 0.0289), vec3(0.9204, 0.0779, 0.0186), vec3(0.9552, 0.0436, 0.0108), vec3(0.9765, 0.0226, 0.0056), vec3(0.9886, 0.0106, 0.0025), vec3(0.9951, 0.0043, 0.0010), vec3(0.9980, 0.0015, 0.0003), vec3(0.9992, 0.0004, 0.0001), vec3(0.9996, 0.0001, 0.0000), vec3(1.0000, 0.0000, 0.0000), vec3(1.0000, 0.0000, 0.0000),
    vec3(0.3661, 0.6120, 0.0345), vec3(0.3801, 0.5179, 0.0423), vec3(0.5359, 0.4147, 0.0504), vec3(0.6773, 0.2977, 0.0476), vec3(0.7852, 0.2003, 0.0386), vec3(0.8628, 0.1278, 0.0279), vec3(0.9161, 0.0773, 0.0182), vec3(0.9511, 0.0440, 0.0108), vec3(0.9730, 0.0232, 0.0057), vec3(0.9858, 0.0111, 0.0027), vec3(0.9928, 0.0047, 0.0011), vec3(0.9962, 0.0017, 0.0003), vec3(0.9977, 0.0005, 0.0001), vec3(0.9983, 0.0001, 0.0000), vec3(0.9983, 0.0000, 0.0000), vec3(0.9980, 0.0000, 0.0000),
    vec3(0.4828, 0.4972, 0.0371), vec3(0.4660, 0.4178, 0.0406), vec3(0.5652, 0.3442, 0.0446), vec3(0.6816, 0.2604, 0.0423), vec3(0.7797, 0.1827, 0.0351), vec3(0.8538, 0.1205, 0.0260), vec3(0.9065, 0.0750, 0.0175), vec3(0.9422, 0.0438, 0.0107), vec3(0.9652, 0.0238, 0.0059), vec3(0.9791, 0.0119, 0.0029), vec3(0.9870, 0.0053, 0.0012), vec3(0.9910, 0.0020, 0.0004), vec3(0.9928, 0.0006, 0.0001), vec3(0.9941, 0.0002, 0.0000), vec3(0.9949, 0.0000, 0.0000), vec3(0.9956, 0.0000, 0.0000),
    vec3(0.5817, 0.3992, 0.0364), vec3(0.5578, 0.3345, 0.0377), vec3(0.6098, 0.2752, 0.0384), vec3(0.6920, 0.2149, 0.0360), vec3(0.7725, 0.1572, 0.0303), vec3(0.8392, 0.1080, 0.0231), vec3(0.8895, 0.0698, 0.0161), vec3(0.9253, 0.0424, 0.0102), vec3(0.9491, 0.0240, 0.0059), vec3(0.9647, 0.0125, 0.0030), vec3(0.9749, 0.0059, 0.0014), vec3(0.9810, 0.0024, 0.0005), vec3(0.9844, 0.0009, 0.0002), vec3(0.9864, 0.0002, 0.0000), vec3(0.9878, 0.0001, 0.0000), vec3(0.9889, 0.0000, 0.0000),
    vec3(0.6628, 0.3187, 0.0337), vec3(0.6394, 0.2657, 0.0336), vec3(0.6606, 0.2169, 0.0325), vec3(0.7099, 0.1713, 0.0297), vec3(0.7673, 0.1289, 0.0252), vec3(0.8206, 0.0919, 0.0196), vec3(0.8644, 0.0618, 0.0141), vec3(0.8991, 0.0391, 0.0093), vec3(0.9251, 0.0232, 0.0056), vec3(0.9431, 0.0127, 0.0031), vec3(0.9550, 0.0064, 0.0015), vec3(0.9632, 0.0029, 0.0007), vec3(0.9685, 0.0011, 0.0002), vec3(0.9722, 0.0004, 0.0001), vec3(0.9748, 0.0001, 0.0000), vec3(0.9768, 0.0000, 0.0000),
    vec3(0.7276, 0.2542, 0.0301), vec3(0.7046, 0.2099, 0.0290), vec3(0.7070, 0.1701, 0.0271), vec3(0.7303, 0.1345, 0.0241), vec3(0.7643, 0.1027, 0.0204), vec3(0.8014, 0.0750, 0.0161), vec3(0.8380, 0.0522, 0.0119), vec3(0.8685, 0.0344, 0.0081), vec3(0.8926, 0.0213, 0.0051), vec3(0.9116, 0.0123, 0.0030), vec3(0.9255, 0.0066, 0.0016), vec3(0.9358, 0.0032, 0.0007), vec3(0.9434, 0.0013, 0.0003), vec3(0.9490, 0.0005, 0.0001), vec3(0.9534, 0.0001, 0.0000), vec3(0.9568, 0.0000, 0.0000),
    vec3(0.7785, 0.2034, 0.0262), vec3(0.7531, 0.1658, 0.0246), vec3(0.7426, 0.1331, 0.0223), vec3(0.7468, 0.1049, 0.0194), vec3(0.7628, 0.0806, 0.0162), vec3(0.7868, 0.0598, 0.0129), vec3(0.8111, 0.0426, 0.0097), vec3(0.8343, 0.0290, 0.0068), vec3(0.8548, 0.0187, 0.0045), vec3(0.8722, 0.0113, 0.0027), vec3(0.8865, 0.0064, 0.0015), vec3(0.8979, 0.0033, 0.0008), vec3(0.9072, 0.0015, 0.0003), vec3(0.9147, 0.0006, 0.0001), vec3(0.9209, 0.0002, 0.0000), vec3(0.9260, 0.0000, 0.0000),
    vec3(0.8181, 0.1638, 0.0226), vec3(0.7870, 0.1314, 0.0205), vec3(0.7658, 0.1045, 0.0182), vec3(0.7569, 0.0819, 0.0156), vec3(0.7617, 0.0629, 0.0129), vec3(0.7713, 0.0470, 0.0102), vec3(0.7847, 0.0340, 0.0077), vec3(0.7992, 0.0237, 0.0056), vec3(0.8137, 0.0157, 0.0038), vec3(0.8273, 0.0099, 0.0024), vec3(0.8396, 0.0058, 0.0014), vec3(0.8505, 0.0032, 0.0007), vec3(0.8601, 0.0015, 0.0004), vec3(0.8685, 0.0006, 0.0001), vec3(0.8758, 0.0002, 0.0000), vec3(0.8824, 0.0000, 0.0000),
    vec3(0.8486, 0.1333, 0.0193), vec3(0.8088, 0.1048, 0.0171), vec3(0.7778, 0.0824, 0.0148), vec3(0.7618, 0.0641, 0.0124), vec3(0.7550, 0.0490, 0.0102), vec3(0.7542, 0.0368, 0.0080), vec3(0.7575, 0.0268, 0.0061), vec3(0.7634, 0.0189, 0.0044), vec3(0.7708, 0.0129, 0.0031), vec3(0.7790, 0.0083, 0.0020), vec3(0.7874, 0.0051, 0.0012), vec3(0.7957, 0.0029, 0.0007), vec3(0.8036, 0.0015, 0.0003), vec3(0.8112, 0.0007, 0.0001), vec3(0.8184, 0.0002, 0.0000), vec3(0.8252, 0.0000, 0.0000),
    vec3(0.8720, 0.1097, 0.0166), vec3(0.8211, 0.0844, 0.0142), vec3(0.7808, 0.0654, 0.0120), vec3(0.7583, 0.0504, 0.0100), vec3(0.7428, 0.0384, 0.0081), vec3(0.7333, 0.0287, 0.0063), vec3(0.7280, 0.0210, 0.0048), vec3(0.7260, 0.0150, 0.0035), vec3(0.7263, 0.0103, 0.0025), vec3(0.7283, 0.0068, 0.0016), vec3(0.7316, 0.0043, 0.0010), vec3(0.7356, 0.0025, 0.0006), vec3(0.7403, 0.0013, 0.0003), vec3(0.7454, 0.0006, 0.0001), vec3(0.7507, 0.0002, 0.0000), vec3(0.7561, 0.0000, 0.0000),
    vec3(0.8899, 0.0916, 0.0143), vec3(0.8257, 0.0686, 0.0119), vec3(0.7780, 0.0524, 0.0098), vec3(0.7472, 0.0399, 0.0080), vec3(0.7246, 0.0302, 0.0064), vec3(0.7078, 0.0225, 0.0050), vec3(0.6954, 0.0165, 0.0038), vec3(0.6864, 0.0118, 0.0028), vec3(0.6801, 0.0082, 0.0020), vec3(0.6760, 0.0055, 0.0013), vec3(0.6736, 0.0035, 0.0008), vec3(0.6726, 0.0021, 0.0005), vec3(0.6728, 0.0012, 0.0003), vec3(0.6740, 0.0006, 0.0001), vec3(0.6759, 0.0002, 0.0000), vec3(0.6784, 0.0000, 0.0000),
    vec3(0.9037, 0.0776, 0.0124), vec3(0.8245, 0.0564, 0.0100), vec3(0.7692, 0.0423, 0.0081), vec3(0.7307, 0.0319, 0.0065), vec3(0.7012, 0.0239, 0.0051), vec3(0.6780, 0.0177, 0.0039), vec3(0.6593, 0.0129, 0.0030), vec3(0.6443, 0.0093, 0.0022), vec3(0.6321, 0.0065, 0.0015), vec3(0.6223, 0.0044, 0.0011), vec3(0.6146, 0.0028, 0.0007), vec3(0.6084, 0.0017, 0.0004), vec3(0.6038, 0.0010, 0.0002), vec3(0.6004, 0.0005, 0.0001), vec3(0.5980, 0.0002, 0.0000), vec3(0.5966, 0.0000, 0.0000),
    vec3(0.9142, 0.0668, 0.0108), vec3(0.8187, 0.0470, 0.0084), vec3(0.7552, 0.0346, 0.0067), vec3(0.7093, 0.0257, 0.0053), vec3(0.6735, 0.0190, 0.0041), vec3(0.6444, 0.0140, 0.0031), vec3(0.6203, 0.0102, 0.0023), vec3(0.5999, 0.0073, 0.0017), vec3(0.5827, 0.0051, 0.0012), vec3(0.5680, 0.0035, 0.0008), vec3(0.5554, 0.0023, 0.0005), vec3(0.5446, 0.0014, 0.0003), vec3(0.5354, 0.0008, 0.0002), vec3(0.5275, 0.0004, 0.0001), vec3(0.5209, 0.0002, 0.0000), vec3(0.5153, 0.0000, 0.0000),
    vec3(0.9222, 0.0584, 0.0096), vec3(0.8095, 0.0395, 0.0072), vec3(0.7373, 0.0285, 0.0056), vec3(0.6844, 0.0208, 0.0043), vec3(0.6424, 0.0152, 0.0033), vec3(0.6080, 0.0111, 0.0025), vec3(0.5790, 0.0080, 0.0019), vec3(0.5541, 0.0057, 0.0013), vec3(0.5326, 0.0040, 0.0010), vec3(0.5138, 0.0027, 0.0007), vec3(0.4972, 0.0018, 0.0004), vec3(0.4826, 0.0011, 0.0003), vec3(0.4696, 0.0007, 0.0002), vec3(0.4580, 0.0004, 0.0001), vec3(0.4476, 0.0002, 0.0000), vec3(0.4383, 0.0000, 0.0000),
    vec3(0.9284, 0.0519, 0.0086), vec3(0.7980, 0.0337, 0.0062), vec3(0.7171, 0.0237, 0.0047), vec3(0.6569, 0.0170, 0.0036), vec3(0.6091, 0.0123, 0.0027), vec3(0.5698, 0.0089, 0.0020), vec3(0.5365, 0.0064, 0.0015), vec3(0.5078, 0.0045, 0.0011), vec3(0.4828, 0.0032, 0.0008), vec3(0.4607, 0.0021, 0.0005), vec3(0.4411, 0.0014, 0.0003), vec3(0.4235, 0.0009, 0.0002), vec3(0.4077, 0.0005, 0.0001), vec3(0.3933, 0.0003, 0.0001), vec3(0.3803, 0.0001, 0.0000), vec3(0.3684, 0.0000, 0.0000),
    vec3(0.9332, 0.0467, 0.0078), vec3(0.7857, 0.0290, 0.0054), vec3(0.6948, 0.0199, 0.0040), vec3(0.6276, 0.0140, 0.0029), vec3(0.5745, 0.0100, 0.0022), vec3(0.5308, 0.0071, 0.0016), vec3(0.4938, 0.0051, 0.0012), vec3(0.4620, 0.0036, 0.0008), vec3(0.4343, 0.0025, 0.0006), vec3(0.4098, 0.0017, 0.0004), vec3(0.3880, 0.0011, 0.0003), vec3(0.3685, 0.0007, 0.0002), vec3(0.3508, 0.0004, 0.0001), vec3(0.3348, 0.0002, 0.0001), vec3(0.3202, 0.0001, 0.0000), vec3(0.3068, 0.0000, 0.0000)
);

// Charlie fuzz albedo with the Neubelt-Pettineo visibility, capped at 1; rows run fuzz roughness 0.07 to 1, columns the view cosine 0 to 1.
const float OPENPBR_FUZZ_ALBEDO[256] = float[](
    1.0000, 1.0000, 0.3803, 0.0978, 0.0215, 0.0039, 0.0006, 0.0001, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    1.0000, 1.0000, 0.6851, 0.3539, 0.1778, 0.0853, 0.0386, 0.0162, 0.0062, 0.0021, 0.0006, 0.0001, 0.0000, 0.0000, 0.0000, 0.0000,
    1.0000, 1.0000, 0.7335, 0.4717, 0.3015, 0.1894, 0.1160, 0.0686, 0.0388, 0.0207, 0.0103, 0.0046, 0.0018, 0.0005, 0.0001, 0.0000,
    1.0000, 1.0000, 0.7133, 0.5110, 0.3666, 0.2611, 0.1833, 0.1262, 0.0846, 0.0548, 0.0339, 0.0197, 0.0105, 0.0049, 0.0017, 0.0003,
    1.0000, 0.9044, 0.6775, 0.5178, 0.3975, 0.3043, 0.2310, 0.1732, 0.1276, 0.0918, 0.0641, 0.0429, 0.0270, 0.0155, 0.0075, 0.0023,
    1.0000, 0.8201, 0.6413, 0.5116, 0.4106, 0.3294, 0.2630, 0.2081, 0.1627, 0.1251, 0.0940, 0.0685, 0.0478, 0.0312, 0.0182, 0.0083,
    1.0000, 0.7551, 0.6087, 0.5005, 0.4145, 0.3436, 0.2840, 0.2334, 0.1901, 0.1528, 0.1208, 0.0932, 0.0697, 0.0496, 0.0327, 0.0186,
    0.9230, 0.7042, 0.5807, 0.4883, 0.4138, 0.3514, 0.2979, 0.2516, 0.2110, 0.1753, 0.1436, 0.1156, 0.0907, 0.0687, 0.0493, 0.0322,
    0.8522, 0.6638, 0.5569, 0.4764, 0.4108, 0.3553, 0.3072, 0.2648, 0.2271, 0.1933, 0.1628, 0.1352, 0.1101, 0.0873, 0.0665, 0.0476,
    0.7974, 0.6313, 0.5369, 0.4655, 0.4069, 0.3570, 0.3133, 0.2745, 0.2396, 0.2078, 0.1788, 0.1521, 0.1274, 0.1045, 0.0833, 0.0635,
    0.7542, 0.6048, 0.5200, 0.4557, 0.4028, 0.3574, 0.3175, 0.2817, 0.2493, 0.2195, 0.1921, 0.1665, 0.1427, 0.1203, 0.0991, 0.0792,
    0.7195, 0.5831, 0.5058, 0.4471, 0.3987, 0.3571, 0.3203, 0.2872, 0.2570, 0.2291, 0.2032, 0.1789, 0.1560, 0.1343, 0.1137, 0.0939,
    0.6912, 0.5650, 0.4937, 0.4395, 0.3949, 0.3563, 0.3222, 0.2914, 0.2632, 0.2370, 0.2126, 0.1895, 0.1677, 0.1468, 0.1268, 0.1076,
    0.6679, 0.5498, 0.4834, 0.4329, 0.3913, 0.3554, 0.3235, 0.2947, 0.2682, 0.2436, 0.2205, 0.1986, 0.1778, 0.1579, 0.1387, 0.1200,
    0.6484, 0.5371, 0.4746, 0.4272, 0.3881, 0.3543, 0.3244, 0.2973, 0.2723, 0.2491, 0.2272, 0.2065, 0.1867, 0.1676, 0.1492, 0.1313,
    0.6321, 0.5262, 0.4669, 0.4221, 0.3852, 0.3533, 0.3250, 0.2993, 0.2757, 0.2537, 0.2330, 0.2133, 0.1944, 0.1762, 0.1586, 0.1414
);

// Bilinear position in a 16 x 16 table: the first coordinate walks rows, the second columns, both 0 to 1.
void openpbr_table_cell(float row, float column, out int index, out vec2 weight) {
    vec2 p = clamp(vec2(row, column), 0.0, 1.0) * 15.0;
    ivec2 cell = min(ivec2(p), ivec2(14));
    weight = p - vec2(cell);
    index = cell.x * 16 + cell.y;
}

vec3 openpbr_ggx_albedo(float cosine, float alpha) {
    int i;
    vec2 w;
    openpbr_table_cell((sqrt(alpha) - 0.14142136) / 0.85857864, cosine, i, w);
    return mix(mix(OPENPBR_GGX_ALBEDO[i], OPENPBR_GGX_ALBEDO[i + 1], w.y),
               mix(OPENPBR_GGX_ALBEDO[i + 16], OPENPBR_GGX_ALBEDO[i + 17], w.y), w.x);
}

float openpbr_fuzz_albedo(float cosine, float roughness) {
    int i;
    vec2 w;
    openpbr_table_cell((roughness - 0.07) / 0.93, cosine, i, w);
    return mix(mix(OPENPBR_FUZZ_ALBEDO[i], OPENPBR_FUZZ_ALBEDO[i + 1], w.y),
               mix(OPENPBR_FUZZ_ALBEDO[i + 16], OPENPBR_FUZZ_ALBEDO[i + 17], w.y), w.x);
}

// Multiple-scattering compensation (Turquin 2019): scales a single-scattering lobe by the energy its albedo e misses.
vec3 openpbr_energy_compensation(vec3 f0, float e) {
    return 1.0 + clamp(f0, 0.0, 1.0) * (1.0 - e) / max(e, 1e-3);
}

// Unpolarised dielectric Fresnel for a relative index eta, 1 under total internal reflection (Walter et al. 2007).
float openpbr_fresnel_dielectric(float cosine, float eta) {
    float c = clamp(cosine, 0.0, 1.0);
    float g2 = eta * eta - 1.0 + c * c;
    if (g2 < 0.0) {
        return 1.0;
    }
    float g = sqrt(g2);
    float a = (g - c) / max(g + c, 1e-6);
    float b = (c * (g + c) - 1.0) / (c * (g - c) + 1.0);
    return 0.5 * a * a * (1.0 + b * b);
}

// F82-tint metal Fresnel (Kutz et al. 2021, as OpenPBR adopts it); 0.46266437 is (6/7)^5 and 0.05665278 is (1/7)(6/7)^6.
vec3 openpbr_f82_dip(vec3 f0, vec3 tint) {
    return (f0 + (1.0 - f0) * 0.46266437) * (1.0 - tint) / 0.05665278;
}

vec3 openpbr_fresnel_f82(vec3 f0, vec3 tint, float cosine) {
    float x = 1.0 - clamp(cosine, 0.0, 1.0);
    float x5 = x * x * x * x * x;
    return clamp(f0 + (1.0 - f0) * x5 - openpbr_f82_dip(f0, tint) * (1.0 - x) * x5 * x, 0.0, 1.0);
}

// Signed amplitude into a medium of relative index eta; the magnitude takes max(eta, 1/eta), so a lower index never totally reflects.
float openpbr_amplitude(float cosine, float eta) {
    return sign(1.0 - eta) * sqrt(openpbr_fresnel_dielectric(cosine, max(eta, 1.0 / eta)));
}

// Airy reflectance of one film at three wavelengths (Born and Wolf) from the signed amplitudes above and below it and its optical path in nm.
vec3 openpbr_thin_film(float r12, vec3 r23, float opticalPath) {
    vec3 phase = 12.566370614359172 * opticalPath / vec3(650, 510, 475);
    vec3 interference = 2.0 * r12 * r23 * cos(phase);
    return clamp((r12 * r12 + r23 * r23 + interference) / (1.0 + r12 * r12 * r23 * r23 + interference), 0.0, 1.0);
}

struct OpenPbrBase {
    float eta;
    vec3 color;
    vec3 tint;
    float weight;
    float metal;
    float film;
    float filmIor;
    float filmEta;
    float filmBaseEta;
    float filmThickness;
};

// Reflectance of the dielectric and the metal fully covered by the thin film.
void openpbr_film_fresnel(OpenPbrBase b, float cosine, out vec3 dielectric, out vec3 metal) {
    float c = clamp(cosine, 0.0, 1.0);
    float into = max(b.filmEta, 1.0 / b.filmEta);
    float filmCosine = sqrt(max(0.0, 1.0 - (1.0 - c * c) / (into * into)));
    float r12 = openpbr_amplitude(c, b.filmEta);
    float opticalPath = b.filmIor * b.filmThickness * filmCosine;
    dielectric = openpbr_thin_film(r12, vec3(openpbr_amplitude(filmCosine, b.filmBaseEta)), opticalPath);
    metal = b.weight * openpbr_thin_film(r12, -sqrt(openpbr_fresnel_f82(b.color, b.tint, filmCosine)), opticalPath);
}

// Dielectric and metal Fresnel of the base at one cosine, the thin film laid over both by its weight.
void openpbr_base_fresnel(OpenPbrBase b, float cosine, out vec3 dielectric, out vec3 metal) {
    dielectric = vec3(openpbr_fresnel_dielectric(cosine, b.eta));
    metal = b.weight * openpbr_fresnel_f82(b.color, b.tint, cosine);
    if (b.film > 0.0) {
        vec3 dielectricFilm;
        vec3 metalFilm;
        openpbr_film_fresnel(b, cosine, dielectricFilm, metalFilm);
        dielectric = mix(dielectric, dielectricFilm, b.film);
        metal = mix(metal, metalFilm, b.film);
    }
}

// Anisotropic GGX with correlated Smith visibility, times the cosine and without Fresnel; vh returns the half-vector cosine for it.
float openpbr_ggx(vec3 n, vec3 v, vec3 l, float ax, float ay, float angle, out float vh) {
    float nv = dot(n, v);
    float nl = dot(n, l);
    vec3 sum = v + l;
    float sum2 = dot(sum, sum);
    vh = 1.0;
    if (nv <= 0.0 || nl <= 0.0 || sum2 < 1e-12) {
        return 0.0;
    }
    vec3 h = sum * inversesqrt(sum2);
    vh = dot(v, h);
    vec3 t;
    vec3 b;
    studio_basis(n, angle, t, b);
    vec3 scaledH = vec3(dot(h, t) / ax, dot(h, b) / ay, dot(h, n));
    float q = dot(scaledH, scaledH);
    float distribution = 1.0 / max(3.141592653589793 * ax * ay * q * q, 1e-20);
    float maskV = nl * length(vec3(ax * dot(v, t), ay * dot(v, b), nv));
    float maskL = nv * length(vec3(ax * dot(l, t), ay * dot(l, b), nl));
    return distribution * 0.5 / max(maskV + maskL, 1e-12) * nl;
}

// Charlie sheen (Estevez and Kulla 2017) with the Neubelt and Pettineo 2013 visibility, times the cosine.
float openpbr_sheen(vec3 n, vec3 v, vec3 l, float alpha) {
    float nv = dot(n, v);
    float nl = dot(n, l);
    if (nv <= 0.0 || nl <= 0.0) {
        return 0.0;
    }
    float nh = dot(n, normalize(v + l));
    float inverse = 1.0 / alpha;
    float distribution = (2.0 + inverse) * pow(max(1.0 - nh * nh, 0.0), 0.5 * inverse) / 6.283185307179586;
    return distribution / (4.0 * (nl + nv - nl * nv)) * nl;
}

// Hemispherical average of dielectric Fresnel, a quartic in eta - 1 fitted for eta 1 to 3 (debug/openpbr_fit), error below 0.004.
float openpbr_fresnel_average(float eta) {
    float x = eta - 1.0;
    return clamp((((-0.01049928 * x + 0.0501705) * x - 0.09786714) * x + 0.21478529) * x + 0.00376353, 0.0, 1.0);
}

// OpenPBR Surface: fuzz over a dielectric coat over a metal / dielectric base whose diffuse is the native relief shading.
vec3 openpbr_surface(
    vec3 base,
    vec3 diffuseBase,
    vec3 n,
    vec3 ns,
    vec3 v,
    vec3 keyDir,
    vec3 fillDir,
    float specAngle,
    float rough,
    float aoFactor,
    float variance,
    float footprint,
    float boundaryMask
) {
    vec3 key = srgb_to_linear(vec3(
        slope_attr.specular_color_r, slope_attr.specular_color_g, slope_attr.specular_color_b
    ));
    vec3 sky = srgb_to_linear(vec3(
        slope_attr.sky_color_r, slope_attr.sky_color_g, slope_attr.sky_color_b
    ));
    vec3 ground = srgb_to_linear(vec3(
        slope_attr.ground_color_r, slope_attr.ground_color_g, slope_attr.ground_color_b
    ));
    vec3 lightDirs[2] = vec3[](keyDir, fillDir);
    vec3 lightColors[2] = vec3[](key, sky * clamp(slope_attr.fill_intensity, 0.0, 1.0));
    float directScale = slope_attr.studio_direct_intensity * 3.0;
    float environmentAngle = radians(slope_attr.studio_environment_rotation)
                           + specAngle * clamp(slope_attr.studio_environment_follow, 0.0, 1.0);

    float coat = clamp(slope_attr.studio_clearcoat, 0.0, 1.0);
    float coatIor = clamp(slope_attr.openpbr_coat_ior, 1.0, 3.0);
    float coatRough = clamp(slope_attr.studio_clearcoat_roughness, 0.04, 1.0);
    if (variance > 0.0) {
        coatRough = clamp(pow(pow(coatRough, 4.0) + variance, 0.25), 0.04, 1.0);
    }
    // Waves on Clear Coat Only keeps the waves on the coat, as in Studio GGX; the fuzz above it follows the same normal.
    vec3 coatNormal = slope_attr.surface_waves_coat_only > 0.5 && slope_attr.surface_relief_waves > 0.0 &&
                      slope_attr.surface_lustre_relief > 0.5
                      ? normalize(vec3(-coat_wave_slope, 1.0)) : n;

    // A rough coat roughens the base beneath it, and the base's index is taken relative to the coat it sits under.
    float baseRough = mix(rough, min(1.0, pow(pow(rough, 4.0) + 2.0 * pow(coatRough, 4.0), 0.25)), coat);
    float ior = clamp(slope_attr.studio_ior, 1.0, 3.0);
    float eta = mix(ior, coatIor > ior ? coatIor / ior : ior / coatIor, coat);
    // Specular Intensity is OpenPBR's specular weight: it scales the dielectric's normal reflectance and the metal's Fresnel.
    float specularWeight = clamp(slope_attr.specular_intensity, 0.0, 1.0);
    float etaReflectance = pow((eta - 1.0) / (eta + 1.0), 2.0);
    float epsilon = sign(eta - 1.0) * sqrt(specularWeight * etaReflectance);

    OpenPbrBase b;
    b.eta = (1.0 + epsilon) / (1.0 - epsilon);
    b.color = clamp(base, 0.0, 1.0);
    b.tint = srgb_to_linear(vec3(slope_attr.openpbr_edge_tint_r, slope_attr.openpbr_edge_tint_g, slope_attr.openpbr_edge_tint_b));
    b.weight = specularWeight;
    b.metal = clamp(slope_attr.studio_metalness, 0.0, 1.0);
    b.film = clamp(slope_attr.surface_iridescence, 0.0, 1.0);
    b.filmIor = clamp(slope_attr.openpbr_film_ior, 1.0, 3.0);
    b.filmThickness = slope_attr.surface_film_thickness;
    // The film lies under the coat, so it is indexed against the coat as the base is; Specular Intensity weights the base below it.
    b.filmEta = b.filmIor / mix(1.0, coatIor, coat);
    float filmBase = ior / b.filmIor;
    float filmEpsilon = sign(filmBase - 1.0) * sqrt(specularWeight * pow((filmBase - 1.0) / (filmBase + 1.0), 2.0));
    b.filmBaseEta = (1.0 + filmEpsilon) / (1.0 - filmEpsilon);

    vec2 anisotropyAxis = studio_anisotropy(n);
    float anisotropy = clamp(anisotropyAxis.x, 0.0, 1.0);
    float alphaT = max(0.02, baseRough * baseRough) * sqrt(2.0 / (1.0 + (1.0 - anisotropy) * (1.0 - anisotropy)));
    float alphaB = max((1.0 - anisotropy) * alphaT, 0.005);
    float alpha = sqrt(alphaT * alphaB);
    float axis = anisotropyAxis.y;

    // Albedo of the base lobes toward the viewer: it lights the environment reflection and sets how much light the diffuse keeps.
    float nv = clamp(dot(ns, v), 0.0, 1.0);
    vec3 g = openpbr_ggx_albedo(nv, alpha);
    float e = g.x + g.y;
    float f0 = pow((b.eta - 1.0) / (b.eta + 1.0), 2.0);
    vec3 dielectricAlbedo = vec3(f0 * g.x + g.y);
    vec3 metalAlbedo = b.weight * (b.color * g.x + g.y - openpbr_f82_dip(b.color, b.tint) * g.z);
    if (b.film > 0.0) {
        vec3 dielectricFilm;
        vec3 metalFilm;
        openpbr_film_fresnel(b, nv, dielectricFilm, metalFilm);
        dielectricAlbedo = mix(dielectricAlbedo, dielectricFilm * e, b.film);
        metalAlbedo = mix(metalAlbedo, metalFilm * e, b.film);
    }
    vec3 dielectricBoost = openpbr_energy_compensation(vec3(f0), e);
    vec3 metalBoost = openpbr_energy_compensation(b.weight * b.color, e);
    dielectricAlbedo = min(dielectricAlbedo * dielectricBoost, 1.0);
    metalAlbedo *= metalBoost;

    vec3 direct = vec3(0);
    for (int i = 0; i < 2; ++i) {
        float vh;
        float lobe = openpbr_ggx(ns, v, lightDirs[i], alphaT, alphaB, axis, vh);
        if (lobe > 0.0) {
            vec3 dielectric;
            vec3 metal;
            openpbr_base_fresnel(b, vh, dielectric, metal);
            direct += lobe * mix(dielectric * dielectricBoost, metal * metalBoost, b.metal) * lightColors[i];
        }
    }
    direct *= directScale;
    vec3 env = studio_environment_source(reflect(-v, ns), environmentAngle, sky, ground, alpha, footprint)
             * mix(dielectricAlbedo, metalAlbedo, b.metal) * slope_attr.studio_environment_intensity * mix(1.0, aoFactor, 0.3);
    vec3 diffuse = diffuseBase * aoFactor * (1.0 - b.metal) * (1.0 - dielectricAlbedo);
    vec3 result = diffuse + (direct + env) * boundaryMask;

    if (coat > 0.0) {
        float coatAlpha = max(0.02, coatRough * coatRough);
        float cv = clamp(dot(coatNormal, v), 0.0, 1.0);
        vec3 gc = openpbr_ggx_albedo(cv, coatAlpha);
        float coatF0 = pow((coatIor - 1.0) / (coatIor + 1.0), 2.0);
        float coatBoost = openpbr_energy_compensation(vec3(coatF0), gc.x + gc.y).x;
        float coatAlbedo = min((coatF0 * gc.x + gc.y) * coatBoost, 1.0);
        // Coat Softbox reflects the lights as rectangles, with Schlick Fresnel at the coat's normal reflectance.
        bool softbox = slope_attr.studio_coat_softbox > 0.5;
        float lightAzimuths[2] = float[](specAngle, radians(slope_attr.fill_azimuth));
        vec3 coatLight = vec3(0);
        for (int i = 0; i < 2; ++i) {
            if (softbox) {
                coatLight += studio_softbox(coatNormal, v, lightDirs[i], lightAzimuths[i], coatRough, vec3(coatF0)) * lightColors[i];
            } else {
                float vh;
                float lobe = openpbr_ggx(coatNormal, v, lightDirs[i], coatAlpha, coatAlpha, 0.0, vh);
                coatLight += lobe * openpbr_fresnel_dielectric(vh, coatIor) * lightColors[i];
            }
        }
        coatLight *= directScale * coatBoost;
        coatLight += studio_environment_source(reflect(-v, coatNormal), environmentAngle, sky, ground, coatAlpha, footprint)
                   * coatAlbedo * slope_attr.studio_environment_intensity;

        // Coat Color absorbs along the refracted view; darkening is the light the coat reflects back down into the base.
        float refracted = sqrt(max(1.0 - (1.0 - cv * cv) / (coatIor * coatIor), 1e-4));
        vec3 coatColor = srgb_to_linear(vec3(slope_attr.openpbr_coat_color_r, slope_attr.openpbr_coat_color_g, slope_attr.openpbr_coat_color_b));
        vec3 absorption = pow(max(coatColor, 1e-6), vec3(1.0 / refracted));
        float roughInternal = 1.0 - (1.0 - openpbr_fresnel_average(coatIor)) / (coatIor * coatIor);
        float baseRoughness = mix(mix(1.0, rough, clamp(specularWeight * etaReflectance, 0.0, 1.0)), rough, b.metal);
        float internal = mix(openpbr_fresnel_dielectric(cv, coatIor), roughInternal, baseRoughness);
        vec3 baseAlbedo = mix(f0 + (1.0 - f0) * b.color, b.weight * b.color, b.metal);
        vec3 darkening = (1.0 - internal) / max(1.0 - baseAlbedo * internal, 1e-4);
        darkening = mix(vec3(1), darkening, coat * clamp(slope_attr.openpbr_coat_darkening, 0.0, 1.0));
        result = result * darkening * mix(vec3(1), (1.0 - coatAlbedo) * absorption, coat) + coatLight * coat * boundaryMask;
    }

    float fuzz = clamp(slope_attr.openpbr_fuzz, 0.0, 1.0);
    if (fuzz > 0.0) {
        float fuzzRough = clamp(slope_attr.openpbr_fuzz_roughness, 0.07, 1.0);
        vec3 fuzzColor = srgb_to_linear(vec3(slope_attr.openpbr_fuzz_color_r, slope_attr.openpbr_fuzz_color_g, slope_attr.openpbr_fuzz_color_b));
        float fuzzAlbedo = openpbr_fuzz_albedo(clamp(dot(coatNormal, v), 0.0, 1.0), fuzzRough);
        vec3 fuzzLight = vec3(0);
        for (int i = 0; i < 2; ++i) {
            fuzzLight += openpbr_sheen(coatNormal, v, lightDirs[i], fuzzRough * fuzzRough) * lightColors[i];
        }
        fuzzLight *= directScale;
        fuzzLight += studio_environment_source(reflect(-v, coatNormal), environmentAngle, sky, ground, 1.0, footprint)
                   * fuzzAlbedo * slope_attr.studio_environment_intensity;
        result = result * (1.0 - fuzz * fuzzAlbedo) + fuzzLight * fuzzColor * fuzz * boundaryMask;
    }
    return result;
}
